
#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2

#define SIMPLE_IOCTL_NUM 'z'
#define IOCTL_SENSE _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM1, unsigned long*)
#define IOCTL_ACT _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM2, unsigned long*)

int transfer_spi(int spi_fd,unsigned char *tbuf, unsigned char *rbuf, int len);
void setup_spi(int spi_fd,int speed,int mode);
int lightsense();
int dev_open();
void dev_close(int);
int ku_sense(int);
int ku_act(int);
