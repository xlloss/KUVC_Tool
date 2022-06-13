#include "stdafx.h"
#include "OpencvLaplacian.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>

using namespace cv;
using namespace std;
Mat src_gray;
int thresh = 100;
RNG rng(12345);

/**
 * @function main
 */

double COpencvLaplacian::Laplacian(Mat img)
{
	cv::Mat gray, laplacian;
	cv::Mat mean, dev;

	// color to gray level
	cv::cvtColor(img, gray, COLOR_BGR2GRAY);

	// laplacian
	cv::Laplacian(gray, laplacian, CV_64F);

	// mean and deviation, 計算矩陣的均值和標準差
	cv::meanStdDev(laplacian, mean, dev);

	return dev.at<double>(0, 0);
}

int COpencvLaplacian::LosdfindContour(Mat img, vector<Point2f> &center)
{
	Mat canny_output;
	cv::Mat gray, imgtt, red_mask0, red_mask1, red_mask;
	cv::Mat mean, dev;

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	

	// color to gray level
	int center_count = 0;
	
	imgtt = cv::imread("1.jpg", IMREAD_COLOR);
	cv::cvtColor(imgtt, gray, COLOR_BGR2HSV);

	cv::inRange(gray, Scalar(0, 70, 50), Scalar(15, 255, 255), red_mask0);
	cv::inRange(gray, Scalar(165, 70, 50), Scalar(180, 255, 255), red_mask1);
	
	cv::bitwise_or(red_mask0, red_mask1, red_mask);
	cv::findContours(red_mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));
	
	//計算影象的中心矩
	vector<Moments> mu(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		mu[i] = moments(contours[i], false);
	}

	//計算影象的質心(中心)
	vector<Point2f> mc(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		if (mu[i].m00 > 5000)
		{
			if (center_count < 5)
			{
				center[center_count] = Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
			}
			center_count++;		
			mc[i] = Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
		}
	}
	
	//在中心點畫圓
	Mat drawing = Mat::zeros(imgtt.size(), CV_8UC1);
	for (int i = 0; i < contours.size(); i++)
	{
		Scalar color = Scalar(255);
		drawContours(drawing, contours, i, color, 2, 8, hierarchy, 0, Point());
		circle(drawing, mc[i], 4, color, 3, 8, 0); //畫圓	
	}

	//imshow("outImage", red_mask);
	//imshow("outImage2", drawing);
	//cv::waitKey(0);

//CString str;
//str.Format(L"center_count=%d", center_count);
//AfxMessageBox(str);

	//if (center_count == 5)
	if (center_count >= 3)
		return 1;
	else
		return 0;	
}

int COpencvLaplacian::findchart(Mat img, vector<Point2f> &center)
{
	//Mat canny_output;
	cv::Mat gray, red_mask0, red_mask1, red_mask;
	//cv::Mat mean, dev;

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	
	int center_count = 0;

//char buf[512];
//SYSTEMTIME st;
//GetLocalTime(&st); //Get system local time
//sprintf(buf, "findchart Start-%02d:%02d:%02d.%03d\r\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
//DebugLog(buf, strlen(buf), false);

	// color to gray level
	cv::cvtColor(img, gray, COLOR_BGR2HSV);

	cv::inRange(gray, Scalar(0, 70, 50), Scalar(15, 255, 255), red_mask0);
	cv::inRange(gray, Scalar(165, 70, 50), Scalar(180, 255, 255), red_mask1); //165=180-15
	cv::bitwise_or(red_mask0, red_mask1, red_mask); //red_mask0與red_mask1進行or處理,結果存到red_mask

	cv::findContours(red_mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0)); //取得輪廓

////char buf[256];
//sprintf(buf, "contours=%d\r\n", contours.size());
//DebugLog(buf, strlen(buf), false);

	//計算影象的質心(中心)
	vector<Moments> mu(contours.size());
	for (int i = 1; i < contours.size(); i++)
	{
		mu[i] = moments(contours[i], false);
	}

	//計算影象的質心(中心)座標
	for (int i = 0; i < contours.size(); i++)
	{
		if (mu[i].m00 > 5000)
		{
			if(center_count < 5)
				center[center_count] = Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
			center_count++;
		}
	}

//GetLocalTime(&st); //Get system local time
//sprintf(buf, "findchart End-%02d:%02d:%02d.%03d\r\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
//DebugLog(buf, strlen(buf), false);

	/*
	//在中心點畫圓
	vector<Point2f> mc(contours.size());
	Mat drawing = Mat::zeros(img.size(), CV_8UC1);
	for (int i = 0; i < contours.size(); i++)
	{
		Scalar color = Scalar(255);
		drawContours(drawing, contours, i, color, 2, 8, hierarchy, 0, Point());
		circle(drawing, mc[i], 4, color, 3, 8, 0);
	}
	*/

//if (center_count == 5)
//{
//char buf[512];
//sprintf(buf, "g_iVideoX1=%d, g_iVideoY1=%d, g_iVideoX2=%d, g_iVideoY2=%d, g_iVideoX3=%d, g_iVideoY3=%d, g_iVideoX4=%d, g_iVideoY4=%d, g_iVideoX5=%d, g_iVideoY5=%d\r\n", center[0].x, center[0].y, center[1].x, center[1].y, center[2].x, center[2].y, center[3].x, center[3].y, center[4].x, center[4].y);
//DebugLog(buf, strlen(buf), false);
//}

	//imshow("outImage", red_mask);
	//imshow("outImage2", drawing);
	//cv::waitKey(0);
	
	if (center_count == 5)
		return 1;
	else
		return 0;
}

int COpencvLaplacian::findContractAndBrightness(Mat img, int Shift , int x, int y , int limit)
{
	Mat gray;
	cv::cvtColor(img, gray, COLOR_BGR2HSV);
	Vec3b hsv;
	int from = x / 2 - 1;
	int under_line = (y-1) - Shift;
	int V,i;
	
	for(i = 0 ;i<=2 ; i++)
	{ 
		hsv = gray.at<Vec3b>(Shift-1, from +i);
		V = hsv.val[2];
		printf("up v=%d \r\n", V);
		if (limit > V)
			return 0;
	}

	for (i = 0; i <= 2; i++)
	{
		hsv = gray.at<Vec3b>(under_line, from + i);
		V = hsv.val[2];
		printf("down v=%d \r\n", V);
		if (limit > V)
			return 0;
	}

	return 1;
}

#define swap(x,y) {int t; t=x; x=y; y=t; }

int partition(int number[], int left, int right) {
	int i = left - 1;
	int j;
	for (j = left; j < right; j++) {
		if (number[j] <= number[right]) {
			i++;
			swap(number[i], number[j]);
		}
	}

	swap(number[i + 1], number[right]);
	return i + 1;
}

void quickSort(int number[], int left, int right) {
	if (left < right) {
		int q = partition(number, left, right);
		quickSort(number, left, q - 1);
		quickSort(number, q + 1, right);
	}
}


int COpencvLaplacian::shitfcalibration(Mat img, int* center_x, int* center_y, int* center_counter)
{
	static int  sum_x = 0, sum_y = 0, sum_radius = 0, avg_x = 0 ,avg_y = 0 ,avg_radius = 0;
	static int  centerX[11], centerY[11], centerRadius[11];
	int i;
	Mat gray;
	cvtColor(img, gray, COLOR_BGR2GRAY);

	Mat contours;
	Canny(gray, contours, 150, 20); //邊緣檢測
	threshold(contours, contours, 128, 255, THRESH_OTSU); //直接閥值化操作

	//降噪,去除雜訊  
	//GaussianBlur(gray, gray, Size(9, 9), 2, 2);

	vector<Vec3f> circles;
	//霍夫變換,檢測圓型  
	/*
		HoughCirclesº¯Ê½µÄÔ­ÐÍžé£º
		void HoughCircles(InputArray image,OutputArray circles, int method, double dp, double minDist, double param1=100, double param2=100, int minRadius=0,int maxRadius=0 )
		imagežéÝ”ÈëÓ°Ïó£¬ÒªÇóÊÇ»Ò¶ÈÓ°Ïó
		circlesžéÝ”³öˆAÏòÁ¿£¬Ã¿‚€ÏòÁ¿°üÀ¨Èý‚€¸¡ücÐÍµÄÔªËØ¡ª¡ªˆAÐÄ™M×ù˜Ë£¬ˆAÐÄ¿v×ù˜ËºÍˆA°ë½
		methodžéÊ¹ÓÃ»ô·ò×ƒ“QˆA™zœyµÄÑÝËã·¨£¬Opencv2.4.9Ö»Œ¬FÁË2-1»ô·ò×ƒ“Q£¬ËüµÄÒý”µÊÇCV_HOUGH_GRADIENT
		dpžéµÚÒ»ëA¶ÎËùÊ¹ÓÃµÄ»ô·ò¿ÕégµÄ½âÎö¶È£¬dp=1•r±íÊ¾»ô·ò¿ÕégÅcÝ”ÈëÓ°Ïó¿ÕégµÄ´óÐ¡Ò»ÖÂ£¬dp=2•r»ô·ò¿ÕégÊÇÝ”ÈëÓ°Ïó¿ÕégµÄÒ»°ë£¬ÒÔ´ËîÍÆ
		minDistžéˆAÐÄÖ®égµÄ×îÐ¡¾àëx£¬Èç¹û™zœyµ½µÄƒÉ‚€ˆAÐÄÖ®ég¾àëxÐ¡ì¶Ô“Öµ£¬„tÕJžéËü‚ƒÊÇÍ¬Ò»‚€ˆAÐÄ
		param1¡¢param2žéé“Öµ
		minRadiusºÍmaxRadiusžéËù™zœyµ½µÄˆA°ë½µÄ×îÐ¡ÖµºÍ×î´óÖµ
	*/
	HoughCircles(contours, circles, HOUGH_GRADIENT, 2, 200, 150, 70, 480, 540);

	//依次在圖中繪製出圓  
	//for (size_t i = 0; i < circles.size(); i++)
	if(circles.size() == 1)
	{
		//引數定義  
		Point center(round(circles[0][0]), round(circles[0][1]));
		int radius = round(circles[0][2]);

		if (*center_counter == -1)
		{
			*center_counter = sum_x = sum_y = sum_radius = avg_x = avg_y = avg_radius = 0;
		}
		else if (*center_counter == 0)
		{
			sum_x = sum_y = sum_radius = 0;
		}

		*center_counter = *center_counter + 1;

		centerX[*center_counter] = center.x;
		centerY[*center_counter] = center.y;
		centerRadius[*center_counter] = radius;
		
		if (*center_counter == 10)
		{
			//sort
			quickSort(centerX, 1, 10); //快速排序
			quickSort(centerY, 1, 10);
			quickSort(centerRadius, 1, 10);
			//remove the largest 2 and smallest 2 
			for (i = 3 ;i <= 8; i++)
			{
				sum_x += centerX[i];
				sum_y += centerY[i];
				sum_radius += centerRadius[i];
			}

			*center_x = avg_x = sum_x / 6;
			*center_y = avg_y = sum_y / 6;
			avg_radius = sum_radius / 6;
			*center_counter = sum_x = sum_y = sum_radius = 0;
		}
		if(avg_x > 0 && avg_y >0 && avg_radius>0)
		{ 
			if (avg_y + avg_radius > 1080 || avg_y - avg_radius < 0)
			{
				//畫中心十字線
				line(img, Point(avg_x-30, avg_y), Point(avg_x+30, avg_y), Scalar(0, 0, 255), 2);
				line(img, Point(avg_x, avg_y-30), Point(avg_x, avg_y+30), Scalar(0, 0, 255), 2);
				//circle(img, Point(avg_x, avg_y), 3, Scalar(0, 0, 255), -1, 4, 0);
				//畫圓
				circle(img, Point(avg_x, avg_y), avg_radius, Scalar(0, 0, 255), 2, 4, 0);
			}
			else
			{
				//畫中心十字線  
				line(img, Point(avg_x - 30, avg_y), Point(avg_x + 30, avg_y), Scalar(0, 255, 0), 2);
				line(img, Point(avg_x, avg_y - 30), Point(avg_x, avg_y + 30), Scalar(0, 255, 0), 2);
				//circle(img, Point(avg_x, avg_y), 3, Scalar(0, 255, 0), -1, 4, 0);
				//畫圓
				circle(img, Point(avg_x, avg_y), avg_radius, Scalar(0, 255, 0), 2, 4, 0);
			}
		}
	}
	else
	{
		*center_counter = sum_x = sum_y = sum_radius = avg_x = avg_y = avg_radius = *center_x = *center_y = 0;
	}

	//namedWindow("Circle", WINDOW_AUTOSIZE);
	//imshow("Circle", contours);
	//waitKey(0);

	return 0;
}

void COpencvLaplacian::DebugLog(char *pData, int nLen, bool bHex)
{
	CFile cf;
	int i;
	char szData[42000];
	char szTemp[16];
	memset(szData, 0x0, sizeof(szData));
	memset(szTemp, 0x0, sizeof(szTemp));

	int iFolderLength;
	TCHAR szPath[MAX_PATH] = { 0 };
	CString strPath, strToken;
	GetModuleFileName(NULL, szPath, MAX_PATH);
	strPath = szPath;
	iFolderLength = strPath.ReverseFind('\\');
	strToken.Format(_T("%s\\Log.txt"), strPath.Left(iFolderLength));

	//if (cf.Open(L"C:\\test\\PCBLog.txt", CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite, NULL))
	if (cf.Open(strToken, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite, NULL))
	{
		if (bHex)
		{
			for (i = 0; i < nLen; i++)
			{
				sprintf(szTemp, "%.2X ", *(pData + i) & 0x000000FF);
				strcat(szData, szTemp);
			}
		}
		else
			sprintf(szData, "%s", pData);
		cf.SeekToEnd();
		cf.Write(szData, strlen(szData));
		cf.Close();
	}
}
