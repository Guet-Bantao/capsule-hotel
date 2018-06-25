
#ifndef _MERGE_H
#define _MERGE_H

#include <lcd_manager.h>

/*
 * 功能描述： 近邻取样插值方法缩放图片
 *            注意该函数会分配内存来存放缩放后的图片,用完后要用free函数释放掉
 *            "近邻取样插值"的原理请参考网友"lantianyu520"所著的"图像缩放算法"
 * 输入参数： ptOriginPic - 内含原始图片的象素数据
 *            ptBigPic    - 内含缩放后的图片的象素数据
 */
//int PicZoom(PT_PixelDatas ptOriginPic, PT_PixelDatas ptZoomPic);

/*
 * 功能描述： 把小图片合并入大图片里
 * 输入参数： x,y      - 小图片合并入大图片的某个区域, x/y确定这个区域的左上角座标
 *            small_picture - 内含小图片的象素数据
 *            big_picture   - 内含大图片的象素数据
 */
int picture_merge(int x, int y, pixel_datas* small_picture, pixel_datas* big_picture);



#endif /* _MERGE_H */

