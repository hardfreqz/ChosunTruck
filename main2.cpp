#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
//#include <opencv2/imageproc/imageproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/photo/cuda.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/cudaimgproc.hpp>
//#include <opencv2/cudafilters.hpp>
//#include <opencv2/gpu/gpu.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include "ets2_self_driving.h"
#include "IPM.h"
#include "getScreen_linux.cpp"
#include "uinput.h"



#define PI 3.1415926

using namespace cv;
using namespace std;

void Thinning(Mat input, int row, int col);

int main() {


	int counter = 0;
	//cudaf();


	long long int sum = 0;
	long long int i = 0;

	while (true) {
		auto begin = chrono::high_resolution_clock::now();
		// ETS2
		Mat image, outputImg;
		int Width = 0;
		int Height = 0;
		int Bpp = 0;
		std::vector<std::uint8_t> Pixels;

		ImageFromDisplay(Pixels, Width, Height, Bpp);

		Mat img = Mat(Height, Width, Bpp > 24 ? CV_8UC4 : CV_8UC3, &Pixels[0]); //Mat(Size(Height, Width), Bpp > 24 ? CV_8UC4 : CV_8UC3, &Pixels[0]); 
		cv::Rect myROI(896, 312, 1024, 768);
		image = img(myROI);


		// Mat to GpuMat
		//cuda::GpuMat imageGPU;
		//imageGPU.upload(image);

		medianBlur(image, image, 3); 
		//cv::cuda::bilateralFilter(imageGPU, imageGPU, );

		int width = 1024, height = 768;

		// The 4-points at the input image	
		vector<Point2f> origPoints;
		
		origPoints.push_back(Point2f(0, (height-50)));
		origPoints.push_back(Point2f(width, height-50));
		origPoints.push_back(Point2f(width/2+125, height/2+30));
		origPoints.push_back(Point2f(width/2-125, height/2+30));
		

		// The 4-points correspondences in the destination image
		vector<Point2f> dstPoints;
		dstPoints.push_back(Point2f(0, height));
		dstPoints.push_back(Point2f(width, height));
		dstPoints.push_back(Point2f(width, 0));
		dstPoints.push_back(Point2f(0, 0));

		// IPM object
		IPM ipm(Size(width, height), Size(width, height), origPoints, dstPoints);

		// Process
		//clock_t begin = clock();
		ipm.applyHomography(image, outputImg);
		//clock_t end = clock();
		//double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
		//printf("%.2f (ms)\r", 1000 * elapsed_secs);
		//ipm.drawPoints(origPoints, image);

		//Mat row = outputImg.row[0];
		cv::Mat gray;
		cv::Mat blur;
		cv::Mat sobel;
		cv::Mat contours;
		cv::resize(outputImg, outputImg, cv::Size(320, 240));
		cv::cvtColor(outputImg, gray, COLOR_RGB2GRAY);
		cv::blur(gray, blur, cv::Size(10, 10));
		cv::Sobel(blur, sobel, blur.depth(), 1, 0, 3, 0.5, 127);
		cv::threshold(sobel, contours, 145, 255, CV_THRESH_BINARY);
		//Thinning(contours, contours.rows, contours.cols);
		//cv::Canny(gray, contours, 125, 350);
		
		LineFinder ld; // 인스턴스 생성

		// 확률적 허프변환 파라미터 설정하기
		
		ld.setLineLengthAndGap(20, 120);
		ld.setMinVote(55);

		std::vector<cv::Vec4i> li = ld.findLines(contours);
		ld.drawDetectedLines(contours);


		////////////////////////////////////////////
		// 자율 주행 

		int bottom_center = 160;
		int sum_centerline = 0;
		int count_centerline = 0;

		for(int i=240; i>30; i--){
			double center_to_right = -1;
			double center_to_left = -1;
			for (int j=0;j<150;j++) {
				if (contours.at<unsigned char>(i, bottom_center+j) == 112 && center_to_right == -1) {
					center_to_right = j;
				}
				if (contours.at<unsigned char>(i, bottom_center-j) == 112 && center_to_left == -1) {
					center_to_left = j;
				}
			}
			if(center_to_left!=-1 && center_to_right!=-1){
				int centerline = (center_to_right - center_to_left +2*bottom_center)/2;
				cv::circle(contours, Point(centerline, i), 1, Scalar(50, 50, 50) , 3);
				sum_centerline += centerline;
				count_centerline++;
			}else{
				//Detection d = {center_to_left, center_to_right, bottom_center, i};
				//data.push_back(d);
			}
		}

		// 컨트롤러 입력
		int diff = 0;
		if (count_centerline!=0) {
		diff = sum_centerline/count_centerline - bottom_center;
	
		int move_mouse_pixel = diff/3;

		if (abs(move_mouse_pixel)< 5) {
			goDirection(0 - counter/3);
			counter -= counter/3;
		} else if (abs(move_mouse_pixel) < 4) {
			goDirection(0 - counter/2);
			counter -= counter/2;
		} else if (abs (move_mouse_pixel) < 2) {
			goDirection(0 - counter);
			counter = 0;
		} else {
			goDirection(move_mouse_pixel);
			counter += move_mouse_pixel;
		}
		cout << diff << ", " << move_mouse_pixel << ", "<< counter <<endl;
		} else {}
		////////////////////////////////////////////
		
		//cv::cvtColor(contours, contours, COLOR_GRAY2RGB);
		imshow("Test", contours);
		waitKey(1);
		/*
		auto end = chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		ms++;
		sum += ms;
		cout << 1000 / ms << "fps       avr:" << 1000 / (sum / (++i)) << endl;
		*/
	}
	return 0;
}
