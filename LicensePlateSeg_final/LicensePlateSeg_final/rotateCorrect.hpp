//
//  rotateCorrect.hpp
//  LicensePlateSeg_final
//
//  Created by 顾秀烨 on 11/28/15.
//  Copyright © 2015 laoreja. All rights reserved.
//

#ifndef rotateCorrect_hpp
#define rotateCorrect_hpp

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
using namespace cv;
using namespace std;


#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <string>


void setGrayOpenPath(string& path);
void setOriginOpenPath(string& path);
void setSavePath(string& path);

void rotateCorrect(string fileName);

#endif /* rotateCorrect_hpp */
