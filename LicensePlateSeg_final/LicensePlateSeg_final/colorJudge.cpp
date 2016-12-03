//
//  colorJudge.cpp
//  LicensePlateSegmentation
//
//  Created by 顾秀烨 on 11/28/15.
//  Copyright © 2015 laoreja. All rights reserved.
//

#include "Segmentation.hpp"


ReverseType judgeByWhiteCount(int startX, int endX, int startY, int endY, vector<vector<short>>& inputBinary){
    int whiteCount = 0;
    for (int i = startY; i < endY; i++) {
        for (int j = startX; j < endX; j++) {
            if (inputBinary[i][j]) {
                whiteCount++;
            }
        }
    }
    
    cout << "whiteCount: " <<  whiteCount << endl;
    
    double whiteRatio = (whiteCount*1.0 / (endX - startX) / (endY - startY));
    if (whiteRatio > 0.53) {
        return YES;
    }else if(whiteRatio < 0.47){
        return NO;
    }else{
        return UNKNOWN;
    }
}

void Segmentation::doReverse(vector<vector<short>>& otsuBinary, bool sort){
    if (sumG + sumB > 2.5 * sumR || sumB > sumR * 1.18) {
        needReverse = false;
    }else if (sumR + sumG > 2.5 * sumB ){
        needReverse =true;
    }else{
        ReverseType reverseType = judgeByWhiteCount(0, 120, 5, 75, otsuBinary);
        
        if(reverseType == UNKNOWN){
            namedWindow("color cannot defined", WINDOW_AUTOSIZE);
            Mat src = imread(openPath+fileName);
            imshow("color cannot defined", src);
            waitKey(100);
            cout << "do you want to reverse it? if yes, enter 'y'" << endl;
            string ans;
            cin >> ans;
            if (ans == "y") {
                needReverse = true;
            }else{
                needReverse = false;
            }
        }else{
            needReverse = (reverseType == YES);
        }
    }
    
    if(needReverse){
        for (int i = bound; i < bmpInfo.height - bound; i++) {
            for (int j = 0; j < bmpInfo.width; j++) {
                otsuBinary[i][j] = 255 - otsuBinary[i][j];
            }
        }
    }

    
    if (sort) {
        if (needReverse) {
            system(("cp " + openPath + fileName + " " + globalPath + "needReverse/" + fileName).c_str());
        }else{
            system(("cp " + openPath + fileName + " " + globalPath + "needNotReverse/" + fileName).c_str());
        }
    }
}