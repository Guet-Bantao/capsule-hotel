#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

#include <wifi_manager.h>


static int fd;
int flag_wifi=0;

fd_set uart_read;
//static pthread_t recv_tread_id;

static pthread_mutex_t send_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  send_con_var = PTHREAD_COND_INITIALIZER;


int set_uart(int fd,int boud,int flow_ctrl,int databits,int stopbits,int parity);
static void init_tty(int fd);


static int hand(char *recv,char *mach_str)
{
	if(strstr(recv,mach_str)!=0)
		return 1;
	else
		return 0;
}

#define MSGSIZE 256
static void *wifi_recv_thread(void *arg)
{
	//int flag_wifi = *(int *)arg;
	//int fd = fd_uart1;

	char *msg = calloc(1, MSGSIZE);
	int n = 0, total = 0;
	char begin = 1;
	fd_set rset;

	while(1)
	{
		
		FD_ZERO(&rset);
		FD_SET(fd, &rset);

		if(begin)
		{
			select(fd+1, &rset, NULL, NULL, NULL);

			if(FD_ISSET(fd, &rset))//检查某个fd在函数select调用后，相应位是否仍然为1
			{
				n = read(fd, msg+total, MSGSIZE-total);
				if(n > 0)
				{
					total += n;
					begin = 0;
					continue;
				}
			}
		}
		else
		{
			struct timeval tv = {0, 500*1000};
			if(select(fd+1, &rset, NULL, NULL, &tv) > 0)
			{
				if(FD_ISSET(fd, &rset))
				{
					n = read(fd, msg+total, MSGSIZE-total);
					if(n > 0)
					{
						total += n;
						continue;
					}
				}
			}
			else
			{
				printf("the recv[%d]: %s", total, msg);	
				if(hand((char *)msg,(char *)arg))
				{
					flag_wifi = 1;
				}

				total = 0;
				bzero(msg, MSGSIZE);
				begin = 1;

				/* 唤醒netprint的发送线程 */
				usleep(200000);
				pthread_mutex_lock(&send_mutex);
				pthread_cond_signal(&send_con_var);
				pthread_mutex_unlock(&send_mutex);

				if(flag_wifi == 1)
					break;
			}
		}
	}

	free(msg);
}

static int rm04_init(char * name)//"/dev/ttySAC1"
{
	fd = open(name, O_RDWR | O_NOCTTY);
	if (fd < 0)
	{
		printf("can't open /dev/%s\n",name);
		return -1;
	}
	
	/*测试是否为终端设备 */  
  	if(0 == isatty(STDIN_FILENO))
  	{
        printf("standard input is not a terminal device/n");
		return -1;
  	}

	//set_uart(fd,115200,0,8,1,'N');
	init_tty(fd);
	
	/* 将uart_read清零使集合中不含任何fd*/
	FD_ZERO(&uart_read);  
	/* 将fd加入uart_read集合 */
    FD_SET(fd,&uart_read);
	
	return 0;
}

int rm04_send_safe(char * send_str,char * mach_str)
{
	pthread_t tid;

	// ============ 独立线程读取串口信息 ============== //
	pthread_create(&tid, NULL, wifi_recv_thread, (void *)mach_str);

	while(1)
	{
		usleep(1000000);//1s 发一次数据
		pthread_mutex_lock(&send_mutex);
		printf("start send data : %s",send_str);
		write(fd, send_str, strlen(send_str));
		pthread_cond_wait(&send_con_var, &send_mutex);	
		pthread_mutex_unlock(&send_mutex);

		if(flag_wifi==1)
		{
			flag_wifi = 0;
			break;
		}
	}

	pthread_join(tid, NULL);

	return 0;
	
}

int rm04_send(char * send_str,int len)
{
	write(fd, send_str, strlen(send_str));
	return 0;
}

/*wifi_device->send_safe("AT\r\n","OK");//AT+CIPSEND=1920
wifi_device->send_safe("AT+CWMODE=1\r\n","OK");
wifi_device->send_safe("AT+CWMODE=3\r\n","OK");*/

static wifi_opr hlk_rm04_device_opr = {
	.name		= "hlk-rm04",
	.init		= rm04_init,
	.send_safe	= rm04_send_safe,
	.send		= rm04_send,
};

int rm04_register(void)
{
	return register_wifi_opr(&hlk_rm04_device_opr);
}


int set_uart(int fd,int boud,int flow_ctrl,int databits,int stopbits,int parity)  
{ 
	struct termios opt;
	int i;
	int baud_rates[] = { B9600, B19200, B57600, B115200, B38400 };
	int speed_rates[] = {9600, 19200, 57600, 115200, 38400};
	
	/*得到与fd指向对象的相关参数，并保存*/
	if (tcgetattr(fd, &opt) != 0)
	{
		printf("fail get fd data \n");
		return -1;
	}
	for ( i= 0;  i < sizeof(speed_rates) / sizeof(int);  i++)  
	{  
		if (boud == speed_rates[i])  
		{               
			cfsetispeed(&opt, baud_rates[i]);   
			cfsetospeed(&opt, baud_rates[i]);    
		}  
	} 
		
	/*修改控制模式，保证程序不会占用串口*/  
    opt.c_cflag |= CLOCAL;  
    /*修改控制模式，使得能够从串口中读取输入数据 */ 
    opt.c_cflag |= CREAD;

	/*设置数据流控制 */ 
	switch(flow_ctrl)  
	{  
   	case 0 ://不使用流控制  
          	opt.c_cflag &= ~CRTSCTS;  
          	break;     
   	case 1 ://使用硬件流控制  
          	opt.c_cflag |= CRTSCTS;  
          	break;  
   	case 2 ://使用软件流控制  
          	opt.c_cflag |= IXON | IXOFF | IXANY;  
          	break;  
	default:     
            printf("Unsupported mode flow\n");  
            return -1;
	}

	/*屏蔽其他标志位  */
    opt.c_cflag &= ~CSIZE;

	/*设置数据位*/
	switch (databits)  
	{    
   	case 5    :  
                 opt.c_cflag |= CS5;  
                 break;  
   	case 6    :  
                 opt.c_cflag |= CS6;  
                 break;  
   	case 7    :      
             	opt.c_cflag |= CS7;  
             	break;  
   	case 8:      
             	opt.c_cflag |= CS8;  
             	break;    
   	default:     
             	printf("Unsupported data size\n");  
             	return -1;   
    	} 

	/*设置校验位 */ 
   	 switch (parity)  
	{    
   	case 'n':  
   	case 'N': //无奇偶校验位。  
             	opt.c_cflag &= ~PARENB;   
             	opt.c_iflag &= ~INPCK;      
             	break;   
   	case 'o':    
   	case 'O'://设置为奇校验      
            	 opt.c_cflag |= (PARODD | PARENB);   
            	 opt.c_iflag |= INPCK;               
             	break;   
   	case 'e':   
   	case 'E'://设置为偶校验    
           	 opt.c_cflag |= PARENB;         
            	 opt.c_cflag &= ~PARODD;         
            	 opt.c_iflag |= INPCK;        
            	 break;  
  	 	case 's':  
   	case 'S': //设置为空格   
             	opt.c_cflag &= ~PARENB;  
             	opt.c_cflag &= ~CSTOPB;  
             	break;   
    	default:    
             	printf("Unsupported parity\n");      
             	return -1;   
	 }

	 /* 设置停止位 */  
	switch (stopbits)  
	{    
   	case 1:     
             	opt.c_cflag &= ~CSTOPB;
		break;   
   	case 2:     
             	opt.c_cflag |= CSTOPB; 
		break;  
   	default:     
                 printf("Unsupported stop bits\n");   
                   return -1;  
	}

	/*设置使得输入输出时没接收到回车或换行也能发送*/
	//opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	opt.c_oflag &= ~OPOST;//原始数据输出
	/*下面两句,区分0x0a和0x0d,回车和换行不是同一个字符*/
    	opt.c_oflag &= ~(ONLCR | OCRNL); 

	opt.c_iflag &= ~(ICRNL | INLCR);
	/*使得ASCII标准的XON和XOFF字符能传输*/
    	opt.c_iflag &= ~(IXON | IXOFF | IXANY);    //不需要这两个字符

	/*如果发生数据溢出，接收数据，但是不再读取*/
	tcflush(fd, TCIFLUSH);
	
	/*如果有数据可用，则read最多返回所要求的字节数。如果无数据可用，则read立即返回0*/
    	opt.c_cc[VTIME] = 0;        //设置超时
    	opt.c_cc[VMIN] = 0;        //Update the Opt and do it now

	/*
	*使能配置
	*TCSANOW：立即执行而不等待数据发送或者接受完成
	*TCSADRAIN：等待所有数据传递完成后执行
	*TCSAFLUSH：输入和输出buffers  改变时执行
	*/
	if (tcsetattr(fd,TCSANOW,&opt) != 0)
	{
		printf("uart set error!\n");
		return -1; 
	}
	return 0;
	
}

static void init_tty(int fd)
{
	struct termios new_cfg, old_cfg;
	if(tcgetattr(fd, &old_cfg) != 0)
	{
		perror("tcgetattr() failed");
		exit(0);
	}

	bzero(&new_cfg, sizeof(new_cfg));

	new_cfg = old_cfg;
	cfmakeraw(&new_cfg);

	cfsetispeed(&new_cfg, B115200);
	cfsetospeed(&new_cfg, B115200);

	new_cfg.c_cflag |= CLOCAL | CREAD;

	new_cfg.c_cflag &= ~CSIZE;
	new_cfg.c_cflag |= CS8;

	new_cfg.c_cflag &= ~PARENB;
	new_cfg.c_cflag &= ~CSTOPB;

	new_cfg.c_cc[VTIME] = 0;
	new_cfg.c_cc[VMIN]  = 1;
	tcflush(fd, TCIFLUSH);

	if(tcsetattr(fd, TCSANOW, &new_cfg) != 0)
	{
		perror("tcsetattr() failed");
		exit(0);
	}
}


	
