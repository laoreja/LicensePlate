//
//  main.cpp
//  LicensePlateSegmentation
//
//  Created by 顾秀烨 on 11/18/15.
//  Copyright © 2015 laoreja. All rights reserved.
//

#include <iostream>
#include "Segmentation.hpp"

int main(int argc, const char * argv[]) {
    for (int i = 1; i < 988; i++) {
        string filename = to_string(i) + ".bmp";
        Segmentation seg(filename);
        
        seg.openPath = "/Users/laoreja/Downloads/work2/车牌/bmp/pics/";
        seg.globalPath = "/Users/laoreja/study/MachineLearning/result/plateSegment/";
        
        vector<vector<unsigned char>> basicGray;
        seg.transformToBasicGray(basicGray);

        vector<vector<unsigned char>> contrastGray;
        seg.transformToContrastStretchedGray(basicGray, contrastGray, true, "contrastGray/");
        
        //do minor rotate
        string contrastGrayPath = seg.globalPath + "contrastGray/";
        setGrayOpenPath(contrastGrayPath);
        setOriginOpenPath(seg.openPath);
        setSavePath(seg.globalPath);
        rotateCorrect(filename);
        
        seg.openPath = seg.globalPath + "originalAfterRotate/";
        seg.transformToBasicGray(basicGray);
        seg.transformToContrastStretchedGray(basicGray, contrastGray, true, "contrastGray/");
        
        vector<vector<unsigned char>> boundedRes;
        seg.sobelMaskBinary(contrastGray, boundedRes);

        vector<vector<short>> otsuBinary;
//        seg.OtsuGlobal(boundedRes, otsuBinary, true, "globalOtsu/");
        seg.OtsuLocalAdaptive(50, 50, 15, boundedRes, otsuBinary, true, true, "localOtsu/");
        
        seg.doReverse(otsuBinary, true);
        
        vector<int> segStarts;
        vector<int> segEnds;
        seg.getVerticalProj(otsuBinary);

        seg.refineStartThreshold = 3;
        seg.VerticalProjection(0, seg.bmpInfo.width, 3, segStarts, segEnds, basicGray, true, "initialSeg/");
        seg.verticalProjectionRefine(segStarts, segEnds, basicGray, otsuBinary, true, "res/");
        
    }
    
    
    //below are for the 100 picture, after slant correct, see the file path.
//    for (int i = 1; i < 101; i++) {
//        string filename = to_string(i) + ".bmp";
//        Segmentation seg(filename);
//        
//        seg.openPath = "/Users/laoreja/study/MachineLearning/largerTry/bmp/1/globalOtsu/gray/binaryAfterRotate/";
//        seg.globalPath = "/Users/laoreja/study/MachineLearning/largerTry/bmp/1/globalOtsu/gray/";
//        
//        vector<vector<unsigned char>> basicGray1;
//        seg.transformToBasicGray(basicGray1);
//        
//        vector<vector<unsigned char>> boundedRes;
//        seg.sobelMaskBinary(basicGray1, boundedRes);
//        
//        vector<vector<short>> otsuBinary;
//        seg.OtsuGlobal(boundedRes, otsuBinary);
////        seg.OtsuLocalAdaptive(20, 20, 5, boundedRes, otsuBinary, "localOtsu/s");
//        
//        vector<int> segStarts;
//        vector<int> segEnds;
//        seg.getVerticalProj(otsuBinary);
//        
//        seg.refineStartThreshold = 3;
//        seg.VerticalProjection(0, seg.bmpInfo.width, 3, segStarts, segEnds, basicGray1, "initial", true);
//        seg.verticalProjectionRefine(segStarts, segEnds, basicGray1, otsuBinary, true);
//    }
    return 0;
}
