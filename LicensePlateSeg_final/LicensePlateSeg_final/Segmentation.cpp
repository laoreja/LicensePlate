//
//  Segmentation.cpp
//  LicensePlateSegmentation
//
//  Created by 顾秀烨 on 11/18/15.
//  Copyright © 2015 laoreja. All rights reserved.
//

#include "Segmentation.hpp"

void Segmentation::getVerticalProj(vector<vector<short>>& inputBinary){
    verticalProj.clear();
    for (int j = 0; j < bmpInfo.width; j++) {
        int lineSum = 0;
        for (int i = 0; i < bmpInfo.height; i++) {
            lineSum += inputBinary[i][j] / 255;
        }
        verticalProj.push_back(lineSum);
    }
}

void Segmentation::split(int start, int end, vector<int>& segStarts, vector<int>& segEnds, int startThre){
    segStarts.clear();
    segEnds.clear();
    
    int splitLoopLimit = 40 - startThre;
    int splitTimes = 0;
    
    while (segStarts.size() < 2 && splitTimes < splitLoopLimit) {
        segStarts.clear();
        segEnds.clear();
        segStarts.push_back(start);
        for (int i = start + 2; i < end - 2; i++) {
            
            if ( verticalProj[i] <= startThre ) {
                segEnds.push_back(i);
                segStarts.push_back(i+2);
                break;
            }
            //            if (verticalProj[i] > startThre  ) {
            //                segStarts.push_back(i);
            //            }
            //
        }
        
        
        startThre+=2;
        splitTimes++;
    }
    segEnds.push_back(end);
    
    
    if (segStarts.size() != segEnds.size()) {
        cout << "in split, segStarts.size() != segEnds.size()" << endl;
    }
}

int Segmentation::verticalProjectionRefine(vector<int>& segStarts, vector<int>& segEnds, vector<vector<unsigned char>>& inputGray, vector<vector<short>>& inputBinary, bool saveBmp, string subpath){
    
    //remove vertical edges
    if ( segStarts[0] < 3 && segEnds[0] - segStarts[0] < leftEdgeLimit) {
        cout << "remove left edge" << segStarts[0] << " " << segEnds[0] << endl;
        segStarts.erase(segStarts.begin());
        segEnds.erase(segEnds.begin());
    }
    
    if ( segStarts[segStarts.size()-1] >= 115 && segEnds[segEnds.size()-1] - segStarts[segStarts.size()-1] < rightEdgeLimit) {
        cout << "remove right edge" << segStarts[segStarts.size()-1] << " " << segEnds[segEnds.size()-1] << endl;
        segStarts.erase(segStarts.end() - 1);
        segEnds.erase(segEnds.end() - 1);
    }
    
    //try remove the disrupting point, if using a larger threshold,this step may not needed.
    for (int i = 0; i < segStarts.size(); i++) {
        if ((segEnds[i] - segStarts[i] <= 5) && (segStarts[i] > 19) && (segEnds[i] < 49)) {
            bool remove = true;
            for (int col = segStarts[i]; col < segEnds[i]; col++) {
                int tmpSum = 0;
                int entries = 0;
                bool inside = false;
                for (int row = disPointBoundLift; row <= bmpInfo.height - disPointBoundLift; row++) {
                    if (inputBinary[row][col]) {
                        tmpSum++;
                        if (!inside) {
                            entries++;
                            inside = true;
                        }
                    }else{
                        if (inside) {
                            inside = false;
                        }
                    }
                }
                if (tmpSum >= disruptingPointLimit || entries > 2) {
                    remove = false;
                }
            }
            if (remove) {
                cout << "remove " << segStarts[i] << " " << segEnds[i] << endl;
                segStarts.erase(segStarts.begin() + i);
                segEnds.erase(segEnds.begin() + i);
                break;
            }
        }
    }
    //end of remove the disrupting point
    
    vector<int> innerStarts;
    vector<int> innerEnds;
    for (int i = 0; i < segStarts.size(); i++) {
        if (segEnds[i] - segStarts[i] > initSplitWidth) {
            split(segStarts[i], segEnds[i], innerStarts, innerEnds, refineStartThreshold);
            if (innerStarts.size() > 1) {
                segStarts.insert(segStarts.begin() + i + 1, innerStarts.begin() + 1, innerStarts.end());
                segEnds.insert(segEnds.begin() + i, innerEnds.begin(), innerEnds.end() - 1 );
                i = i - 1;
            }
        }
    }
    
    
    bool endRefine = false;
    
    vector<int> midPos;
    vector<int> distances;
    int disSum, disAvg;
    
    int loopTimes = 0;
    
    while (!endRefine && loopTimes < maxRefineLoop) {
        
        loopTimes++;
        midPos.clear();
        distances.clear();
        
        for (int i = 0; i < segStarts.size(); i++) {
            midPos.push_back((segEnds[i] + segStarts[i])/2);
        }
        
        disSum = 0;
        for (int i = 0; i < midPos.size() - 1; i++) {
            int distance = midPos[i+1] - midPos[i];
            distances.push_back(distance);
            disSum += distance;
        }
        if (distances.size() == 0) {
            cout << "distance count == 0" << endl;
            break;
        }
        disAvg = disSum / distances.size();
        cout << "disAvg = " << disAvg << endl;
        
        for (int i = 0; i < distances.size(); i++) {
            cout << "i =: " << i << " distance: " << distances[i] << endl;
            if (distances[i] < minDistance) {
                //            if (distances[i] < minDistance && distances[i] < disAvg * mergeFactor) {
                segEnds.erase(segEnds.begin() + i);
                segStarts.erase(segStarts.begin() + i + 1);
                goto NEXTLOOP;
            }
            
            int fineMaxDistance = maxDistance;
            if (segEnds[i] > pointLeft && segStarts[i+1] < pointRight) {
                fineMaxDistance = maxDistance_point;
            }
            
            if ( (distances[i] > fineMaxDistance /* || distances[i] > disAvg * divFactor */)){
                //            if ( (distances[i] > maxDistance || distances[i] > disAvg * divFactor) ) {
                
                int index = i;
                if ((segEnds[i+1] - segStarts[i+1]) > (segEnds[i] - segStarts[i])) {
                    index = i + 1;
                }
                
                if (segEnds[index] - segStarts[index] > minSplitWidth) {
                    split(segStarts[index], segEnds[index], innerStarts, innerEnds, refineStartThreshold);
                    
                    if (innerStarts.size() > 1) {
                        segStarts.insert(segStarts.begin() + index + 1, innerStarts.begin() + 1, innerStarts.end());
                        segEnds.insert(segEnds.begin() + index, innerEnds.begin(), innerEnds.end() - 1 );
                        
                        goto NEXTLOOP;
                    }
                }
                
                
            }
            
        }
        //        endRefine = true;
    NEXTLOOP:
        if (segStarts.size() > 5 && segStarts.size() < 9 ) {
            int i;
            for (i = 0; i < distances.size(); i++) {
                if (distances[i] < 10 || distances[i] > 22) {
                    break;
                }
            }
            if (i == distances.size()) {
                endRefine = true;
            }
        }
        
    }
    
    //    vector<int>::iterator it1 = segStarts.begin(), it2 = segEnds.begin();
    //
    //    while (it1 != segStarts.end() && it2 != segEnds.end()) {
    //        if (*it2 - *it1 <= 2) {
    //            it1 = segStarts.erase(it1);
    //            it2 = segEnds.erase(it2);
    //        }else{
    //            it1++;
    //            it2++;
    //        }
    //    }
    
    if (segStarts.size() && segStarts[0] > 0) {
        segStarts[0] -= 1;
    }
    for (int i = 1; i < segStarts.size(); i++) {
        if (segStarts[i] - segEnds[i - 1] > 1) {
            segStarts[i] -= 1;
        }
    }
    if (segEnds.size() && segEnds[segEnds.size() - 1] < bmpInfo.width - 1) {
        segEnds[segEnds.size() - 1] += 1;
    }
    for (int i = 0; i < segEnds.size(); i++) {
        if (segStarts[i+1] - segEnds[i] > 1) {
            segEnds[i] += 1;
        }
    }
    
    cout << "seg count: " << segStarts.size() << endl;
    
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) + "final" + ".bmp";
        
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        
        unsigned char fullColor = 255;
        set<int> startSet, endSet;
        for (int i = 0; i < segStarts.size(); i++) {
            startSet.insert(segStarts[i]);
            endSet.insert(segEnds[i]);
        }
        
        FILE* bmpFile = NULL;
        if (saveWithColor) {
            string bmpPath = openPath + fileName;
            if((bmpFile = fopen(bmpPath.c_str(), "r")) == NULL){
                cout << "open original bmp error, in refine function." << endl;
                return -1;
            }
            fseek(bmpFile, bmpInfo.biOffset, SEEK_SET);
        }
        
        for (int row = 0; row < bmpInfo.height; row++) {
            for (int col = 0; col < bmpInfo.width; col++) {
                unsigned char R, G, B;
                if (saveWithColor) {
                    fread(&B, sizeof(B), 1, bmpFile);
                    fread(&G, sizeof(G), 1, bmpFile);
                    fread(&R, sizeof(R), 1, bmpFile);
                }
                
                if (startSet.find(col) != startSet.end()) {
                    fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
                    fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
                    fwrite(&fullColor, sizeof(unsigned char), 1, processedBmp);
                }else if (endSet.find(col) != endSet.end()) {
                    fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
                    fwrite(&fullColor, sizeof(unsigned char), 1, processedBmp);
                    fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
                }else{
                    if (saveWithColor) {
                        fwrite(&B, sizeof(unsigned char), 1, processedBmp);
                        fwrite(&G, sizeof(unsigned char), 1, processedBmp);
                        fwrite(&R, sizeof(unsigned char), 1, processedBmp);
                    }else{
                        fwrite(&inputGray[row][col], sizeof(unsigned char), 1, processedBmp);
                        fwrite(&inputGray[row][col], sizeof(unsigned char), 1, processedBmp);
                        fwrite(&inputGray[row][col], sizeof(unsigned char), 1, processedBmp);
                    }
                }
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                unsigned char noUseChar;
                if (saveWithColor) {
                    fread(&noUseChar, sizeof(unsigned char), 1, bmpFile);
                }
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
        }
        fclose(processedBmp);
        if (saveWithColor) {
            fclose(bmpFile);
        }
    }
    
    return (int)segStarts.size();
}



void Segmentation::VerticalProjection(int start, int end, int inputThre, vector<int>& segStarts, vector<int>& segEnds, const vector<vector<unsigned char>>& inputGray, bool saveBmp, string subpath){
    
    segStarts.clear();
    segEnds.clear();
    
    if (inputThre < 0) {
        int tmpSum = 0;
        for (int j = start; j < end; j++) {
            tmpSum += verticalProj[j];
            //            cout << verticalProj[j] << " ";
        }
        inputThre = tmpSum * 1.0 / (end - start) / (-inputThre);
        if (inputThre > maxThreshold) {
            inputThre = maxThreshold;
        }
        if (inputThre < minThreshold) {
            inputThre = minThreshold;
        }
        //        cout << endl << "start: " << start << " end: " << end << " threshold: " << inputThre << endl;
    }
    
    set<int> posSet;
    bool inside = false;
    for (int j = start; j < end; j++) {
        if (verticalProj[j] > inputThre && !inside) {
            //come inside
            inside = true;
            segStarts.push_back(j);
            posSet.insert(j);
        }
        if (verticalProj[j] <= inputThre && inside) {
            //come outside
            inside = false;
            segEnds.push_back(j);
            posSet.insert(j);
        }
    }
    
    if (segStarts.size() > segEnds.size()) {
        segEnds.push_back(end - 1);
    }
    if (segStarts.size() != segEnds.size()) {
        cout << "start: " << start << " end: " << end << " not equal" << endl;
    }
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) + ".bmp";
        
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        
        unsigned char red = 255;
        
        
        
        for (int row = 0; row < bmpInfo.height; row++) {
            for (int col = 0; col < bmpInfo.width; col++) {
                if (posSet.find(col) != posSet.end()) {
                    fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
                    fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
                    fwrite(&red, sizeof(unsigned char), 1, processedBmp);
                }else{
                    fwrite(&inputGray[row][col], sizeof(unsigned char), 1, processedBmp);
                    fwrite(&inputGray[row][col], sizeof(unsigned char), 1, processedBmp);
                    fwrite(&inputGray[row][col], sizeof(unsigned char), 1, processedBmp);
                }
                
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
        }
        fclose(processedBmp);
    }
}

//horizontal edge detection
void Segmentation::sobelMaskBinary(vector<vector<unsigned char>>& inputGray, vector<vector<unsigned char>>& res, bool saveBmp, string subpath){
    res.clear();
    vector<vector<unsigned char>> sobelRes;
    int sobelMask[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0 ,1}};
    
    sobelRes.push_back(inputGray[0]);
    for (int i = 1; i < bmpInfo.height - 1; i++) {
        vector<unsigned char> sobelLine;
        sobelLine.push_back(inputGray[i][0]);
        
        for (int j = 1; j < bmpInfo.width - 1; j++) {
            int tmpRes = 0;
            for (int a = 0; a < 3; a++) {
                for (int b = 0; b < 3; b++) {
                    tmpRes += sobelMask[a][b] * inputGray[i + a - 1][j + b - 1];
                }
            }
            if (tmpRes < 0) {
                sobelLine.push_back(0);
            }else if(tmpRes > 255){
                sobelLine.push_back(255);
            }else{
                sobelLine.push_back((unsigned char)tmpRes);
            }
            
        }
        sobelLine.push_back(inputGray[i][bmpInfo.width-1]);
        sobelRes.push_back(sobelLine);
    }
    sobelRes.push_back(inputGray[bmpInfo.height-1]);
    
    vector<vector<short>> otsuBinary;
    OtsuGlobal(sobelRes, otsuBinary);
    
    vector<long> horizontalProj;
    long sum = 0;
    for (int i = 0; i < bmpInfo.height; i++) {
        long lineSum = 0;
        for (int j = 0; j < bmpInfo.width; j++) {
            lineSum += otsuBinary[i][j]/255;
        }
        horizontalProj.push_back(lineSum);
        sum += lineSum;
    }
    int horizontalThre = sum * 1.0 / bmpInfo.height;
    //    cout << "horizontal threshold: " << horizontalThre << endl;
    
    horizontalThre /= boundThreDivFactor;
    
    
    vector<int> lowerBounds;
    vector<int> upperBounds;
    bool inside = false;
    
    
    for (int i = 0; i < bmpInfo.height; i++) {
        
        if (horizontalProj[i] >= horizontalThre && !inside) {
            //come inside
            inside = true;
            lowerBounds.push_back(i);
        }
        if (horizontalProj[i] < horizontalThre && inside) {
            //come outside
            inside = false;
            upperBounds.push_back(i);
        }
    }
    
    if (lowerBounds.size() > 0) {
        int index = 0;
        while (lowerBounds[index+1] < 15 && index + 1 < lowerBounds.size()) {
            lowerBound = lowerBounds[index];
            index++;
        }
    }
    if (lowerBound < 5) {
        lowerBound = 5;
    }
    
    if (upperBounds.size() > 0) {
        int index = (int)upperBounds.size() - 1;
        while (upperBounds[index-1] > 65 && index - 1 >= 0) {
            upperBound = upperBounds[index];
            index--;
        }
        
        //        if (upperBound + boundLift < bmpInfo.height) {
        //            upperBound += boundLift;
        //        }else{
        //            upperBound = bmpInfo.height - 1;
        //        }
    }
    //    else{
    //        upperBound = 75;
    //    }
    if (upperBound > 75) {
        upperBound = 75;
    }
    
    //    cout << endl << "lowerbound: " << lowerBound << " upperbound: " << upperBound << endl << endl;
    
    
    vector<unsigned char> tmp;
    for (int i = 0; i < lowerBound; i++) {
        res.push_back(tmp);
        res[i].assign(bmpInfo.width, 0);
    }
    for (int i = lowerBound; i < upperBound; i++) {
        res.push_back(inputGray[i]);
    }
    for (int i = upperBound; i < bmpInfo.height; i++) {
        res.push_back(tmp);
        res[i].assign(bmpInfo.width, 0);
    }
    
    if (saveBmp) {
        FILE* processedBmp;
        string processedPath = globalPath + subpath + fileName.substr(0, fileName.length()-4) + ".bmp";
        
        if((processedBmp = fopen(processedPath.c_str(), "w")) == NULL ){
            printf("cannot create file.\n");
            exit(-2);
        }
        
        fwrite(bmpInfo.headBuffer, bmpInfo.biOffset, 1, processedBmp);
        
        unsigned char red = 255;
        for (int row = 0; row < bmpInfo.height; row++) {
            for (int col = 0; col < bmpInfo.width; col++) {
                if (row == lowerBound || row == upperBound) {
                    fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
                    fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
                    fwrite(&red, sizeof(unsigned char), 1, processedBmp);
                }else{
                    fwrite(&inputGray[row][col], sizeof(unsigned char), 1, processedBmp);
                    fwrite(&inputGray[row][col], sizeof(unsigned char), 1, processedBmp);
                    fwrite(&inputGray[row][col], sizeof(unsigned char), 1, processedBmp);
                }
                
            }
            for (int col = 0; col < bmpInfo.zeroPaddingLength; col++) {
                fwrite(&zeroChar, sizeof(unsigned char), 1, processedBmp);
            }
        }
        fclose(processedBmp);
    }
    
    
    
}










