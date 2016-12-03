#include "PlateDetect.h"
#include "ColorLocate.h"

bool debug = false;
bool colorDebug = false;

//判断是否是车牌框的参数
float rmin = 1.80; //1.8 1062.jpg  1.91 1276.jpg
float rmax = 4.95; //1300 4.9

float widthRatioMinInit = 0.3;
float widthRatioMin = widthRatioMinInit;
float widthRatioMax = 1.0;

float maxAngle = 60;

float errorAdmit = 1;//error < binary_plate_region.cols * binary_plate_region.cols / 10000 * errorAdmit

Mat src;
Mat blueMatch, yellowMatch;

string savePath = "/Users/laoreja/Downloads/work2/Data/license_plate_reg/";
//在savePath 路径下,需要有rect, binaryRotate, rot_dst, need_not_affine, can_affine, can_not_affine, affinePoints 以下名称的文件夹保存临时文件,除了 need_not_affine, can_affine, can_not_affine 三个文件夹外,其余均为临时结果.
string openPath = "/Users/laoreja/Downloads/work2/Data/license_plate_reg/bad_phone_reg_origin/";

int main( int argc, char** argv ){
    string fileName;
    string input;
    if (debug){
        cin >> input;
    }else{
        input = string(argv[1]);
    }
    cout << endl << input << endl;

    fileName = openPath + input;

    src = imread(fileName, 1);
    if (src.empty()){
        cout << "cannot open picture" << endl;
        return -1;
    }

    //preprocess
    Mat src_blur;
    medianBlur(src, src_blur, 5);

    //color locate
    colorMatch(src_blur, blueMatch, PlateColor::BLUE, true);
    Mat blueMorph;
    closing(blueMatch, blueMorph, PlateColor::BLUE);
    Mat finalMorph = blueMorph;

    vector<RotatedRect> rects;
    findPlate(blueMorph, rects);

    if(rects.size() == 0){
        colorMatch(src_blur, yellowMatch, YELLOW, true);
        Mat yellowMorph;
        closing(yellowMatch, yellowMorph, PlateColor::YELLOW);

        findPlate(yellowMorph, rects);
        finalMorph = yellowMorph;
    }

    if(debug){
        namedWindow("binary", WINDOW_AUTOSIZE);
        imshow("binary", finalMorph);
        waitKey(0);
    }

    if(rects.size() > 1){
        vector<RotatedRect>::iterator itRects = rects.begin();
        string ans = "n";
        Mat chooseRect;
        namedWindow("choose", WINDOW_AUTOSIZE);

        while (ans == "n" && itRects != rects.end()){
//            cout << "center: " << (*itRects).center << " size: " << (*itRects).size << endl;

            chooseRect = src.clone();
            Point2f vertices[4];
            (*itRects).points(vertices);
            for (int i = 0; i < 4; i++)
                line(chooseRect, vertices[i], vertices[(i+1)%4], Scalar(0,0,255), 2);

            imshow("choose", chooseRect);
            waitKey(500);
            cout << "Is this the plate locate? (answer 'y' or 'n')" << endl;
            cin >> ans;

            if(ans == "y"){
                ++itRects;
                rects.erase(itRects, rects.end());
            }else{
                itRects = rects.erase(itRects);
            }
        }
    }

    if(rects.size() == 0){
        cout << "color locate cannot be used here." << endl;
        return -1;
    }


    Mat display = src.clone();
    Point2f vertices[4];
    rects[0].points(vertices);
    for (int i = 0; i < 4; i++)
        line(display, vertices[i], vertices[(i+1)%4], Scalar(0,0,255), 2);

    if(colorDebug){
        cout<<"width: "<<rects[0].size.width << " height: " << rects[0].size.height << " angle: " << rects[0].angle << endl;
        namedWindow("rect", WINDOW_AUTOSIZE);
        imshow("rect", display);
        waitKey(0);
    }else{
        imwrite(savePath + "rect/" + input.substr(0, input.length() - 4) + ".bmp", display);
    }

    Mat rot_dst;
    rot_dst.create(int(src.rows * 1.5), int(src.cols * 1.5), src.type());

    Point2f srcTri[3];
    Point2f dstTri[3];

    srcTri[0] = Point(0, 0);
    srcTri[1] = Point(src.cols-1, 0);
    srcTri[2] = Point(0, src.rows - 1);

    int xdiff = (rot_dst.cols - src.cols) / 2;
    int ydiff = (rot_dst.rows - src.rows) / 2;
    dstTri[0] = Point(xdiff, ydiff);
    dstTri[1] = Point(rot_dst.cols - xdiff - 1, ydiff);
    dstTri[2] = Point(xdiff, rot_dst.rows - 1 - ydiff);

    Mat copyMat = getAffineTransform(srcTri, dstTri);
    warpAffine(src, rot_dst, copyMat, rot_dst.size());

//    if (debug){
//        namedWindow("try create", WINDOW_AUTOSIZE);
//        imshow("try create", rot_dst);
//        waitKey(0);
//    }

    double rotateAngle;
    int width, height;

    if (rects[0].size.width > rects[0].size.height){
        width = rects[0].size.width;
        height = rects[0].size.height;
        rotateAngle = rects[0].angle;
    }else{
        width = rects[0].size.height;
        height = rects[0].size.width;
        rotateAngle = (rects[0].angle > 0) ? -(90 - rects[0].angle) : 90 + rects[0].angle;
    }
    cout << "rotateAngle: " << rotateAngle << endl;

    Point newCenter = Point(xdiff + rects[0].center.x, ydiff + rects[0].center.y);

    Mat rot_mat = getRotationMatrix2D(newCenter, rotateAngle, 1);
    warpAffine(rot_dst, rot_dst, rot_mat, rot_dst.size());

    if(debug){
        namedWindow("rotate", WINDOW_AUTOSIZE);
        imshow("rotate", rot_dst);
    }else{
        imwrite(savePath+"rot_dst/" + input.substr(0, input.length() - 4) + ".bmp", rot_dst);
    }


    Mat binary_rot_dst;
    binary_rot_dst.create(int(finalMorph.rows * 1.5), int(finalMorph.cols * 1.5), finalMorph.type());
    warpAffine(finalMorph, binary_rot_dst, copyMat, binary_rot_dst.size());
    warpAffine(binary_rot_dst, binary_rot_dst, rot_mat, binary_rot_dst.size());

    if(debug){
        namedWindow("binary rotate", WINDOW_AUTOSIZE);
        imshow("binary rotate", binary_rot_dst);
    }else{
        imwrite(savePath + "binaryRotate/" + input.substr(0, input.length() - 4) + ".bmp", binary_rot_dst);
    }

    int startX = newCenter.x - width / 2;
    if (startX < 0) startX = 0;
    int endX = newCenter.x + width / 2;
    if (endX > rot_dst.cols - 1) endX = rot_dst.cols - 1;
    int startY = newCenter.y - height / 2;
    if (startY < 0) startY = 0;
    int endY = newCenter.y + height / 2;
    if (endY > rot_dst.rows - 1) endY = rot_dst.rows - 1;;

    Mat plate_region = rot_dst(Range(startY, endY), Range(startX, endX));
    Mat binary_plate_region = binary_rot_dst(Range(startY, endY), Range(startX, endX));

    if(debug){
        namedWindow("binary plate region", WINDOW_AUTOSIZE);
        imshow("binary plate region", binary_plate_region);
    }

    //affine adjustment
    Mat affinePoint = binary_plate_region.clone();
    cvtColor(affinePoint, affinePoint, COLOR_GRAY2BGR);

    vector<int> xArray, yArray;
    int count = 31;
    int xsum = 0, ysum = 0, xavg, yavg;
    long segmaXY = 0, segmaX2 = 0;

    for(int i = 10; i < 41; i++){
        int x = int(binary_plate_region.rows * 1.0/ 49 * i);
        int y = calcSequentialZeroPoint(binary_plate_region, x);
        xArray.push_back(x);
        yArray.push_back(y);
        xsum += x;
        ysum += y;
        segmaX2 += x*x;
        segmaXY += x*y;

        circle(affinePoint, Point(y, x), 1, Scalar(0, 0, 255), -1);
    }
    xavg = xsum / count;
    yavg = ysum / count;
    imwrite(savePath + "affinePoints/" + input.substr(0, input.length() - 4) + ".bmp", affinePoint);

    double b = (segmaXY - count * xavg * yavg) * 1.0 / (segmaX2 - count * xavg * xavg);
    double a = yavg - b * xavg;

    long error = 0;
    for(int i = 0; i < count; i++){
        error += (yArray[i] - (b * xArray[i] + a)) * (yArray[i] - (b * xArray[i] + a));
    }
    error /= count;
    cout << "b: " << b << " a: " << a << " error: " << error << endl;

    int affine_x = (int)(b * binary_plate_region.rows / 2 + a);

    if(abs(b) < 0.086){
        imwrite(savePath + "need_not_affine/" + input.substr(0, input.length() - 4) + ".bmp", plate_region);
    }else if(error < binary_plate_region.cols * binary_plate_region.cols / 10000 * errorAdmit){
        cout << "affine_x =: " << affine_x << endl;
        cout << "do affine!" << endl;
//        if (firstHalfSum / firstHalfCount <= secondHalfSum / secondHalfCount){
        if(b > 0){
            cout << "first half <= second half, 向左仿射" << endl;
            srcTri[0] = Point2f( 0,0 );
            srcTri[1] = Point2f( plate_region.cols - 1 - affine_x * 2, 0 );
            srcTri[2] = Point2f( affine_x * 2, plate_region.rows - 1 );
        }else{
            cout << "first half > second half, 向右仿射" << endl;
            srcTri[0] = Point2f(affine_x * 2, 0);
            srcTri[1] = Point2f(plate_region.cols - 1, 0);
            srcTri[2] = Point2f(0, plate_region.rows - 1);
        }

        dstTri[0] = Point2f(affine_x, 0);
        dstTri[1] = Point2f(plate_region.cols - 1 - affine_x, 0);
        dstTri[2] = Point2f(affine_x, plate_region.rows - 1);


        Mat warp_dst = Mat::zeros(plate_region.rows, plate_region.cols, plate_region.type());
        Mat warp_mat = getAffineTransform (srcTri, dstTri);
        warpAffine(plate_region, warp_dst, warp_mat, warp_dst.size());
        if(colorDebug){
            namedWindow("affine", WINDOW_AUTOSIZE);
            imshow("affine", warp_dst);
            waitKey(0);
        }
        imwrite(savePath + "can_affine/" + input.substr(0, input.length() - 4) + ".bmp", warp_dst);

    } else{
        imwrite(savePath + "can_not_affine/" + input.substr(0, input.length() - 4) + ".bmp", plate_region);
    }

    if(debug){
        waitKey(0);
    }
//old version, may be need updates
//
//        namedWindow("origin", CV_WINDOW_AUTOSIZE);
//        imshow("origin", src);
//
//        namedWindow("blue match", CV_WINDOW_AUTOSIZE);
//        createTrackbar("blue min", "blue match", &min_blue, 150, blueOnChange);
//        createTrackbar("blue max", "blue match", &max_blue, 160, blueOnChange);

//    namedWindow("yellow match", CV_WINDOW_AUTOSIZE);
//    createTrackbar("blue min", "yellow match", &min_yellow, 20, yellowOnChange);
//    createTrackbar("blue max", "yellow match", &max_yellow, 45, yellowOnChange);

//        waitKey(0);
}

int calcSequentialZeroPoint(Mat& src, int rowNum){
    int i = 0;
    int count = 0;
    while(i < src.cols && src.at<uchar>(rowNum, i) == 0){
        count++;
        i++;
    }
    return count;

}

bool sizeJudge(RotatedRect mr, int srcWidth){

    float ratio = mr.size.width * 1.0 / mr.size.height;
    float wRatio = mr.size.width * 1.0 / srcWidth;
    float rotateAngle = abs(mr.angle);
    if (ratio < 1){
        ratio = mr.size.height * 1.0 / mr.size.width;
        wRatio = mr.size.height * 1.0 / srcWidth;
        rotateAngle = 90 - rotateAngle;

    }
    if(debug)
        cout << "ratio: " << ratio << " wRatio: " << wRatio << " angle: " << rotateAngle << endl;

    if (ratio < rmin || ratio > rmax || wRatio < widthRatioMin || wRatio > widthRatioMax || rotateAngle > maxAngle) {
        return false;
    } else{
        return true;
    }
}





