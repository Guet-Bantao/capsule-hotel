#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

#include <dirent.h>

void init_tty(int fd)
{
	struct termios new_cfg, old_cfg;
	if(tcgetattr(fd, &old_cfg) != 0)
	{
		perror("tcgetattr() failed");
		return;
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

	tcsetattr(fd, TCSANOW, &new_cfg);
}

void *routine(void *arg)
{
	int fd = (int)arg;

	char *msg = calloc(1, 1024);
	int n;
	while(1)
	{
		bzero(msg, 1024);
		n = read(fd, msg, 1024);
		if(n <= 0)
		{
			perror("read() failed");
			break;
		}
		fprintf(stderr, "%s", msg);
	}
	free(msg);
}


int main(void)
{
	// 输入要打开的串口文件，默认是/dev/ttySAC1
	// GEC210是/dev/s3c2410_serial1
	fprintf(stderr, "serial port:[/dev/ttySAC1] ? ");

	char *device = calloc(1, 32);
	fgets(device, 32, stdin);

	if(device[0] == '\n')
		strcpy(device, "/dev/ttyS1");
	else
		device[strlen(device)-1] = '\0';


	// 打开指定的串口设备节点，并初始化
	int fd = open(device, O_RDWR|O_NOCTTY);
	if(fd == -1)
	{
		fprintf(stderr, "open() %s failed: %s\n",
				device, strerror(errno));
		exit(0);
	}
	init_tty(fd);

	// ============ 独立线程读取串口信息 ============== //
	pthread_t tid;
	pthread_create(&tid, NULL, routine, (void*)fd);


	// ============ 主线程发送信息给串口 ============ //
	int n;
	char *AT_RST = "AT+RST\r\n";
	char *AT_CWMODE1 = "AT+CWMODE=1\r\n";
	char *AT_CWMODE2 = "AT+CWMODE=2\r\n";
	char *AT_CWMODE3 = "AT+CWMODE=3\r\n";

	char *AT_CWSAP = "AT+CWSAP=\"ESP8266_vincent\",\"123456789\",11,3\r\n";
	char *AT_CIPMUX1 = "AT+CIPMUX=1\r\n";
	char *AT_CIPSERVER = "AT+CIPSERVER=1,8080\r\n";

	char *AT_CWLAP = "AT+CWLAP\r\n";
	char *AT_CWJAP = "AT+CWJAP=\"KITE\",\"dbt18290019557\"\r\n";

	char *AT_CIFSR = "AT+CIFSR\r\n";

	char *AT_CIPSTART0 = "AT+CIPSTART=0,\"TCP\",\"192.168.4.1\",8080\r\n";
	char *AT_CIPSTART = "AT+CIPSTART=\"TCP\",\"192.168.4.1\",8080\r\n";

	char *AT_CIPSEND0 = "AT+CIPSEND=0,10\r\n";
	char *AT_CIPSEND = "AT+CIPSEND=10\r\n";

	char *menu[] = {
		[0] = AT_RST,
		[1] = AT_CWMODE1,
		[2] = AT_CWMODE2,
		[3] = AT_CWMODE3,
		
		[4] = AT_CWSAP,
		[5] = AT_CIPMUX1,
		[6] = AT_CIPSERVER,
		
		[7] = AT_CWLAP,
		[8] = AT_CWJAP,
		
		[9] = AT_CIFSR,
		
		[10] = AT_CIPSTART0,
		[11] = AT_CIPSTART,
		
		[12] = AT_CIPSEND0,
		[13] = AT_CIPSEND,
	};
	
	int i;
	while(1)
	{
		scanf("%d", &i);
		write(fd, menu[i], strlen(menu[i]));
	}

	return 0;
}
