#include <merge.h>
#include <string.h>

/*
 * ���������� ��СͼƬ�ϲ����ͼƬ��
 * ��������� x,y      - СͼƬ�ϲ����ͼƬ��ĳ������, x/yȷ�������������Ͻ�����
 *            small_picture - �ں�СͼƬ����������
 *            big_picture   - �ں���ͼƬ����������
 */
int picture_merge(int x, int y, pixel_datas* small_picture, pixel_datas* big_picture)
{
	int i;
	unsigned char *pucSrc;
	unsigned char *pucDst;
	
	if ((small_picture->width > big_picture->width)  ||
		(small_picture->height > big_picture->height) ||
		(small_picture->bpp != big_picture->bpp))
	{
		return -1;
	}

	pucSrc = small_picture->pixel_datas;
	pucDst = big_picture->pixel_datas + y * big_picture->line_bytes + x * big_picture->bpp / 8;
	for (i = 0; i < small_picture->height; i++)
	{
		memcpy(pucDst, pucSrc, small_picture->line_bytes);
		pucSrc += small_picture->line_bytes;
		pucDst += big_picture->line_bytes;
	}
	return 0;
}



/*
 * ���������� ����ͼƬ��ĳ����, �ϲ�����ͼƬ��ָ������
 * ��������� iStartXofNewPic, iStartYofNewPic : ����ͼƬ��(iStartXofNewPic, iStartYofNewPic)���괦��ʼ�����������ںϲ�
 *            iStartXofOldPic, iStartYofOldPic : �ϲ�����ͼƬ��(iStartXofOldPic, iStartYofOldPic)����ȥ
 *            iWidth, iHeight                  : �ϲ�����Ĵ�С
 *            ptNewPic                         : ��ͼƬ
 *            ptOldPic                         : ��ͼƬ
 */
int picture_merge_region(int iStartXofNewPic, int iStartYofNewPic, int iStartXofOldPic, int iStartYofOldPic, int iWidth, int iHeight, pixel_datas* ptNewPic, pixel_datas* ptOldPic)
{
	int i;
	unsigned char *pucSrc;
	unsigned char *pucDst;
    int iLineBytesCpy = iWidth * ptNewPic->bpp / 8;

    if ((iStartXofNewPic < 0 || iStartXofNewPic >= ptNewPic->width) || \
        (iStartYofNewPic < 0 || iStartYofNewPic >= ptNewPic->height) || \
        (iStartXofOldPic < 0 || iStartXofOldPic >= ptOldPic->width) || \
        (iStartYofOldPic < 0 || iStartYofOldPic >= ptOldPic->height))
    {
        return -1;
    }
	
	pucSrc = ptNewPic->pixel_datas + iStartYofNewPic * ptNewPic->line_bytes + iStartXofNewPic * ptNewPic->bpp / 8;
	pucDst = ptOldPic->pixel_datas + iStartYofOldPic * ptOldPic->line_bytes + iStartXofOldPic * ptOldPic->bpp / 8;
	for (i = 0; i < iHeight; i++)
	{
		memcpy(pucDst, pucSrc, iLineBytesCpy);
		pucSrc += ptNewPic->line_bytes;
		pucDst += ptOldPic->line_bytes;
	}
	return 0;
}

