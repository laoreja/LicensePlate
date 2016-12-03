//
// Created by 顾秀烨 on 11/26/15.
//

#ifndef PLATEDETECTBYCOLOR_COLORMATCH_H
#define PLATEDETECTBYCOLOR_COLORMATCH_H

#include "PlateDetect.h"

enum PlateColor{BLUE, YELLOW, RED, BLACK, WHITE, UNKNOWN};





//! 根据一幅图像与颜色模板获取对应的二值图
//! 输入RGB图像, 颜色模板（蓝色、黄色）
//! 输出灰度图（只有0和255两个值，255代表匹配，0代表不匹配）
Mat colorMatch(const Mat& src, Mat& match, const PlateColor r, const bool adaptive_minsv);
void closing(Mat &src, Mat &dst, PlateColor color);
void findPlate(Mat& morphInput, vector<RotatedRect>& resRects);


#endif //PLATEDETECTBYCOLOR_COLORMATCH_H
