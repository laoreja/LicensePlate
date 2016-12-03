//
// Created by 顾秀烨 on 11/27/15.
//

#ifndef PLATEDETECTBYCOLOR_PLATEDETECT_H
#define PLATEDETECTBYCOLOR_PLATEDETECT_H

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <vector>

using namespace std;
using namespace cv;


bool sizeJudge(RotatedRect mr, int srcWidth);
int calcSequentialZeroPoint(Mat& src, int rowNum);


//void blueOnChange(int, void*);
//void yellowOnChange(int, void*);
#endif //PLATEDETECTBYCOLOR_PLATEDETECT_H
