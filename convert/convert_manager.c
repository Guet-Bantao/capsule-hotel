#include <string.h>
#include <convert_manager.h>

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

int register_convert_opr(convert_opr* opr)
{
	list_add(&opr->node,head);

	return 0;
}

convert_opr* get_convert_formats(int pixel_format_in, int Pixel_format_out)
{
	int ret;
	convert_opr * pos;
	
	list_for_each_entry(pos, head, node)
	{
		if((pos->is_support)!=NULL)//if have is_support
		{
			ret=pos->is_support(pixel_format_in, Pixel_format_out);
			if(ret)
			{
				return pos;//success find operation
			}
		}
	}
	return NULL;
}



int convert_init(void)
{
	Create_list();
	mjpeg_register();
	yuyv_register();
	yuyv_new_register();
	return 0;
}





