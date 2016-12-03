//
// Created by 顾秀烨 on 11/26/15.
//
#include "PlateDetect.h"
#include "ColorLocate.h"

//blue的H范围
int min_blue = 90;  //100
int max_blue = 150;  //140

//yellow的H范围
int min_yellow = 15; //15
int max_yellow = 40; //40

//morphology
//closing operation
int color_morph_width = 15;
int color_morph_height = 3;

extern bool colorDebug;


void closing(Mat &src, Mat &dst, PlateColor color){
    //may be not needed
    threshold(src, dst, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);


    Mat element = getStructuringElement(MORPH_RECT, Size(color_morph_width, color_morph_height));
    morphologyEx(dst, dst, MORPH_CLOSE, element);

//    if(debug){
//        namedWindow("threshold", WINDOW_AUTOSIZE);
//        imshow("threshold", dst);
//    }
}

void findPlate(Mat& morphInput, vector<RotatedRect>& resRects){
    resRects.clear();
    Mat tmp;
    morphInput.copyTo(tmp);

    vector<vector<Point>> contours;

    Mat display = morphInput.clone();
    cvtColor(display, display, COLOR_GRAY2BGR);

    findContours(tmp, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    vector<vector<Point>>::iterator contourIt = contours.begin();


    while(contourIt != contours.end()){
        RotatedRect rr = minAreaRect(Mat(*contourIt));
        if(colorDebug){
            Point2f vertices[4];
            rr.points(vertices);
            for (int i = 0; i < 4; i++)
                line(display, vertices[i], vertices[(i+1)%4], Scalar(0,0,255), 2);
            cout<<"width: "<< rr.size.width << " height: " << rr.size.height << " angle: " << rr.angle << endl;
        }

        if(!sizeJudge(rr, morphInput.cols)){
            contourIt = contours.erase(contourIt);
        }else{
            ++contourIt;
            resRects.push_back(rr);
        }
    }

    if(colorDebug){
        namedWindow("all rects", WINDOW_AUTOSIZE);
        imshow("all rects", display);
        waitKey(0);
    }

}

Mat colorMatch(const Mat& src, Mat& match, const PlateColor r, const bool adaptive_minsv)
{
    // S和V的最小值由adaptive_minsv这个bool值判断
    // 如果为true，则最小值取决于H值，按比例衰减
    // 如果为false，则不再自适应，使用固定的最小值minabs_sv
    // 默认为false
    const float max_sv = 255;
    const float minref_sv = 64;

    const float minabs_sv = 95;


    Mat src_hsv;
    // 转到HSV空间进行处理，颜色搜索主要使用的是H分量进行蓝色与黄色的匹配工作
    cvtColor(src, src_hsv, CV_BGR2HSV);

    vector<Mat> hsvSplit;
    split(src_hsv, hsvSplit);
    equalizeHist(hsvSplit[2], hsvSplit[2]);
    merge(hsvSplit, src_hsv);

    //匹配模板基色,切换以查找想要的基色
    int min_h = 0;
    int max_h = 0;
    switch (r) {
        case PlateColor::BLUE:
            min_h = min_blue;
            max_h = max_blue;
            break;
        case PlateColor::YELLOW:
            min_h = min_yellow;
            max_h = max_yellow;
            break;
    }

    float diff_h = float((max_h - min_h) / 2);
    int avg_h = min_h + diff_h;

    int channels = src_hsv.channels();
    int nRows = src_hsv.rows;
    //图像数据列需要考虑通道数的影响；
    int nCols = src_hsv.cols * channels;

    if (src_hsv.isContinuous())//连续存储的数据，按一行处理
    {
        nCols *= nRows;
        nRows = 1;
    }

    int i, j;
    uchar* p;
    float s_all = 0;
    float v_all = 0;
    float count = 0;
    for (i = 0; i < nRows; ++i)
    {
        p = src_hsv.ptr<uchar>(i);
        for (j = 0; j < nCols; j += 3)
        {
            int H = int(p[j]); //0-180
            int S = int(p[j + 1]);  //0-255
            int V = int(p[j + 2]);  //0-255

            s_all += S;
            v_all += V;
            count++;

            bool colorMatched = false;

            if (H > min_h && H < max_h)
            {
                int Hdiff = 0;
                if (H > avg_h)
                    Hdiff = H - avg_h;
                else
                    Hdiff = avg_h - H;

                float Hdiff_p = float(Hdiff) / diff_h;

                // S和V的最小值由adaptive_minsv这个bool值判断
                // 如果为true，则最小值取决于H值，按比例衰减
                // 如果为false，则不再自适应，使用固定的最小值minabs_sv
                float min_sv = 0;
                if (true == adaptive_minsv)
                    min_sv = minref_sv - minref_sv / 2 * (1 - Hdiff_p); // minref_sv - minref_sv / 2 * (1 - Hdiff_p)
                else
                    min_sv = minabs_sv; // add

                if ((S > min_sv && S < max_sv) && (V > min_sv && V < max_sv))
                    colorMatched = true;
            }

            if (colorMatched == true) {
                p[j] = 0; p[j + 1] = 0; p[j + 2] = 255;
            }
            else {
                p[j] = 0; p[j + 1] = 0; p[j + 2] = 0;
            }
        }
    }

    //cout << "avg_s:" << s_all / count << endl;
    //cout << "avg_v:" << v_all / count << endl;

    // 获取颜色匹配后的二值灰度图
    Mat src_grey;
    vector<Mat> hsvSplit_done;
    split(src_hsv, hsvSplit_done);
    src_grey = hsvSplit_done[2];

    match = src_grey;

    return src_grey;
}

//extern Mat src, blueMatch, yellowMatch;
//extern float widthRatioMinInit, widthRatioMin, widthRationMax;
//extern bool debug;
//
//void blueOnChange(int, void*){
//    cout << "min blue: " << min_blue << " max blue: " << max_blue << endl;
//    colorMatch(src, blueMatch, BLUE, true);
//    imshow("blue match", blueMatch);
//
//    Mat morph;
//    closing(blueMatch, morph, PlateColor::BLUE);
//
//    vector<RotatedRect> rects;
//    widthRatioMin = widthRatioMinInit;
//    while(rects.size() < 1 && widthRatioMin > 0.1){
//        findPlate(morph, rects);
//        widthRatioMin -= 0.05;
//    }
//    if(rects.size() > 1){
//        for(vector<RotatedRect>::iterator rrit = rects.begin(); rrit != rects.end(); rrit++){
//            cout << "rect width: "<<(*rrit).size.width << " height: " << (*rrit).size.height << endl;
//            cout << "do you want to keep it? if yes, input 'y'" << endl;
//            string ans;
//            cin >> ans;
//            if(ans != "y") {
//                rrit = rects.erase(rrit);
//            }
//        }
//    }
//
//    if(debug){
//        Mat rectMat = src.clone();
//        Point2f vertices[4];
//        for(int k = 0; k < rects.size(); k++){
//            rects[k].points(vertices);
//            for (int i = 0; i < 4; i++)
//                line(rectMat, vertices[i], vertices[(i+1)%4], Scalar(0,0,255));
//        }
//
//        namedWindow("blue rects", WINDOW_AUTOSIZE);
//        imshow("blue rects", rectMat);
//    }
//
//}
//
//
//void yellowOnChange(int, void*){
//    cout << "min yellow: " << min_yellow << " max yellow: " << max_yellow << endl;
//    colorMatch(src, yellowMatch, PlateColor::YELLOW, true);
//    imshow("yellow match", yellowMatch);
//
//    Mat morph;
//    closing(yellowMatch, morph, PlateColor::YELLOW);
//
//    vector<RotatedRect> rects;
//    widthRatioMin = widthRatioMinInit;
//    while(rects.size() < 1 && widthRatioMin > 0.1){
//        findPlate(morph, rects);
//        widthRatioMin -= 0.05;
//    }
//    if(rects.size() > 1){
//        for(vector<RotatedRect>::iterator rrit = rects.begin(); rrit != rects.end(); rrit++){
//            cout << "rect width: "<<(*rrit).size.width << " height: " << (*rrit).size.height << endl;
//            cout << "do you want to keep it? if yes, input 'y'" << endl;
//            string ans;
//            cin >> ans;
//            if(ans != "y") {
//                rrit = rects.erase(rrit);
//            }
//        }
//    }
//
//    if(debug){
//        Mat rectMat = src.clone();
//        Point2f vertices[4];
//        for(int k = 0; k < rects.size(); k++){
//            rects[k].points(vertices);
//            for (int i = 0; i < 4; i++)
//                line(rectMat, vertices[i], vertices[(i+1)%4], Scalar(0,0,255));
//        }
//
//        namedWindow("yellow rects", WINDOW_AUTOSIZE);
//        imshow("yellow rects", rectMat);
//    }
//}
