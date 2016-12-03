//
//  Morphology.cpp
//  LicensePlateSegmentation
//
//  Created by 顾秀烨 on 11/28/15.
//  Copyright © 2015 laoreja. All rights reserved.
//

#include "Segmentation.hpp"

void Segmentation::erosion(int width, int height, vector<vector<short>>& source, vector<vector<short>>& res, bool saveBmp, string subpath){
    if (source.size() == 0) {
        printf("empty binary vector\n");
        return;
    }
    if (height < 3 || width < 3) {
        printf("height < 3 || width < 3, I refuse to work\n");
        return;
    }
    if (res.size() > 0) {
        res.clear();
    }
    
    for (int i = 0; i < height; i++) {
        vector<short> resLine;
        for (int j = 0; j < width; j++) {
            short tempPoint = (source[i][j] && erosionStructure[4]) || (erosionStructure[4] == 0);
            if (i != 0) {
                tempPoint &= (source[i-1][j] && erosionStructure[1]) || (erosionStructure[1] == 0);
                if (j != 0) {
                    tempPoint &= (source[i-1][j-1] && erosionStructure[0]) || (erosionStructure[0] == 0);
                }
                if (j != width - 1) {
                    tempPoint &= (source[i-1][j+1] && erosionStructure[2])|| (erosionStructure[2] == 0);
                }
            }
            if (i != height - 1) {
                tempPoint &= (source[i+1][j] && erosionStructure[7])|| (erosionStructure[4] == 0);
                if (j != 0) {
                    tempPoint &= (source[i+1][j-1] && erosionStructure[6])|| (erosionStructure[6] == 0);
                }
                if (j != width - 1) {
                    tempPoint &= (source[i+1][j+1] && erosionStructure[8])|| (erosionStructure[8] == 0);
                }
            }
            if (j != 0) {
                tempPoint &= (source[i][j-1] && erosionStructure[3])|| (erosionStructure[3] == 0);
            }
            if (j != width - 1) {
                tempPoint &= (source[i][j+1] && erosionStructure[5])|| (erosionStructure[5] == 0);
            }
            
            if (tempPoint) {
                resLine.push_back(255);
            }else{
                resLine.push_back(0);
            }
        }
        res.push_back(resLine);
    }
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) +".bmp";
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        for (int row = 0; row < bmpInfo.height; row++) {
            
            for (int col = 0; col < bmpInfo.width; col++) {
                
                fwrite(&res[row][col], sizeof(unsigned char), 1, processedBmp);
                fwrite(&res[row][col], sizeof(unsigned char), 1, processedBmp);
                fwrite(&res[row][col], sizeof(unsigned char), 1, processedBmp);
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
            
        }
        fclose(processedBmp);
    }
}

void Segmentation::dilation(int width, int height, vector<vector<short>>& source, vector<vector<short>>& res){
    if (source.size() == 0) {
        printf("empty binary vector\n");
        return;
    }
    if (height < 3 || width < 3) {
        printf("height < 3 || width < 3, I refuse to work\n");
        return;
    }
    if(res.size() > 0){
        res.clear();
    }
    
    for (int i = 0; i < height; i++) {
        vector<short> resLine;
        for (int j = 0; j < width; j++) {
            short tempPoint = source[i][j] && dilationStructure[4];
            if (i != 0) {
                tempPoint |= (source[i-1][j] && dilationStructure[1]);
                if (j != 0) {
                    tempPoint |= (source[i-1][j-1] && dilationStructure[0]);
                }
                if (j != width - 1) {
                    tempPoint |= (source[i-1][j+1] && dilationStructure[2]);
                }
            }
            if (i != height - 1) {
                tempPoint |= (source[i+1][j] && dilationStructure[7]);
                if (j != 0) {
                    tempPoint |= (source[i+1][j-1] && dilationStructure[6]);
                }
                if (j != width - 1) {
                    tempPoint |= (source[i+1][j+1] && dilationStructure[8]);
                }
            }
            if (j != 0) {
                tempPoint |= (source[i][j-1] && dilationStructure[3]);
            }
            if (j != width - 1) {
                tempPoint |= (source[i][j+1] && dilationStructure[5]);
            }
            
            if (tempPoint) {
                resLine.push_back(255);
            }else{
                resLine.push_back(0);
            }
        }
        res.push_back(resLine);
    }
}

void Segmentation::opening(int width, int height, vector<vector<short>>& source, vector<vector<short>>& res, bool saveBmp, string subpath){
    if(res.size() > 0){
        res.clear();
    }
    vector<vector<short>> tmp;
    erosion(width, height, source, tmp, false);
    dilation(width, height, tmp, res);
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) +".bmp";
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        for (int row = 0; row < bmpInfo.height; row++) {
            
            for (int col = 0; col < bmpInfo.width; col++) {
                
                fwrite(&res[row][col], sizeof(unsigned char), 1, processedBmp);
                fwrite(&res[row][col], sizeof(unsigned char), 1, processedBmp);
                fwrite(&res[row][col], sizeof(unsigned char), 1, processedBmp);
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
            
        }
        fclose(processedBmp);
    }
}

void Segmentation::closing(int width, int height, vector<vector<short>>& source, vector<vector<short>>& res){
    if(res.size() > 0){
        res.clear();
    }
    
    vector<vector<short>> tmp;
    
    dilation(width, height, source, tmp);
    erosion(width, height, tmp, res);
}
