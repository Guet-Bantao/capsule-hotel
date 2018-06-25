#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <wifi_manager.h>
#include <camera_manager.h>
//#include <lcd_manager.h>
#include <convert_manager.h>
#include <merge.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

#include <alsa/asoundlib.h>
#include <math.h> 


static void *pictuer_send_thread(void *arg);
static void *sensor_send_thread(void *arg);
static void *receive_thread(void *arg);
static void *play_thread(void *arg);
static void *sleep_detection_thread(void *arg);




static pthread_mutex_t send_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  send_con_var = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t alsa_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  alsa_con_var = PTHREAD_COND_INITIALIZER;
int choice_play = 0;

static pthread_mutex_t socket_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t safe_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sleep_mutex  = PTHREAD_MUTEX_INITIALIZER;




wifi_opr  *wifi_device;

int hc_04_distance(void)
{
	int fd_hc04;
	int distance=0;
	
	fd_hc04 = open("/dev/hc04", O_RDWR);
	if (fd_hc04 < 0)
	{
		printf("can't open hc04 dev\n");
		return -1;
	}
	read(fd_hc04,&distance,1);
	printf("distance is %d us\n",distance);

	if(distance>=2300)
	{
			/* ���ѷ����߳� */
			pthread_mutex_lock(&send_mutex);
			pthread_cond_signal(&send_con_var);
			pthread_mutex_unlock(&send_mutex);
		
			printf("start take a pictuer at distance is %d us\n",distance);
	}
	else
	{
	}
	return 0;
}

//====���������ǵ�ƫ��===========
#define Gy_offset -0.8157
#define pi 3.1415
int LastTime=0;
float Get_Angle(int mpu6050[6])//ax ay az gx gy gz
{
	float Gyro_x,Gyro_y,Gyro_z;//�洢����������� ��λ ��/s
	float Angle_x,Angle_y,Angle_z;//��λ g(9.8m/s^2)
	float Gyro_accX,Gyro_accY;//�洢���ֺ�ĵļ��ٶ�
	float Angle_accX,Angle_accY,Angle_accZ;//�洢���ٶȼ�����ĽǶ�
	long NowTime,TimeSpan;//�����Խ��ٶȻ��ֵ�	
	struct timeval tv_start, tv_end;          //��¼��¼ʱ�俪ʼ�ͽ�������������
	
	Angle_y = mpu6050[1];
	Angle_z = mpu6050[2];
	Gyro_y = mpu6050[4];
	Gyro_z = mpu6050[5];
	//printf("Angle_y is %d ,Angle_z is %d,Gyro_y is %d,Gyro_z is %d\n",mpu6050[1],mpu6050[2],mpu6050[4],mpu6050[5]);
	
	//�Լ��ٶȽ�����������������16384�ó���λΪg�ļ��ٶ�ֵ +-2g����
	Angle_y=(Angle_y+65)/16384.00;//ȥ�����ƫ��
	Angle_z=(Angle_z+2700)/16384.00;//ȥ�����ƫ��
	//�ü��ٶȼ����������ˮƽ������ϵ֮��ļн�,�ǶȽ�Сʱ��x=sinx�õ��Ƕȣ����ȣ�, deg = rad*180/3.14
	Angle_accY= atan(Angle_y / Angle_z)*180/ pi;   //��ýǶ�ֵ������-1����
	//�Խ��ٶ������� ����16.4�ó���λΪ ��/sֵ +-250�������ĸ��������� 250,500,1000,2000��Ӧ��GYRO_Sensitivity�ֱ�Ϊ131,65.5,32.8,16.4
	Gyro_y=Gyro_y/131.0;
	Gyro_z=Gyro_z/131.0; 
//===============�����Ƕ�ʱ����л��ִ���================
	gettimeofday(&tv_start, NULL);		//��¼��ʼ��ʱʱ��tpstart 
	NowTime=tv_start.tv_usec;//��ȡ��ǰ�������еĺ�����
	TimeSpan=NowTime-LastTime;//����ʱ�������㲻�Ǻ��Ͻ�
	gettimeofday(&tv_end, NULL);
	LastTime=tv_end.tv_usec;
	/*gettimeofday(&tv_start, NULL);		//��¼��ʼ��ʱʱ��tpstart 
	NowTime=tv_start.tv_usec;//��ȡ��ǰ�������еĺ�����
	TimeSpan=NowTime-LastTime;//����ʱ�������㲻�Ǻ��Ͻ�
	LastTime=NowTime;*/
//ͨ���Խ��ٶȻ���ʵ�ָ�����ĽǶȲ�����
	//Gyro_accY=(Gyro_y-(Gy_offset))*TimeSpan/1000/1000;
	Gyro_accY=(Gyro_y-(Gy_offset))*0.4;
	Angle_accY = 0.8 * Angle_accY+ (1-0.8) * (Angle_accY + Gyro_accY);//һ�׻����˲�
	
	return Angle_accY;
}

//��������������
#define dt           0.012//0.012//Kalman�˲�������ʱ��
#define R_angle      0.5
#define Q_angle      0.001
#define Q_gyro       0.003//Խ��Խ�ͺ�
float Q_bias_x = 0;
float Kalman_Filter_X(float Accel, float Gyro) //����������
{
	float Angle_X_Final;
	float Angle_err_x;
	float roll; 		//ŷ����
	float PCt_0, PCt_1, E;
	float K_0, K_1, t_0, t_1;
	char  C_0 = 1;
	float Gyro_x;		    //X�������������ݴ�
	float Pdot[4] = { 0,0,0,0 };
	float PP[2][2] = { { 1, 0 },{ 0, 1 } };
	
	Angle_X_Final += (Gyro - Q_bias_x) * dt; //�������

	Pdot[0] = Q_angle - PP[0][1] - PP[1][0]; // Pk-����������Э�����΢��

	Pdot[1] = -PP[1][1];
	Pdot[2] = -PP[1][1];
	Pdot[3] = Q_gyro;

	PP[0][0] += Pdot[0] * dt;   // Pk-����������Э����΢�ֵĻ���
	PP[0][1] += Pdot[1] * dt;   // =����������Э����
	PP[1][0] += Pdot[2] * dt;
	PP[1][1] += Pdot[3] * dt;

	Angle_err_x = Accel - Angle_X_Final;	//zk-�������

	PCt_0 = C_0 * PP[0][0];
	PCt_1 = C_0 * PP[1][0];

	E = R_angle + C_0 * PCt_0;

	K_0 = PCt_0 / E;
	K_1 = PCt_1 / E;

	t_0 = PCt_0;
	t_1 = C_0 * PP[0][1];

	PP[0][0] -= K_0 * t_0;		 //����������Э����
	PP[0][1] -= K_0 * t_1;
	PP[1][0] -= K_1 * t_0;
	PP[1][1] -= K_1 * t_1;

	Angle_X_Final += K_0 * Angle_err_x;	 //�������
	Q_bias_x += K_1 * Angle_err_x;	 //�������
	Gyro_x = Gyro - Q_bias_x;	 //���ֵ(�������)��΢��=���ٶ�
	
	roll=Angle_X_Final;
	if(roll<-90)
	{
		roll=-roll;
		roll=180-(roll-90.0)+90;
	}
	return Angle_X_Final;
}

//�Ƕȼ���
float Angle_Calcu(int mpu6050[6])
{
	//��ΧΪ2gʱ�������ϵ��16384 LSB/g
	//deg = rad*180/3.14
	float x=0, y=0, z=0;
	float Accel_x;	     	//X����ٶ�ֵ�ݴ�
	float Accel_y;	     	//Y����ٶ�ֵ�ݴ�
	float Accel_z;	     	//Z����ٶ�ֵ�ݴ�
	float Gyro_x;		    //X�������������ݴ�
	float Gyro_y;    		//Y�������������ݴ�
	float Gyro_z;		    //Z�������������ݴ�

	float Angle_x_temp;  	//�ɼ��ٶȼ����x��б�Ƕ�
	float Angle_y_temp;  	//�ɼ��ٶȼ����y��б�Ƕ�
	float Angle_z_temp;		//�ɼ��ٶȼ����z��б�Ƕ�

	Accel_x = mpu6050[0]; //x����ٶ�ֵ�ݴ�
	Accel_y = mpu6050[1]; //y����ٶ�ֵ�ݴ�
	Accel_z = mpu6050[2]; //z����ٶ�ֵ�ݴ�
	Gyro_x = mpu6050[3];  //x��������ֵ�ݴ�
	Gyro_y = mpu6050[4];  //y��������ֵ�ݴ�
	Gyro_z = mpu6050[5];  //z��������ֵ�ݴ�

	//����x����ٶ�
	if (Accel_x<32768)
		x = Accel_x / 16384;
	else
		x = 1 - (Accel_x - 49152) / 16384;
	//����y����ٶ�
	if (Accel_y<32768)
		y = Accel_y / 16384;
	else 
		y = 1 - (Accel_y - 49152) / 16384;
	//����z����ٶ�
	if (Accel_z<32768)
		z = Accel_z / 16384;
	else
		z = (Accel_z - 49152) / 16384;

	//�ü��ٶȼ����������ˮƽ������ϵ֮��ļн�
	Angle_x_temp = (atan2(z , y)) * 180 / pi;//atan2(z , y)
	Angle_y_temp = (atan2(x , z)) * 180 / pi;
	Angle_z_temp = (atan2(y , x)) * 180 / pi;

	//�Ƕȵ�������
	if (Accel_y<32768) Angle_y_temp = -Angle_y_temp;
	if (Accel_y>32768) Angle_y_temp = +Angle_y_temp;
	if (Accel_x<32768) Angle_x_temp = +Angle_x_temp;
	if (Accel_x>32768) Angle_x_temp = -Angle_x_temp;
	if (Accel_z<32768) Angle_z_temp = +Angle_z_temp;
	if (Accel_z>32768) Angle_z_temp = -Angle_z_temp;
	
	//���ٶ�
	//��ǰ�˶�
	if (Gyro_x<32768) Gyro_x = -(Gyro_x / 16.4);//��ΧΪ1000deg/sʱ�������ϵ��16.4 LSB/(deg/s)
	//����˶�
	if (Gyro_x>32768) Gyro_x = +(65535 - Gyro_x) / 16.4;
	//��ǰ�˶�
	if (Gyro_y<32768) Gyro_y = -(Gyro_y / 16.4);//��ΧΪ1000deg/sʱ�������ϵ��16.4 LSB/(deg/s)
	//����˶�
	if (Gyro_y>32768) Gyro_y = +(65535 - Gyro_y) / 16.4;
	//��ǰ�˶�
	if (Gyro_z<32768) Gyro_z = -(Gyro_z / 16.4);//��ΧΪ1000deg/sʱ�������ϵ��16.4 LSB/(deg/s)
	//����˶�
	if (Gyro_z>32768) Gyro_z = +(65535 - Gyro_z) / 16.4;

	return Kalman_Filter_X(Angle_x_temp, Gyro_x);	//�������˲�����X���
}

int safe=0;
int my_sleep=0;
int main(int argc, char **argv)
{
	int error;

	pthread_t tid_pic;
	pthread_t tid_sensor;
	
	int fd_ads1115;

	if (argc != 2)
    {
        printf("Usage:\n");
        printf("%s </dev/video0,1,...>\n", argv[0]);
        return -1;
    }

	/* register wifi devices */
	wifi_init();
	error = wifi_device_init(argv[1], &wifi_device);//"192.168.43.32"
	if (error)
    {
        printf("wifi Init for %s error!\n",argv[1]);
        return -1;
    }
	

	fd_ads1115 = open("/dev/ads1115", O_RDWR);//ads1115 16bit ADC
	if (fd_ads1115 < 0)
	{
		printf("can't open ads1115 dev\n");
		return -1;
	}

	//============�����߳���������ͼƬ=================================//
	pthread_create(&tid_pic, NULL, pictuer_send_thread, NULL);
	//============�����߳�������������=================================//
	pthread_create(&tid_sensor, NULL, sensor_send_thread, &fd_ads1115);
	//============�����߳�������������=================================//
	pthread_create(&tid_sensor, NULL, receive_thread, NULL);
	//============�����߳�������������=================================//
	pthread_create(&tid_sensor, NULL, play_thread, NULL);
	//============�����߳�����˯�߼��=================================//
	pthread_create(&tid_sensor, NULL, sleep_detection_thread, NULL);

	while (1)
    {	
		pthread_mutex_lock(&safe_mutex);
		if(safe)
		{
			pthread_mutex_unlock(&safe_mutex);
			hc_04_distance();
		}
		else
		{
			pthread_mutex_unlock(&safe_mutex);
		}
		
		usleep(1000000);//run once a second
	}
	
	return 0;
}

static void *sleep_detection_thread(void *arg)
{
	int mpu6050[6]={0};//ax ay az gx gy gz
	float angle=0,standard=0,error=0;
	int fd_mpu6050;
	int can=1;
	char send_sleep_data[18]={"011000000001FFFF\n"};

	fd_mpu6050 = open("/dev/mpu6050", O_RDWR);//mpu6050
	if (fd_mpu6050 < 0)
	{
		printf("can't open mpu6050 dev\n");
		return -1;
	}
	read(fd_mpu6050,mpu6050,6);
	angle = Angle_Calcu(mpu6050);
	standard = angle;
	printf("standard angle is %f \n",standard);

	while(1)
	{
		pthread_mutex_lock(&sleep_mutex);
		if(my_sleep==1)
		{
			pthread_mutex_unlock(&sleep_mutex);
			read(fd_mpu6050,mpu6050,6);
			//printf("Angle_y is %d ,Angle_z is %d,Gyro_y is %d,Gyro_z is %d\n",mpu6050[1],mpu6050[2],mpu6050[4],mpu6050[5]);
			//angle = Get_Angle(mpu6050);
			angle = Angle_Calcu(mpu6050);
			error = standard - angle;
			if((error<-0.9)&&(can==1))
			{
				can = 0;
				pthread_mutex_lock(&socket_mutex);//������ƽ��
				
				if(wifi_device->send(send_sleep_data,strlen(send_sleep_data)))//��������
				{
					printf("send sleep data error\n");
				}
				pthread_mutex_unlock(&socket_mutex);//������ƽ��
				//printf("error angle is %f \n",error);
			}
			if(error>-0.75)
			{
				can = 1;
			}
			
		}
		else
		{
			pthread_mutex_unlock(&sleep_mutex);
		}
		usleep(500000);
	}
	
	return (void *)0;
}

static void *sensor_send_thread(void *arg)
{
	int fd_ads1115 = *(int *)arg;
	int fd_dht11;
	
	int ads1115_in1 = 0;
	//int ads1115_in2 = 0;
	//int ads1115_in3 = 0;
	int ads1115_in0 = 0;

	char chanel[]={0,1,2,3};
	char send_voltage[18]={0};
	char dht11_data[4]={0};

	fd_dht11 = open("/dev/dht11", O_RDWR);//dht11
	if (fd_dht11 < 0)
	{
		printf("can't open dht11 dev\n");
		return (void *)-1;
	}
	
	while(1)
	{
		write(fd_ads1115,&chanel[0],1);//ads1115 chanel 0
		read(fd_ads1115,&ads1115_in0,1);
		write(fd_ads1115,&chanel[1],1);//ads1115 chanel 1
		read(fd_ads1115,&ads1115_in1,1);

		read(fd_dht11,dht11_data,4);
		
		//write(fd_ads1115,&chanel[2],1);//ads1115 chanel 2
		//read(fd_ads1115,&ads1115_in2,1);
		//write(fd_ads1115,&chanel[3],1);//ads1115 chanel 3
		//read(fd_ads1115,&ads1115_in3,1);
		//printf("the  in0 is %d , in1 is %d , in2 is %d , in3 is %d \n",ads1115_in0,ads1115_in1,ads1115_in2,ads1115_in3);
		//printf("the  in0 is %d , in1 is %d \n",ads1115_in0,ads1115_in1);

		sprintf(send_voltage,"%s%4d%s","01030000",ads1115_in0,"FFFF\n");//Coֵ
		pthread_mutex_lock(&socket_mutex);//������ƽ��
		if(wifi_device->send(send_voltage,strlen(send_voltage)))//��������
		{
			printf("send a pictuer error\n");
		}
		sprintf(send_voltage,"%s%4d%s","01040000",ads1115_in1,"FFFF\n");//����ֵ
		if(wifi_device->send(send_voltage,strlen(send_voltage)))//��������
		{
			printf("send a pictuer error\n");
		}
		//printf("the humi is %d %d temp is %d %d\n",dht11_data[0],dht11_data[1],dht11_data[2],dht11_data[3]);//��ʾ��ʪ��
		sprintf(send_voltage,"%s%4d%s","01020000",dht11_data[0],"FFFF\n");//ʪ��
		if(wifi_device->send(send_voltage,strlen(send_voltage)))//��������
		{
			printf("send a pictuer error\n");
		}
		sprintf(send_voltage,"%s%4d%s","01010000",dht11_data[2],"FFFF\n");//�¶�
		if(wifi_device->send(send_voltage,strlen(send_voltage)))//��������
		{
			printf("send a pictuer error\n");
		}
		pthread_mutex_unlock(&socket_mutex);
		usleep(5000000);
	}
	return 0;
	
}

void relay_contrl(int fd, int ctrl)
{
	char on_off[]={0,1};
	
	write(fd,&on_off[ctrl],1);//kai suo
}

int open_sound(char *argv);


static void *play_thread(void *arg)
{
	int choice = 0;
	int fd_relay;

	fd_relay = open("/dev/relay", O_RDWR);//relay for gate
	if (fd_relay < 0)
	{
		printf("can't open relay dev\n");
		return (void*)-1;
	}
	while(1)
	{
		pthread_mutex_lock(&alsa_mutex);
		pthread_cond_wait(&alsa_con_var, &alsa_mutex);	
		choice = choice_play;
		pthread_mutex_unlock(&alsa_mutex);

		switch(choice)
		{
			case 5:relay_contrl(fd_relay,1);open_sound("welcome.wav");relay_contrl(fd_relay,0);break;//����
			//case 2:open_sound("suo.wav");break;
			case 6: pthread_mutex_lock(&safe_mutex); safe = 1; pthread_mutex_unlock(&safe_mutex); open_sound("anquan.wav");break;
			case 7: pthread_mutex_lock(&safe_mutex); safe = 0; pthread_mutex_unlock(&safe_mutex); open_sound("jiechu.wav");break;

			case 8:open_sound("Co.wav");break;

			case 11:break;
			case 12:relay_contrl(fd_relay,0);break;//����
			
			default:relay_contrl(fd_relay,0);break;
		}

		//printf("start play \n");
	}
	return (void*)0;
}

static void *receive_thread(void *arg)
{
	char rcv_buf[24]={0};
	int len = 0;
	
	while(1)
	{
		memset(&rcv_buf, 0, sizeof(rcv_buf));
		pthread_mutex_lock(&socket_mutex);//������ƽ��
		len = wifi_device->receive(rcv_buf,sizeof(rcv_buf)/sizeof(char));
		pthread_mutex_unlock(&socket_mutex);
		if(len > 0)
		{
			printf("receive is %s \n",rcv_buf);
			if((rcv_buf[0]=='A') && (rcv_buf[1]=='A'))
			{
				if((rcv_buf[2]=='0') && (rcv_buf[3]=='5'))//����
				{
					//printf("the function is 05\n");
					pthread_mutex_lock(&alsa_mutex);
					choice_play = 5;
					pthread_cond_signal(&alsa_con_var);	
					pthread_mutex_unlock(&alsa_mutex);
				}
				else if((rcv_buf[2]=='0') && (rcv_buf[3]=='6'))//��ȫģʽ
				{
					pthread_mutex_lock(&alsa_mutex);
					choice_play = 6;
					pthread_cond_signal(&alsa_con_var);	
					pthread_mutex_unlock(&alsa_mutex);
				}
				else if((rcv_buf[2]=='0') && (rcv_buf[3]=='7'))//�����ȫģʽ
				{
					pthread_mutex_lock(&alsa_mutex);
					choice_play = 7;
					pthread_cond_signal(&alsa_con_var);	
					pthread_mutex_unlock(&alsa_mutex);
				}
				else if((rcv_buf[2]=='0') && (rcv_buf[3]=='8'))//˯�߼�⿪��
				{
					pthread_mutex_lock(&sleep_mutex);
					my_sleep = 1;
					pthread_mutex_unlock(&sleep_mutex);
				}
				else if((rcv_buf[2]=='0') && (rcv_buf[3]=='9'))//˯�߼����
				{
					pthread_mutex_lock(&sleep_mutex);
					my_sleep = 0;
					pthread_mutex_unlock(&sleep_mutex);
				}

				else if((rcv_buf[2]=='8') && (rcv_buf[3]=='8'))//co
				{
					pthread_mutex_lock(&alsa_mutex);
					choice_play = 8;
					pthread_cond_signal(&alsa_con_var);	
					pthread_mutex_unlock(&alsa_mutex);
				}
					
			}
			else
			{
				printf("receive data is error for AA start \n");
				continue;
			}
		}
	}
	return (void*)0;
}

static void *pictuer_send_thread(void *arg)
{
	int error;
	camera_device_opr camera_device;
	int pixel_format_camera;//in ���ظ�ʽ
	video_buf camera_buf;//ԭʼ����
	video_buf convert_buf;//ת������
	convert_opr* convert;

	FILE *fp;

	/* register camera devices */
	camera_init();
	error = camera_device_init("/dev/video0", &camera_device);
	if (error)
    {
        printf("VideoDeviceInit for /dev/video0 error!\n");
        return (void*)-1;
    }
	pixel_format_camera = camera_device.opr->get_fmat(&camera_device);

	/* ��������ͷ�豸 */
    error = camera_device.opr->start(&camera_device);
    if (error)
    {
        printf("StartDevice for camera_device error!\n");
        return (void*)-1;
    }
	printf("StartDevice for video0 on %c%c%c%c!\n", pixel_format_camera & 0xff,(pixel_format_camera >> 8) & 0xFF, (pixel_format_camera >> 16) & 0xFF, (pixel_format_camera >> 24) & 0xFF);

	memset(&camera_buf, 0, sizeof(camera_buf));
	memset(&convert_buf, 0, sizeof(convert_buf));

	convert_init();
	convert = get_convert_formats(pixel_format_camera, V4L2_PIX_FMT_JPEG);
    if (NULL == convert)
    {
        printf("can not support this format convert\n");
        return (void*)-1;
    }
	printf("the formats is:%s\n",convert->name);


	while(1)
	{
		pthread_mutex_lock(&send_mutex);
		pthread_cond_wait(&send_con_var, &send_mutex);	
		pthread_mutex_unlock(&send_mutex);
		
		/* ��������ͷ���� */
		error = camera_device.opr->get_frame(&camera_device, &camera_buf);
		if (error)
		{
    	    printf("GetFrame for camera_device error!\n");
    	    return (void*)-1;
    	}
		//printf("GetFrame for %s\n", argv[1]);

		/* ת��ΪJPG */
   	 	error = convert->convert(&camera_buf, &convert_buf);
    	printf("Convert %s, ret = %d\n", convert->name, error);


		fp = fopen("photo.jpg","w+");//ץȡͼƬ
		//fwrite(camera_buf.video_pixel_datas.pixel_datas,1,camera_buf.video_pixel_datas.width*camera_buf.video_pixel_datas.height,fp);//���ַ�������д�뵽�ļ�
		fwrite(convert_buf.video_pixel_datas.pixel_datas,1,camera_buf.video_pixel_datas.width*camera_buf.video_pixel_datas.height,fp);
		fclose (fp);

		pthread_mutex_lock(&socket_mutex);//������ƽ��
		if(wifi_device->send(convert_buf.video_pixel_datas.pixel_datas,camera_buf.video_pixel_datas.width*camera_buf.video_pixel_datas.height))//��������
		{
			printf("send a pictuer error\n");
		}
		pthread_mutex_unlock(&socket_mutex);//������ƽ��

		

		error = camera_device.opr->put_frame(&camera_device, &camera_buf);
        if (error)
        {
            printf("PutFrame for camera_device error!\n");
            return (void*)-1;
        } 

		usleep(1000000);//run once a second
	
	}
}

struct WAV_HEADER
{
    char rld[4]; //riff ��־����
    int rLen; 
    char wld[4]; //��ʽ���ͣ�wave��
    char fld[4]; //"fmt"

    int fLen; //sizeof(wave format matex)
    
    short wFormatTag; //�����ʽ
    short wChannels; //������
    int nSamplesPersec ; //����Ƶ��
    int nAvgBitsPerSample;//WAVE�ļ�������С
    short wBlockAlign; //�����
    short wBitsPerSample; //WAVE�ļ�������С
    
    char dld[4]; //��data��
    int wSampleLength; //��Ƶ���ݵĴ�С

} wav_header;


int set_pcm_play(FILE *fp);

int open_sound(char *argv)
{
    int nread;
    FILE *fp;
    fp=fopen(argv,"rb");
    if(fp==NULL)
    {
        perror("open file failed:\n");
        exit(1);
    }
    
    nread=fread(&wav_header,1,sizeof(wav_header),fp);
    /*printf("nread=%d\n",nread);
    printf("�ļ���СrLen��%d\n",wav_header.rLen);
    printf("��������%d\n",wav_header.wChannels);
    printf("����Ƶ�ʣ�%d\n",wav_header.nSamplesPersec);
    printf("������λ����%d\n",wav_header.wBitsPerSample);
    printf("wSampleLength=%d\n",wav_header.wSampleLength);  */
 
    set_pcm_play(fp);
	fclose(fp);
    return 0;
}

int set_pcm_play(FILE *fp)
{
	int rc;
	int ret;
	int size;
	snd_pcm_t* handle; //PCI�豸���
	snd_pcm_hw_params_t* params;//Ӳ����Ϣ��PCM������
	unsigned int val;
	int dir=0;
	snd_pcm_uframes_t frames;
	char *buffer;
	int channels=wav_header.wChannels;
	int frequency=wav_header.nSamplesPersec;
	int bit=wav_header.wBitsPerSample;
	int datablock=wav_header.wBlockAlign;
	//unsigned char ch[100]; //�����洢wav�ļ���ͷ��Ϣ
			
	rc=snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if(rc<0)
	{
		perror("\nopen PCM device failed:");
		exit(1);
	}
	
	snd_pcm_hw_params_alloca(&params); //����params�ṹ��
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_alloca:");
		exit(1);
	}
	rc=snd_pcm_hw_params_any(handle, params);//��ʼ��params
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_any:");
		exit(1);
	}
	rc=snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED); //��ʼ������Ȩ��
	if(rc<0)
	{
		perror("\nsed_pcm_hw_set_access:");
		exit(1);
	
	}
	
	//����λ��
	switch(bit/8)
	{
	case 1:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
			break ;
	case 2:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
			break ;
	case 3:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S24_LE);
			break ;
	}
	rc=snd_pcm_hw_params_set_channels(handle, params, channels); //��������,1��ʾ����>����2��ʾ������
	if(rc<0)
	{
			perror("\nsnd_pcm_hw_params_set_channels:");
			exit(1);
	}
	val = frequency;
	rc=snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir); //����>Ƶ��
	if(rc<0)
	{
			perror("\nsnd_pcm_hw_params_set_rate_near:");
			exit(1);
	}
	
	rc = snd_pcm_hw_params(handle, params);
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params: ");
		exit(1);
	}
	
	rc=snd_pcm_hw_params_get_period_size(params, &frames, &dir); /*��ȡ���ڳ���*/
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_get_period_size:");
		exit(1);
	}
	
	size = frames * datablock; /*4 �������ݿ쳤��*/
	
	buffer =(char*)malloc(size);
	fseek(fp,58,SEEK_SET); //��λ������������
	
	while (1)
	{
		memset(buffer,0,sizeof(buffer));
		ret = fread(buffer, 1, size, fp);
		if(ret == 0)
		{
			printf("����д�����\n");
			break;
		}
		 else if (ret != size)
		{
		 }
		// д��Ƶ���ݵ�PCM�豸 
		while((ret = snd_pcm_writei(handle, buffer, frames))<0)
		{
			usleep(2000); 
			if (ret == -EPIPE)
			{
			/* EPIPE means underrun */
				fprintf(stderr, "underrun occurred\n");
				//���Ӳ���������ã�ʹ�豸׼���� 
				snd_pcm_prepare(handle);
			}
			else if (ret < 0)
			{
				fprintf(stderr,"error from writei: %s\n",snd_strerror(ret));
			}
		}
	
	}
	
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
	return 0;
}
	



