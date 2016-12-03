//
//  Preprocess.cpp
//  LicensePlateSegmentation
//
//  Created by 顾秀烨 on 11/28/15.
//  Copyright © 2015 laoreja. All rights reserved.
//

#include "Segmentation.hpp"

void Segmentation::transformToBasicGray(vector<vector<unsigned char>>& basicGray, bool saveBmp, string subpath){
    basicGray.clear();
    
    FILE* bmpFile;
    
    
    string bmpPath = openPath + fileName;
    if((bmpFile = fopen(bmpPath.c_str(), "r")) != NULL){
        fseek(bmpFile, 0x0a, SEEK_SET);
        fread(&(bmpInfo.biOffset), sizeof(DWORD), 1, bmpFile);
        fseek(bmpFile, 0x12, SEEK_SET);
        fread(&(bmpInfo.width),sizeof(DWORD),1,bmpFile);
        fread(&(bmpInfo.height), sizeof(DWORD), 1, bmpFile);
        fseek(bmpFile, 0x1c, SEEK_SET);
        fread(&(bmpInfo.biBitCount), sizeof(WORD), 1, bmpFile);
        if(bmpInfo.biBitCount != 24){
            printf("Not in the RGB form, cannot change to YUV.\n");//add palette RGB later
            exit(-1);
        }
        bmpInfo.zeroPaddingLength = (bmpInfo.width * 3 + 3)/4*4 - bmpInfo.width*3;
        
        
        if((bmpInfo.headBuffer = (unsigned char *)malloc(sizeof(unsigned char) * bmpInfo.biOffset))==NULL){
            printf("cannot malloc enough space!\n");
            exit(-3);
        }
        
        fseek(bmpFile, 0, SEEK_SET);
        fread(bmpInfo.headBuffer, bmpInfo.biOffset, 1, bmpFile);
        
        sumB = sumR = sumG = 0;
        for (int row = 0; row < bmpInfo.height; row++) {
            vector<unsigned char> grayLine;
            for (int col = 0; col < bmpInfo.width; col++) {
                
                unsigned char R, G, B;
                fread(&B, sizeof(B), 1, bmpFile);
                fread(&G, sizeof(G), 1, bmpFile);
                fread(&R, sizeof(R), 1, bmpFile);
                sumR += R;
                sumG += G;
                sumB += B;
                
                double Y = 0.299 * R + 0.587 * G + 0.114 * R;
                unsigned char charY = (unsigned char) Y;
                grayLine.push_back(charY);
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fread(&zeroChar, sizeof(unsigned char), 1, bmpFile);
            }
            basicGray.push_back(grayLine);
        }
        fclose(bmpFile);
        
        
        cout << fileName << ": " << endl;
        cout << "sumR: " << sumR << " sumG: " << sumG << " sumB: " << sumB << endl << endl;
        
        //        if (1.25 * sumB < sumR && 1.25 * sumB < sumG) {
        //            system(("cp " + openPath + fileName + " " + globalPath + "/yellow/" + fileName).c_str());
        //            needReverse = true; //yellow license
        //            cout << "yellow" << endl << endl;
        //
        //        }else if((sumB > sumR || sumG > sumR) && sumB + sumG > 2 * sumR){
        //            system(("cp " + openPath + fileName + " " + globalPath + "/blue/" + fileName).c_str());
        //            needReverse = false; //blue
        //            cout << "blue" << endl << endl;
        //
        //        }else{
        //            system(("cp " + openPath + fileName + " " + globalPath + "/colorNotDefined/" + fileName).c_str());
        //
        //            cout << fileName << " need reverse? " << endl;
        //            string ans;
        //            cin >> ans;
        //            needReverse = false;
        //            if (ans == "y") {
        //                needReverse = true;
        //            }
        //        }
        
        //for test only
        
        //
        //        if (needReverse) {
        //            for (int i = 0; i < bmpInfo.height; i++) {
        //                for (int j = 0; j < bmpInfo.width; j++) {
        //                    basicGray[i][j] = 255 - basicGray[i][j];
        //                }
        //            }
        //        }
        
        if (saveBmp) {
            FILE* bmpGray;
            string grayPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) + ".bmp";
            if((bmpGray = fopen(grayPath.c_str(), "w")) == NULL ){
                printf("cannot create file.\n");
                exit(-2);
            }
            fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, bmpGray);
            
            for (int row = 0; row < bmpInfo.height; row++) {
                for (int col = 0; col < bmpInfo.width; col++) {
                    fwrite(&basicGray[row][col], sizeof(unsigned char), 1, bmpGray);
                    fwrite(&basicGray[row][col], sizeof(unsigned char), 1, bmpGray);
                    fwrite(&basicGray[row][col], sizeof(unsigned char), 1, bmpGray);
                }
                for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                    fwrite(&zeroChar, sizeof(unsigned char), 1, bmpGray);
                }
            }
            fclose(bmpGray);
        }
        
    }else{
        printf("cannot open bmp file!");
        exit(-4);
    }
}

void Segmentation::getGrayDensity(vector<vector<unsigned char>>& sourceGray){
    
    grayDensity.grayDensity.clear();
    grayDensity.maxGrayLevel = 0;
    grayDensity.minGrayLevel = 255;
    
    grayDensity.grayDensity.reserve(256);
    grayDensity.grayDensity.assign(256, 0);
    
    grayDensity.threshold = bmpInfo.width * bmpInfo.height * 1.0 / 256;
    
    for (int row = 0; row < bmpInfo.height; row++) {
        for (int col = 0;  col < bmpInfo.width; col++) {
            grayDensity.grayDensity[sourceGray[row][col]]++;
        }
    }
    
    for (int i = 0; i < 256; i++) {
        if (grayDensity.grayDensity[i] >= grayDensity.threshold) {
            if (i > grayDensity.maxGrayLevel) {
                grayDensity.maxGrayLevel = (unsigned char)i;
            }
            if (i < grayDensity.minGrayLevel) {
                grayDensity.minGrayLevel = (unsigned char)i;
            }
        }
    }
}


unsigned char Segmentation::contrastStretch(unsigned char x){
    if (x <= grayDensity.minGrayLevel) {
        return 0;
    }
    
    if (x >= grayDensity.maxGrayLevel) {
        return 255;
    }
    
    return (unsigned char)((x - grayDensity.minGrayLevel) * 1.0 / (grayDensity.maxGrayLevel - grayDensity.minGrayLevel) * 255);
}


void Segmentation:: transformToContrastStretchedGray(vector<vector<unsigned char>>& sourceGray, vector<vector<unsigned char>>& contrastStretchedGray, bool saveBmp, string subpath){
    //need to do getGrayDensity for the basic gray first.
    getGrayDensity(sourceGray);
    contrastStretchedGray.clear();
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) + "constrastStretchedGray" + ".bmp";
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        for (int row = 0; row < bmpInfo.height; row++) {
            vector<unsigned char>contrastGrayLine;
            for (int col = 0; col < bmpInfo.width; col++) {
                unsigned char processedChar = contrastStretch(sourceGray[row][col]);
                contrastGrayLine.push_back(processedChar);
                fwrite(&processedChar, sizeof(unsigned char), 1, processedBmp);
                fwrite(&processedChar, sizeof(unsigned char), 1, processedBmp);
                fwrite(&processedChar, sizeof(unsigned char), 1, processedBmp);
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
            contrastStretchedGray.push_back(contrastGrayLine);
        }
        fclose(processedBmp);
    }else{
        for (int row = 0; row < bmpInfo.height; row++) {
            vector<unsigned char>contrastGrayLine;
            for (int col = 0; col < bmpInfo.width; col++) {
                unsigned char processedChar = contrastStretch(sourceGray[row][col]);
                contrastGrayLine.push_back(processedChar);
            }
            contrastStretchedGray.push_back(contrastGrayLine);
        }
    }
}

unsigned char Segmentation::generalGrayStretch(unsigned char X1, unsigned char X2, unsigned char Y1, unsigned char Y2, unsigned char Y){
    
    double fY;
    if (Y <= X1) {
        fY = Y1 * 1.0 / X1 * Y;
    }else if(Y <= X2){
        fY = (Y2 - Y1) * 1.0 / (X2 - X1) * (Y - X1) + Y1;
    }else{
        fY = (255 - Y2) * 1.0 / (255 - X2) * (Y - X2) + Y2;
    }
    return (unsigned char)fY;
}

void Segmentation::transformToStretchedGray(vector<vector<unsigned char>>& sourceGray, vector<vector<unsigned char>>& stretchedGray, unsigned char X1, unsigned char X2, unsigned char Y1, unsigned char Y2, bool saveBmp, string subpath){
    
    stretchedGray.clear();
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) + ".bmp";
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        for (int row = 0; row < bmpInfo.height; row++) {
            vector<unsigned char>stretchedGrayLine;
            for (int col = 0; col < bmpInfo.width; col++) {
                unsigned char processedChar = generalGrayStretch(X1, X2, Y1, Y2,sourceGray[row][col]);
                stretchedGrayLine.push_back(processedChar);
                fwrite(&processedChar, sizeof(unsigned char), 1, processedBmp);
                fwrite(&processedChar, sizeof(unsigned char), 1, processedBmp);
                fwrite(&processedChar, sizeof(unsigned char), 1, processedBmp);
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
            stretchedGray.push_back(stretchedGrayLine);
        }
        fclose(processedBmp);
    }
    
    
}


