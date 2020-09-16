#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "ku_lib.c"

int main(int args,char* argv[]){
	/*
	int dev;
	dev=open("/dev/ku_sa_dev",O_RDWR);
	//printf("%d",ioctl(dev,IOCTL_SENSE,NULL));
	int swit=ioctl(dev,IOCTL_SENSE,NULL);
	//sleep(1);
	if(swit>=0){
		ioctl(dev,IOCTL_ACT,lightsense());
	}
	//close(dev);
	*/

	int dev=dev_open();
	
	if(ku_sense(dev)==-1){
		printf("app sensor queue empty\n");
	}else{
		printf("app act");
		ku_act(dev);
	}
	dev_close(dev);
	
	return 0;
}
