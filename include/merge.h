
#ifndef _MERGE_H
#define _MERGE_H

#include <lcd_manager.h>

/*
 * ���������� ����ȡ����ֵ��������ͼƬ
 *            ע��ú���������ڴ���������ź��ͼƬ,�����Ҫ��free�����ͷŵ�
 *            "����ȡ����ֵ"��ԭ����ο�����"lantianyu520"������"ͼ�������㷨"
 * ��������� ptOriginPic - �ں�ԭʼͼƬ����������
 *            ptBigPic    - �ں����ź��ͼƬ����������
 */
//int PicZoom(PT_PixelDatas ptOriginPic, PT_PixelDatas ptZoomPic);

/*
 * ���������� ��СͼƬ�ϲ����ͼƬ��
 * ��������� x,y      - СͼƬ�ϲ����ͼƬ��ĳ������, x/yȷ�������������Ͻ�����
 *            small_picture - �ں�СͼƬ����������
 *            big_picture   - �ں���ͼƬ����������
 */
int picture_merge(int x, int y, pixel_datas* small_picture, pixel_datas* big_picture);



#endif /* _MERGE_H */

