//
//  rotateCorrect.cpp
//  LicensePlateSeg_final
//
//  Created by 顾秀烨 on 11/28/15.
//  Copyright © 2015 laoreja. All rights reserved.
//

#include "rotateCorrect.hpp"

bool debug = false;
string grayOpenPath = "/Users/laoreja/Downloads/work2/车牌/bmp/contrastGray/";
string originOpenPath = "/Users/laoreja/Downloads/work2/车牌/bmp/pics/";
//you need to create "hough", "grayAfterRotate", "originalAfterRotate" folders in your savePath

string savePath = "/Users/laoreja/study/MachineLearning/result/";
//string savePath = "/Users/laoreja/Downloads/work2/车牌/bmp/pics/";

///parameters that can be adjusted:
int borderWidth = 15; // the border used for hough
double rotate_modify = 2;
double rotate_mod_ratio = 0.8;

int minLineLength = 50;
int maxLineGap = 15;


double maxThetaLimit = 13; // only count theta < maxThetaLimit
double minThetaLimit = 2.5;
double thetaThreForCorrect = 3.95;

int startThreshold = 40;


/// Global variables
const char source_window[] = "Source image";
const char warp_rotate_window[] = "Rotate";

void setGrayOpenPath(string& path) {
    grayOpenPath = path;
}

void setOriginOpenPath(string& path) {
    originOpenPath = path;
}

void setSavePath(string& path) {
    savePath = path;
}

//int main(void){
//    for (int i = 1; i < 988; i++) {
//        rotateCorrect(to_string(i) + ".bmp");
//    }
//}
void rotateCorrect(string fileName)
{
    //    string fileName;
    //    if (debug) {
    //        cin >> fileName;
    //    }else{
    //        fileName = string(argv[1]);
    //    }
    
    cout << fileName << endl;
    
    /// Load the image
    Mat src;
    src = imread( grayOpenPath + fileName, 1 );
    if(src.empty()){
        cout << "in rotateCorrect, src cannot open file." << endl;
        return;
    }
    
    //preprocess
    Mat src_gray;
    cvtColor( src, src_gray, COLOR_BGR2GRAY );
    
    Mat edges;
    blur( src_gray, edges, Size(3,3) );
    Canny( edges, edges, 100, 300, 3 );
    
    
    Mat edgeDst;
    edgeDst.create( src.size(), src.type() );
    edgeDst = Scalar::all(0);
    src_gray.copyTo( edgeDst, edges ); // src_gray?
    
    
    //Probabilistic hough
    Mat probabilistic_hough = edgeDst.clone();
    cvtColor(probabilistic_hough, probabilistic_hough, COLOR_GRAY2BGR);
    
    int threshold = startThreshold;
    vector<Vec4i> upper_lines, lower_lines;
    
    Mat upperBorder = edgeDst(Range(0, borderWidth), Range::all());
    Mat lowerBorder = edgeDst(Range(80 - borderWidth, 80), Range::all());
    
    
    while(threshold >= 5 && upper_lines.size() < 2){
        upper_lines.clear();
        HoughLinesP( upperBorder, upper_lines, 1, CV_PI/180, threshold, minLineLength, maxLineGap );
        threshold -= 5;
    }
    
    threshold = startThreshold;
    while(threshold >= 5 && lower_lines.size() < 2){
        lower_lines.clear();
        HoughLinesP( lowerBorder, lower_lines, 1, CV_PI/180, threshold, minLineLength, maxLineGap );
        threshold -= 5;
    }
    
    double theta_sum = 0;
    int theta_count = 0;
    Vec4i l;
    double theta;
    
    for (int i = 0; i < upper_lines.size(); ++i)
    {
        theta = atan((upper_lines[i][3] - upper_lines[i][1])* 1.0 / (upper_lines[i][2] - upper_lines[i][0])) / CV_PI * 180;
        
        cout << "upper " << theta << endl;
        // cout << "coordinates: " << upper_lines[i][0] << " " << upper_lines[i][1] << " " << upper_lines[i][2] << " " << upper_lines[i][3] << endl;
        
        if (abs(theta) < maxThetaLimit && abs(theta) > minThetaLimit)
        {
            theta_sum += theta;
            theta_count++;
            //draw the line
            l = upper_lines[i];
            line( probabilistic_hough, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 1, LINE_AA);
        }
    }
    
    for (int i = 0; i < lower_lines.size(); ++i)
    {
        theta = atan((lower_lines[i][3] - lower_lines[i][1])* 1.0 / (lower_lines[i][2] - lower_lines[i][0])) / CV_PI * 180;
        
        cout << "lower " << theta << endl;
        // cout << "coordinates: " << lower_lines[i][0] << " " << lower_lines[i][1] << " " << lower_lines[i][2] << " " << lower_lines[i][3] << endl;
        
        if (abs(theta) < maxThetaLimit && abs(theta) > minThetaLimit)
        {
            theta_sum += theta;
            theta_count++;
            //draw the line
            l = lower_lines[i];
            line( probabilistic_hough, Point(l[0], l[1] + 80 - borderWidth), Point(l[2], l[3] + 80 - borderWidth), Scalar(0,0,255), 1, LINE_AA);
        }
    }
    
    if (debug) {
        namedWindow("probabilistic_hough", WINDOW_AUTOSIZE);
        imshow("probabilistic_hough", probabilistic_hough);
        waitKey(0);
    }
    imwrite(savePath + "hough/" + fileName, probabilistic_hough);
    
    Mat origin = imread(originOpenPath + fileName, CV_LOAD_IMAGE_COLOR );
    if (!origin.data)
    {
        cout << "origin picture read error: " << fileName << endl;
        return;
    }
    
    double correctAngle;
    if (theta_count == 0)
    {
        cout << "theta count == 0 " << endl;
        correctAngle = 0;
    }else{
        correctAngle = theta_sum / theta_count;
    }
    
    cout << " angle equals: "<< correctAngle << endl;
    
    if ( abs(correctAngle) >= thetaThreForCorrect )
    {
        Mat rot_mat, rotate_dst;
        /** Rotating the image */
        
        /// Compute a rotation matrix with respect to the center of the image
        Point center = Point( src.cols/2, src.rows/2 );
        double scale = 1; // do we need to scale the picture to retain all info?
        
        double angle = correctAngle * rotate_mod_ratio;
        if (angle > 0)
        {
            angle -= rotate_modify;
        }else{
            angle += rotate_modify;
        }
        cout << "rotate angle: " << angle << endl;
        rot_mat = getRotationMatrix2D( center, angle, scale );
        
        /// Rotate the warped image
        warpAffine( src, rotate_dst, rot_mat, src.size() );
        
        if (debug) {
            //         Show what you got
            namedWindow( source_window, WINDOW_AUTOSIZE );
            imshow( source_window, src );
            waitKey(0);
            
            namedWindow( warp_rotate_window, WINDOW_AUTOSIZE );
            imshow( warp_rotate_window, rotate_dst );
            waitKey(0);
        }
        imwrite( savePath + "grayAfterRotate/" + fileName, rotate_dst );
        
        
        warpAffine( origin, origin, rot_mat, origin.size() );
        if (debug) {
            namedWindow("original after rotate", WINDOW_AUTOSIZE);
            imshow("original after rotate", origin);
            waitKey(0);
        }
        imwrite( savePath + "originalAfterRotate/" + fileName, origin );
        
        //        cvtColor( edgeDst, edgeDst, COLOR_GRAY2BGR );
        //        warpAffine( edgeDst, edgeDst, rot_mat, edgeDst.size() );
        // namedWindow("edge after rotate", WINDOW_AUTOSIZE);
        // imshow("edge after rotate", edgeDst);
        // waitKey(0);
        
    }else{
        system( ("cp " + originOpenPath + fileName + " " + savePath + "originalAfterRotate/" + fileName).c_str() );
        system( ("cp " + grayOpenPath + fileName + " " + savePath + "grayAfterRotate/" + fileName).c_str() );
    }
    //    imwrite( savePath +"edgesAfterRotate/" + fileName, edgeDst );
    
}
