#include <camera_manager.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <list_core.h>
#include <string.h>


static struct list_head * head;

/*	头结点的存在使得空链表与非空链表处理一致，
*	也方便对链表的开始结点的插入或删除操作
*/
static int Create_list(void)
{
	//list_info * head = (list_info *)malloc( sizeof(list_info) );  
	 head = (struct list_head *)malloc( sizeof(struct list_head) );
	if(head == NULL)  
		return -1;
	else  
	//	INIT_LIST_HEAD(&head->list_node);
		INIT_LIST_HEAD(head);
	return 0;
}

int register_camera_opr(camera_opr* opr)
{
	list_add(&opr->node,head);

	return 0;
}

void list_for_each_camera(void)
{
	camera_opr * pos;
	list_for_each_entry(pos, head, node)
		printf("list_info_name:%s\n",pos->name);
}

/*
*success return 0; faild return -1
*/
int camera_device_init(char * name, camera_device_opr * camera)
{
	int error;
	camera_opr * pos;
	
	list_for_each_entry(pos, head, node)
	{
		if((pos->init)!=NULL)//if have init
		{
			error=pos->init(name, camera);
			if(!error)
			{
				camera->opr=pos;//success find operation
				return 0;
			}
		}
	}
	return -1;
}




int camera_init(void)
{
	Create_list();
	v4l2_register();
	


	return 0;
}















