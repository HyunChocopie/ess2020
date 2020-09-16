#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "ku_sa.h"
#define DEV_NAME "ku_sa_dev"
MODULE_LICENSE("GPL");

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2

#define SIMPLE_IOCTL_NUM 'z'
#define IOCTL_SENSE _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM1, unsigned long*)
#define IOCTL_ACT _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM2, unsigned long*)


#define SWITCH 17
#define SPEAKER 12
#define LED 26

spinlock_t spinlock;
static int index=0;
struct timestamp{
	int value;
	int month;
	int day;
};

struct sense_list{
	struct list_head list;
	int time;
};

struct sense_list mylist;

struct timestamp my_tasklet_data;
struct tasklet_struct my_tasklet;

void tasklet_func(unsigned long recv_data){
	struct timestamp *temp;
	unsigned long flags;
	temp=(struct timestamp*)recv_data;
	printk("today is %d / %d\n",temp->month,temp->day);
	
	spin_lock_irqsave(&spinlock,flags);
	//queue에 센싱된정보 넣기
	struct sense_list *tmp=0;
	tmp=(struct sense_list*)kmalloc(sizeof(struct sense_list),GFP_KERNEL);
	//tmp->time=temp->day;
	index++;
	tmp->time=index;
	printk("%d,%d\n",index,tmp->time);
	list_add(&tmp->list,&mylist.list);
	kfree(tmp);
	spin_unlock_irqrestore(&spinlock,flags);
}



static void play(int note){
	int i=0;
	for(i=0;i<100;i++){
		gpio_set_value(SPEAKER,1);
		udelay(note);
		gpio_set_value(SPEAKER,0);
		udelay(note);
	}
}

static int speaker_init(int v){
	int notes[]={1911,1702,1516};
	/*
	if(v==1)
		int notes[]={1911,1516,1275};
	else
		int notes[]={1275,1516,1911};
	*/
	int i=0;
	gpio_request_one(SPEAKER,GPIOF_OUT_INIT_LOW,"SPEAKER");

	for(i=0;i<3;i++){
		play(notes[i]);
		mdelay(500);
	}
	gpio_set_value(SPEAKER,0);
	return 0;
}

static int irq_num;
static irqreturn_t switch_irq_isr(int irq, void* dev_id){
	printk("switch_irq detect\n");
	int ret;
	
	tasklet_schedule(&my_tasklet);

	return IRQ_HANDLED;
}


static long ku_sa_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	int ret = 0;
	//struct param *argparam=(struct param*)vmalloc(sizeof(struct param));
	//copy_from_user(argparam,(struct param*)arg, sizeof(struct param));
	int ret_value=-1;
	switch(cmd){
		case IOCTL_SENSE:
			printk("ioctl sense\n");
			unsigned long flags;
			struct sense_list *tmp=0;
			struct list_head *pos=0;
			struct list_head *q=0;
			
			spin_lock_irqsave(&spinlock,flags);
			if(list_empty(&mylist.list)){ //empty
				printk("empty");
				spin_unlock_irqrestore(&spinlock,flags);
				return -1;
			}
			
			
			list_for_each_safe(pos,q,&mylist.list){
				tmp=list_entry(pos,struct sense_list,list);
				printk("%d",tmp->time);
				ret_value=tmp->time;
				list_del(pos);
				kfree(tmp);
				break;
			}
			spin_unlock_irqrestore(&spinlock,flags);
			printk("*switch\n");
			return ret_value;

		case IOCTL_ACT:
			printk("ioctl act\n");
			printk("%d\n",arg);
			if(arg>2000){	//밝으면
				//SPEAKER, turn off LED
				speaker_init(0);
				gpio_request_one(LED,GPIOF_OUT_INIT_LOW,"LED");
				gpio_set_value(LED,0);
				return 0;
				
			}
			else{		//어두우면
				//SPEAKER, turn on LED
				speaker_init(1);
				gpio_request_one(LED,GPIOF_OUT_INIT_LOW,"LED");
				gpio_set_value(LED,1);
				return 1;
			}
			//return ret;			
			break;

	}

	return 0;

}

static int ku_sa_open(struct inode *inode, struct file *file){
	printk("ku_sa_open\n");
	return 0;

}

static int ku_sa_release(struct inode *inode, struct file *file){
	printk("ku_sa_release\n");
	return 0;

}

static const struct file_operations ku_sa_fops = {
	.unlocked_ioctl = ku_sa_ioctl,
	.open = ku_sa_open,
	.release = ku_sa_release,
	.read = seq_read,

};


static dev_t dev_num;
static struct cdev *cd_cdev;

static int __init ku_sa_init(void){
	int ret;

	printk("ku_sa : Init Module\n");
	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &ku_sa_fops);
	ret = cdev_add(cd_cdev, dev_num, 1);
	if (ret <0){
		printk("fail to add character device \n");
		return -1;
	}

	my_tasklet_data.month=6;
	my_tasklet_data.day=25;
	tasklet_init(&my_tasklet,tasklet_func,(unsigned long)&my_tasklet_data);

	INIT_LIST_HEAD(&mylist.list);	

	spin_lock_init(&spinlock);

	//my_wq=create_workqueue("my_workqueue");

	gpio_request_one(SWITCH,GPIOF_IN,"SWITCH");
	irq_num=gpio_to_irq(SWITCH);
	ret=request_irq(irq_num,switch_irq_isr,IRQF_TRIGGER_FALLING, "sensor_irg",NULL);
	if(ret){
		printk("swtich_irg unable to reset request IRQ : %d\n", irq_num);
		free_irq(irq_num,NULL);
	}
	else{
		printk("switch_irg enable to set request IRQ : %d\n", irq_num);
	}
	
	return 0;

}


static void __exit ku_sa_exit(void){
	printk("ku_sa : exit module\n");
	//int i;	
	//
	tasklet_kill(&my_tasklet);
	
	struct sense_list *tmp;
	struct list_head *pos;
	struct list_head *q;
	unsigned int i=0;

	list_for_each_safe(pos,q,&mylist.list){
		tmp=list_entry(pos,struct sense_list,list);
		list_del(pos);
		kfree(tmp);
		i++;
	}
	
	//flush_workqueue(my_wq);
	//destroy_workqueue(my_wq);
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num,1);
	disable_irq(irq_num);
	free_irq(irq_num,NULL);
	gpio_free(SWITCH);
	gpio_free(SPEAKER);
	gpio_free(LED);

}


module_init(ku_sa_init);
module_exit(ku_sa_exit);





