#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>


#include <wifi_manager.h>




int sockfd;

static int rtl8188eus_init(char * name)//"/dev/ttySAC1"
{
	struct sockaddr_in server_addr;
	
	/*客户端程序开始建立sockfd描述符*/
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
		printf("Socket Error:%s\a\n",strerror(errno));
		return -1;
	}

	/*客户端程序填充服务端的资料*/
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5500);
    server_addr.sin_addr.s_addr = inet_addr(name);// IP Addr

	/*客户端程序发起连接请求*/
    if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr)) == -1){
		printf("Connect Error:%s\a\n",strerror(errno));
		return -1;
	}
	
	return 0;
}

int rtl8188eus_send(char * send_str,int len)
{
	if(len != write(sockfd,send_str,len))
	{
		printf("wifi send data Error\a\n");
		return -1;
	}
	
	return 0;
}

int rtl8188eus_receive(char *rcv_buf, int len)
{
	int rcv = 0 ;
	int read_flag;
	fd_set rset;
	struct timeval my_time;

	/*不阻塞*/
	my_time.tv_sec = 0;  
    my_time.tv_usec = 0; 

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	
	read_flag = select(sockfd+1,&rset,NULL,NULL,&my_time);
	if(read_flag)
		rcv = read(sockfd,rcv_buf,len);
	
	return rcv;
}

int rtl8188eus_exit(void )
{
	/*结束通信*/
    close(sockfd);
	
	return 0;
}


static wifi_opr rtl8188eus_device_opr = {
	.name		= "rtl8188eus",
	.init		= rtl8188eus_init,
	//.send_safe	= rtl8188eus_send_safe,
	.send		= rtl8188eus_send,
	.receive	= rtl8188eus_receive,
	.exit		= rtl8188eus_exit,
};

int rtl8188eus_register(void)
{
	return register_wifi_opr(&rtl8188eus_device_opr);
}





