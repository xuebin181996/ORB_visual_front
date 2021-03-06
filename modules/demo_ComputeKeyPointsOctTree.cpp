//
// Created by lab on 20-2-8.
//


#include "ComputeKeyPointsOctTree/ComputeKeyPointsOctTree.h"

using namespace std;
using namespace cv;
string data_path = "../dates/";
// 设置特征点提取需要的一些参数
int nFeatures = 1000;//图像金字塔上特征点的总数量
int nLevels = 8;//图像金字塔层数
float fScaleFactor = 1.2;//金字塔比例因子
int fIniThFAST = 20;//检测fast角点阈值
int fMinThFAST = 8;//最低阈值



int main(int argc, char** argv) {
    cv::Mat image = cv::imread(data_path+"test1.png", 0);

    //TODO this shows how to read file to load parameter------------------------------------------------
    cv::FileStorage fSettings(data_path+"EuRoC.yaml", cv::FileStorage::READ);
    float fx = fSettings["Camera.fx"];
    float fy = fSettings["Camera.fy"];
    float cx = fSettings["Camera.cx"];
    float cy = fSettings["Camera.cy"];
    cv::Mat K = cv::Mat::eye(3, 3, CV_32F);
    K.at<float>(0, 0) = fx;
    K.at<float>(1, 1) = fy;
    K.at<float>(0, 2) = cx;
    K.at<float>(1, 2) = cy;
    cv::Mat DistCoef(4, 1, CV_32F);
    DistCoef.at<float>(0) = fSettings["Camera.k1"];
    DistCoef.at<float>(1) = fSettings["Camera.k2"];
    DistCoef.at<float>(2) = fSettings["Camera.p1"];
    DistCoef.at<float>(3) = fSettings["Camera.p2"];
    const float k3 = fSettings["Camera.k3"];
    if (k3 != 0) {
        DistCoef.resize(5);
        DistCoef.at<float>(4) = k3;
    }
    float mbf = fSettings["Camera.bf"];
    float fps = fSettings["Camera.fps"];
    if (fps == 0)
        fps = 30;

    int nFeatures = fSettings["ORBextractor.nFeatures"];
    float fScaleFactor = fSettings["ORBextractor.scaleFactor"];
    int nLevels = fSettings["ORBextractor.nLevels"];
    int fIniThFAST = fSettings["ORBextractor.iniThFAST"];
    int fMinThFAST = fSettings["ORBextractor.minThFAST"];


    std::vector<float> mvScaleFactor;
    std::vector<float> mvInvScaleFactor;
    std::vector<float> mvLevelSigma2;
    std::vector<float> mvInvLevelSigma2;
    std::vector<Mat> mvImagePyramid;
    mvScaleFactor.resize(nLevels);
    mvLevelSigma2.resize(nLevels);
    mvImagePyramid.resize(nLevels);
    mvInvScaleFactor.resize(nLevels);
    mvInvLevelSigma2.resize(nLevels);



    const int PATCH_SIZE = 31;
    const int HALF_PATCH_SIZE = 15;
    const int EDGE_THRESHOLD = 19;
    mvScaleFactor[0] = 1.0f;
    mvLevelSigma2[0] = 1.0f;

    for (int i = 1; i < nLevels; i++) {
        mvScaleFactor[i] = mvScaleFactor[i - 1] * fScaleFactor;
        mvLevelSigma2[i] = mvScaleFactor[i] * mvScaleFactor[i];
    }

    for (int i = 0; i < nLevels; i++) {
        mvInvScaleFactor[i] = 1.0f / mvScaleFactor[i];
        mvInvLevelSigma2[i] = 1.0f / mvLevelSigma2[i];
    }


    int nlevels = 8;
    for (int level = 0; level < nlevels; ++level) {

        float scale = mvInvScaleFactor[level];

        Size sz(cvRound((float) image.cols * scale), cvRound((float) image.rows * scale));

        Size wholeSize(sz.width + EDGE_THRESHOLD * 2, sz.height + EDGE_THRESHOLD * 2);

        Mat temp(wholeSize, image.type()), masktemp;

        mvImagePyramid[level] = temp(Rect(EDGE_THRESHOLD, EDGE_THRESHOLD, sz.width, sz.height));

        // Compute the resized image
        if (level != 0) {
            resize(mvImagePyramid[level - 1], mvImagePyramid[level], sz, 0, 0, INTER_LINEAR);

            copyMakeBorder(mvImagePyramid[level], temp, EDGE_THRESHOLD, EDGE_THRESHOLD, EDGE_THRESHOLD,
                           EDGE_THRESHOLD,
                           BORDER_REFLECT_101 + BORDER_ISOLATED);
        } else {
            copyMakeBorder(image, temp, EDGE_THRESHOLD, EDGE_THRESHOLD, EDGE_THRESHOLD, EDGE_THRESHOLD,
                           BORDER_REFLECT_101);
        }
    }



    Mat workingMat;


    vector<int> mnFeaturesPerLevel;
    mnFeaturesPerLevel.resize(nlevels);
    float factor = 1.0f / fScaleFactor;
    float nDesiredFeaturesPerScale =
            nFeatures * (1 - factor) / (1 - (float) pow((double) factor, (double) nlevels));

    int sumFeatures = 0;
    for (int level = 0; level < nlevels - 1; level++) {
        mnFeaturesPerLevel[level] = cvRound(nDesiredFeaturesPerScale);
        sumFeatures += mnFeaturesPerLevel[level];
        nDesiredFeaturesPerScale *= factor;
    }
    mnFeaturesPerLevel[nlevels - 1] = std::max(nFeatures - sumFeatures, 0);

    vector<vector<KeyPoint> > allKeypoints;
    int minThFAST = 7;
    vector<KeyPoint> keypointsFinal;
    ComputeKeyPointsOctTree(mvImagePyramid, allKeypoints,
                            minThFAST,
                            mnFeaturesPerLevel,
                            mvScaleFactor,
                            nlevels,keypointsFinal);

    cv::drawKeypoints(image, keypointsFinal, image);

    cout << "in total "<<keypointsFinal.size() <<" keypoints"<< endl;
    cv::imshow("image", image);

    cv::waitKey(0);
    return 0;

}