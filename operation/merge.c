#include <merge.h>
#include <string.h>

/*
 * 功能描述： 把小图片合并入大图片里
 * 输入参数： x,y      - 小图片合并入大图片的某个区域, x/y确定这个区域的左上角座标
 *            small_picture - 内含小图片的象素数据
 *            big_picture   - 内含大图片的象素数据
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
 * 功能描述： 把新图片的某部分, 合并入老图片的指定区域
 * 输入参数： iStartXofNewPic, iStartYofNewPic : 从新图片的(iStartXofNewPic, iStartYofNewPic)座标处开始读出数据用于合并
 *            iStartXofOldPic, iStartYofOldPic : 合并到老图片的(iStartXofOldPic, iStartYofOldPic)座标去
 *            iWidth, iHeight                  : 合并区域的大小
 *            ptNewPic                         : 新图片
 *            ptOldPic                         : 老图片
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

