#pragma once

#ifndef __OPENCVLAPLACIAN_H__
#define __OPENCVLAPLACIAN_H__

// opencv include
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/videoio.hpp>

using namespace cv;
using namespace std;

#include "stdafx.h"
#include "OpencvLaplacian.h"

class COpencvLaplacian
{
protected:
    ///void PrepareSlices();

public:
	double COpencvLaplacian::Blur_Detect(Mat img);
	int COpencvLaplacian::LosdfindContour(Mat img, vector<Point2f> &center);
	int COpencvLaplacian::findchart(Mat img, vector<Point2f> &center);
	int COpencvLaplacian::findContractAndBrightness(Mat img, int Shift, int x, int y, int limit);
	int COpencvLaplacian::shitfcalibration(Mat img, int* center_x, int* center_y, int* center_counter);
	///COpencvLaplacian();

	void DebugLog(char *pData, int nLen, bool bHex);
};

#endif // !__OPENCVLAPLACIAN_H__
