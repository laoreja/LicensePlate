//
//  Segmentation.hpp
//  LicensePlateSegmentation
//
//  Created by 顾秀烨 on 11/18/15.
//  Copyright © 2015 laoreja. All rights reserved.
//

#ifndef Segmentation_hpp
#define Segmentation_hpp

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <iostream>

#include "rotateCorrect.hpp"
using namespace cv;
using namespace std;


//declaration for reading bmp
typedef uint16_t WORD;
typedef uint32_t DWORD;

typedef struct tagBITMAPFILEHEADER {
    WORD bfType; //'BM'
    DWORD bfSize; //size of the file in bytes
    WORD bfReserved1; //0
    WORD bfReserved2; //0
    DWORD bfOffbits; //offset bitmap data;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    DWORD biWidth;
    DWORD biHeight;
    WORD biPlanes; //number of color planes, must be 1
    WORD biBitCount; //bits per pixel, 1, 4, 8 ,16, 24, 32
    DWORD biCompression;
    DWORD biSizeImage;
    DWORD biXpelsPerMeter;
    DWORD biYpelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;

//declaration for morphology

enum ReverseType{YES, NO, UNKNOWN};

ReverseType needReverse(int startX, int endX, int startY, int endY, vector<vector<short>>& inputBinary);


class Segmentation;

class grayDensityInfo{
private:
    vector<int>grayDensity;
    int threshold;
    //the min and max is the levels with density greater than threshold
    unsigned char maxGrayLevel = 0;
    unsigned char minGrayLevel = 255;
    
public:
    friend Segmentation;
};

class bmpInformation{
public://public for test only
    DWORD width;
    DWORD height;
    DWORD biOffset;
    WORD biBitCount;
    DWORD zeroPaddingLength;
    unsigned char* headBuffer;
public:
    friend Segmentation;
};

class Segmentation{
    
public:
    bmpInformation bmpInfo;//it is public for test only
    
private:
    unsigned char zeroChar = 0;
    
    string fileName;
    grayDensityInfo grayDensity;
    bool needReverse = false;
    
    vector<vector<double> >sauvolaThresholds;
    vector<vector<long> >integralOnce;
    vector<vector<long> >integralTwice;
    
    vector<int> verticalProj;
    
    int lowerBound = -1;
    int upperBound = 80;
    
    unsigned char generalGrayStretch(unsigned char X1, unsigned char X2, unsigned char Y1, unsigned char Y2, unsigned char x);
    unsigned char contrastStretch(unsigned char x);
    void split(int start, int end, vector<int>& segStarts, vector<int>& segEnds, int startThre);
    
    
    
public:
    int refineInputThre = -2;
    int maxRefineLoop = 50;
    
    int bound = 5;
    int boundThreDivFactor = 2;
    int boundLift = 3;
    
    double mergeFactor = 2.0/3;
    double divFactor = 1.5;
    int maxThreshold = 20;
    int minThreshold = 0;
    int refineStartThreshold = 0;
    
    int initSplitWidth = 23;
    int minSplitWidth = 15;
    
    int leftEdgeLimit = 3; //左边比较小是因为有川字的干扰，虽然右边也有可能有'1'的干扰。在不使用平均值的情况下可以选择保留边框，以免错误去除。
    int rightEdgeLimit = 5;
    
    
    int disPointBoundLift = 20; //不设得太大（卡在中间）是为了以免把汉字、数字的中间部分错认为小点。
    int disruptingPointLimit = 12;
    int pointLeft = 19;
    int pointRight = 45;
    
    int maxDistance_point = 23;
    int maxDistance = 22;
    int minDistance = 11;
    
    
    long sumR = 0, sumG = 0, sumB = 0;
    
    
    enum elementType{ZERO = 0, ONE = 1, DONTCARE = 2};
    elementType erosionStructure[9] = {ONE, ONE, ONE, ONE, ONE, ONE, ONE, ONE, ONE};
    elementType dilationStructure[9] = {ONE, ONE, ONE, ONE, ONE, ONE, ONE, ONE, ONE};
    
    bool saveWithColor = true;
    
    string openPath = "/Users/laoreja/study/MachineLearning/largerTry/bmp/";
    string globalPath = "/Users/laoreja/study/MachineLearning/largerTry/bmp/1/";
    
    Segmentation(){};
    Segmentation(const string& inputFileName):fileName(inputFileName){};
    
    void setFileName(const string& inputFileName){
        fileName = inputFileName;
    }
    
    void doReverse(vector<vector<short>>& otsuBinary, bool sort);
    
    void getGrayDensity(vector<vector<unsigned char>>& gray);
    
    void getVerticalProj(vector<vector<short>>& inputBinary);
    
    void transformToBasicGray(vector<vector<unsigned char>>& basicGray, bool saveBmp = false, string subpath = "");
    void transformToStretchedGray(vector<vector<unsigned char>>& sourceGray, vector<vector<unsigned char>>& stretchedGray, unsigned char X1 = 50, unsigned char X2 = 200, unsigned char Y1 = 30, unsigned char Y2 = 220, bool saveBmp = false, string subpath = "");
    void transformToContrastStretchedGray(vector<vector<unsigned char>>& sourceGray, vector<vector<unsigned char>>& contrastStretchedGray, bool saveBmp = false, string subpath = "");
    
    
    void OtsuGlobal(vector<vector<unsigned char>>& inputGray, vector<vector<short>>& otsuBinary, bool saveBmp = false, string subpath = "");
    //step, winWidth, winHeight need to be a factor of height and width
    void OtsuLocalAdaptive(int winWidth, int winHeight, int step, vector<vector<unsigned char>>& inputGray, vector<vector<short>>& otsuBinary,bool bounded, bool saveBmp = false, string subpath = "");
    
    
    void erosion(int width, int height, vector<vector<short>>& source, vector<vector<short>>& res, bool saveBmp = false, string subpath = "");
    void dilation(int width, int height, vector<vector<short>>& source, vector<vector<short>>& res);
    void opening(int width, int height, vector<vector<short>>& source, vector<vector<short>>& res, bool saveBmp = false, string subpath = "");
    void closing(int width, int height, vector<vector<short>>& source, vector<vector<short>>& res);
    
    void sobelMaskBinary(vector<vector<unsigned char>>& inputGray, vector<vector<unsigned char>>& res, bool saveBmp = false, string subpath = "");
    
    //when calling it in main, first set the refineStartThreshold to input threshold
    void VerticalProjection(int start, int end, int inputThre, vector<int>& segStarts, vector<int>& segEnds, const vector<vector<unsigned char>>& inputGray, bool saveBmp = false, string subpath = "");
    //[start, end)
    
    int verticalProjectionRefine(vector<int>& segStarts, vector<int>& segEnds, vector<vector<unsigned char>>& inputGray, vector<vector<short>>& inputBinary, bool saveBmp = false, string subpath = "");
    
    void SauvolaLocalAdaptive(int windowWidth, double k, double R, vector<vector<unsigned char>>& inputGray, vector<vector<short>>& sauvolaBinary, bool saveBmp = false, string subpath = "");
    
    
};

#endif /* Segmentation_hpp */
