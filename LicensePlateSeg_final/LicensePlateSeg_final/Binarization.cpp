//
//  Binarization.cpp
//  LicensePlateSegmentation
//
//  Created by 顾秀烨 on 11/28/15.
//  Copyright © 2015 laoreja. All rights reserved.
//

#include "Segmentation.hpp"

long getHistogram(int startY, int startX, int winHeight, int winWidth, vector<vector<unsigned char>>& inputGray, vector<int>& histogram){
    histogram.clear();
    histogram.reserve(256);
    histogram.assign(256, 0);
    long sum = 0;
    for (int i = startY; i < startY + winHeight; i++) {
        for (int j = startX; j < startX + winWidth; j++) {
            histogram[inputGray[i][j]]++;
            sum += inputGray[i][j];
        }
    }
    return sum;
}

void fillLocalThre(int i, int j, int winHeight, int winWidth, vector<vector<unsigned char>>& inputGray, vector<vector<int>>& thresholds){
    vector<int> histogram;
    
    int N = winWidth * winHeight;
    int NFgrd, NBgrd;
    long sumFgrd, sumBgrd, sumTotal;
    double AvgFgrd, AvgBgrd;
    
    int bestThre = 128;
    double maxBetweenVar = 0;
    
    sumTotal = getHistogram(i, j, winHeight, winWidth, inputGray, histogram);
    
    for (int tmpThre = 0; tmpThre < 255; tmpThre++) {
        NBgrd = 0;
        sumBgrd = 0;
        for (int grayLevel = 0; grayLevel <= tmpThre; grayLevel++) {
            NBgrd += histogram[grayLevel];
            sumBgrd += histogram[grayLevel] * grayLevel;
        }
        NFgrd = N - NBgrd;
        sumFgrd = sumTotal - sumBgrd;
        if (NBgrd && NFgrd) {
            AvgBgrd = sumBgrd * 1.0 / NBgrd;
            AvgFgrd = sumFgrd * 1.0 / NFgrd;
            
            double betweenVar = NFgrd * NBgrd * 1.0 / N / N * (AvgFgrd - AvgBgrd) * (AvgFgrd - AvgBgrd);
            if (betweenVar > maxBetweenVar) {
                bestThre = tmpThre;
                maxBetweenVar = betweenVar;
            }
        }
    }
    
    for (int y = i; y < i + winHeight; y++) {
        for (int x = j; x < j + winWidth; x++) {
            thresholds[y][x] = bestThre;
        }
    }
    
}

void Segmentation::OtsuLocalAdaptive(int winWidth, int winHeight, int step, vector<vector<unsigned char>>& inputGray, vector<vector<short>>& otsuBinary, bool bounded, bool saveBmp, string subpath){
    if (otsuBinary.size()) {
        for (int i = 0; i < otsuBinary.size(); i++) {
            otsuBinary[i].clear();
        }
        otsuBinary.clear();
    }
    
    int startY = 0, endY = bmpInfo.height;
    if (bounded) {
        startY += bound;
        endY -= bound;
    }
    
    vector<vector<int>> thresholds(bmpInfo.height, vector<int>(bmpInfo.width, 128));
    
    
    if (bounded) {
        for (int i = 0; i < bound; i++) {
            thresholds[i].assign(bmpInfo.width, 256);
            thresholds[bmpInfo.height - 1 - i].assign(bmpInfo.width, 256);
        }
    }
    
    for (int i = startY; i <= endY - winHeight; i += step) {
        for (int j = 0; j <= bmpInfo.width - winWidth; j += step) {
            //get threshold
            fillLocalThre(i, j, winHeight, winWidth, inputGray, thresholds);
        }
        fillLocalThre(i, bmpInfo.width - winWidth, winHeight, winWidth, inputGray, thresholds);
    }
    int i = endY - winHeight;
    for (int j = 0; j <= bmpInfo.width - winWidth; j += step) {
        //get threshold
        fillLocalThre(i, j, winHeight, winWidth, inputGray, thresholds);
    }
    fillLocalThre(i, bmpInfo.width - winWidth, winHeight, winWidth, inputGray, thresholds);
    
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) + ".bmp";
        
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        
        for (int row = 0; row < bmpInfo.height; row++) {
            vector<short> binaryLine;
            for (int col = 0; col < bmpInfo.width; col++) {
                unsigned char val;
                if (inputGray[row][col] <= thresholds[row][col]) {
                    val = 0;
                }else{
                    val = 255;
                }
                binaryLine.push_back(val);
                
                fwrite(&val, sizeof(unsigned char), 1, processedBmp);
                fwrite(&val, sizeof(unsigned char), 1, processedBmp);
                fwrite(&val, sizeof(unsigned char), 1, processedBmp);
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
            otsuBinary.push_back(binaryLine);
        }
        fclose(processedBmp);
    }else{
        for (int row = 0; row < bmpInfo.height; row++) {
            vector<short> binaryLine;
            for (int col = 0; col < bmpInfo.width; col++) {
                unsigned char val;
                if (inputGray[row][col] <= thresholds[row][col]) {
                    val = 0;
                }else{
                    val = 255;
                }
                binaryLine.push_back(val);
            }
            otsuBinary.push_back(binaryLine);
        }
    }
    
}

void Segmentation::OtsuGlobal(vector<vector<unsigned char>>& inputGray, vector<vector<short>>& otsuBinary, bool saveBmp, string subpath){
    if (otsuBinary.size()) {
        for (int i = 0; i < otsuBinary.size(); i++) {
            otsuBinary[i].clear();
        }
        otsuBinary.clear();
    }
    
    int bestThreshold = 128;
    double betweenVariance;
    double maxBetweenVariance = 0;
    double foregroundAvg, backgroundAvg;
    long foregroundSum, backgroundSum;
    
    int count, foregroundCount, backgroundCount;
    //    int bestForegroundCount = 0, bestBackgroundCount = 0;
    // they are used to detect if the characters is not white and need to reverse the image. Bad effect.
    
    maxBetweenVariance = 0;
    for (int tmpThreshold = 0; tmpThreshold < 255; tmpThreshold++) {
        count = 0;
        foregroundCount = backgroundCount = 0;
        foregroundSum = backgroundSum = 0;
        
        for (int i = 0; i < bmpInfo.height; i++) {
            for (int j = 0; j < bmpInfo.width; j++) {
                if (inputGray[i][j] <= tmpThreshold) {
                    backgroundCount++;
                    backgroundSum += inputGray[i][j];
                }else{
                    foregroundCount++;
                    foregroundSum += inputGray[i][j];
                }
                count++;
            }
        }
        
        if (backgroundCount != 0 && foregroundCount != 0) {
            backgroundAvg = backgroundSum * 1.0 / backgroundCount;
            foregroundAvg = foregroundSum * 1.0 / foregroundCount;
            betweenVariance = foregroundCount * backgroundCount * 1.0 / count / count * (foregroundAvg - backgroundAvg) * (foregroundAvg - backgroundAvg);
            if (betweenVariance > maxBetweenVariance) {
                bestThreshold = tmpThreshold;
                maxBetweenVariance = betweenVariance;
                //                bestForegroundCount = foregroundCount;
                //                bestBackgroundCount = backgroundCount;
            }
        }
    }
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) + ".bmp";
        
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        
        for (int row = 0; row < bmpInfo.height; row++) {
            vector<short> binaryLine;
            for (int col = 0; col < bmpInfo.width; col++) {
                unsigned char val;
                if (inputGray[row][col] <= bestThreshold) {
                    val = 0;
                }else{
                    val = 255;
                }
                binaryLine.push_back(val);
                
                fwrite(&val, sizeof(unsigned char), 1, processedBmp);
                fwrite(&val, sizeof(unsigned char), 1, processedBmp);
                fwrite(&val, sizeof(unsigned char), 1, processedBmp);
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
            otsuBinary.push_back(binaryLine);
        }
        fclose(processedBmp);
    }else{
        for (int row = 0;  row < bmpInfo.height; row++) {
            vector<short> binaryLine;
            for (int col = 0; col < bmpInfo.width; col++) {
                unsigned char val;
                if (inputGray[row][col] <= bestThreshold) {
                    val = 0;
                }else{
                    val = 255;
                }
                binaryLine.push_back(val);
            }
            otsuBinary.push_back(binaryLine);
        }
    }
}

void Segmentation::SauvolaLocalAdaptive(int windowWidth, double k, double R, vector<vector<unsigned char>>& inputGray, vector<vector<short>>& sauvolaBinary, bool saveBmp, string subpath){
    
    if (integralOnce.size()>0) {
        integralOnce.clear();
    }
    if (integralTwice.size()>0) {
        integralTwice.clear();
    }
    
    for (int row = 0; row < bmpInfo.height; row++) {
        vector<long> integralOnceLine;
        vector<long> integralTwiceLine;
        for (int col = 0; col < bmpInfo.width; col++) {
            unsigned char charY = inputGray[row][col];
            long i1, i2;
            
            if (row == 0 && col == 0) {
                i1 = charY;
                i2 = charY * charY;
            }else if(row == 0){
                i1 = integralOnceLine[col - 1] + charY;
                i2 = integralTwiceLine[col - 1] + charY * charY;
            }else if(col == 0){
                i1 = integralOnce[row-1][0] + charY;
                i2 = integralTwice[row-1][0] + charY * charY;
            }else{
                i1 = integralOnce[row-1][col] + integralOnceLine[col-1] - integralOnce[row-1][col-1] + charY;
                i2 = integralTwice[row-1][col] + integralTwiceLine[col-1] - integralTwice[row-1][col-1] + charY * charY;
            }
            integralOnceLine.push_back(i1);
            integralTwiceLine.push_back(i2);
        }
        integralOnce.push_back(integralOnceLine);
        integralTwice.push_back(integralTwiceLine);
    }
    
    for (int i = windowWidth/2; i < bmpInfo.height - windowWidth/2; i++){
        vector<double> thresholdsLine;
        for (int j = windowWidth/2; j < bmpInfo.height - windowWidth/2; j++) {
            double m = (integralOnce[i+windowWidth/2][j+windowWidth/2] + integralOnce[i-windowWidth/2][j-windowWidth/2] - integralOnce[i+windowWidth/2][j-windowWidth/2]-integralOnce[i-windowWidth/2][j+windowWidth/2]) / windowWidth / windowWidth;
            double s = (integralTwice[i+windowWidth/2][j+windowWidth/2] + integralTwice[i-windowWidth/2][j-windowWidth/2] - integralTwice[i+windowWidth/2][j-windowWidth/2]-integralTwice[i-windowWidth/2][j+windowWidth/2])/windowWidth/windowWidth - m*m;
            s = sqrt(s);
            
            double t = m * (1 + k * (s / R - 1));
            thresholdsLine.push_back(t);
        }
        sauvolaThresholds.push_back(thresholdsLine);
    }
    
    //    int foregroundCount = 0, backgroundCount = 0;
    for (int row = 0; row < bmpInfo.height; row++) {
        vector<short> binaryLine;
        int ii, jj;
        unsigned char val;
        
        for (int col = 0; col < bmpInfo.width; col++) {
            if (row < windowWidth / 2) {
                ii = 0;
            }else if(row >= bmpInfo.height - windowWidth / 2){
                ii = bmpInfo.height - windowWidth - 1;
            }else{
                ii = row - windowWidth/2;
            }
            
            if (col < windowWidth / 2) {
                jj = 0;
            }else if(col >= bmpInfo.width - windowWidth / 2){
                jj = bmpInfo.width - windowWidth - 1;
            }else{
                jj = col - windowWidth / 2;
            }
            
            if (inputGray[row][col] <= sauvolaThresholds[ii][jj]) {
                val = 0;
                //                backgroundCount++;
            }else{
                val = 255;
                //                foregroundCount++;
            }
            binaryLine.push_back(val);
        }
        sauvolaBinary.push_back(binaryLine);
    }
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4)+"Sauvola"+".bmp";
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        
        for (int row = 0; row < bmpInfo.height; row++) {
            for (int col = 0; col < bmpInfo.width; col++) {
                fwrite(&sauvolaBinary[row][col], sizeof(unsigned char), 1, processedBmp);
                fwrite(&sauvolaBinary[row][col], sizeof(unsigned char), 1, processedBmp);
                fwrite(&sauvolaBinary[row][col], sizeof(unsigned char), 1, processedBmp);
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
        }
        fclose(processedBmp);
    }
    
    
}
