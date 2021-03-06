//the following code is extracted from
//http://www.cvchina.info/2011/09/25/orb-test/
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgproc/imgproc.hpp"

#if defined _DEBUG
#pragma comment(lib,"opencv_core231d.lib")
#pragma comment(lib,"opencv_imgproc231d.lib")
#pragma comment(lib,"opencv_highgui231d.lib")
#pragma comment(lib,"opencv_features2d231d.lib")
#pragma comment(lib,"opencv_calib3d231d.lib")
#else
#pragma comment(lib,"opencv_core231.lib")
#pragma comment(lib,"opencv_imgproc231.lib")
#pragma comment(lib,"opencv_highgui231.lib")
#pragma comment(lib,"opencv_features2d231.lib")
#pragma comment(lib,"opencv_calib3d231.lib")
#endif

#include <vector> 

using namespace std;
using namespace cv;

char* image_filename1 = "apple_vinegar_1.png";
char* image_filename2 = "apple_vinegar_2.png";

unsigned int hamdist(unsigned int x, unsigned int y)
{
	unsigned int dist = 0, val = x ^ y;

	// Count the number of set bits

	while(val)
	{
		++dist;
		val &= val - 1;
	}

	return dist;
}

unsigned int hamdist2(unsigned char* a, unsigned char* b, size_t size)
{
	HammingLUT lut;

	unsigned int result;
	result = lut((a), (b), size);
	return result;
}

void naive_nn_search(vector<KeyPoint>& keys1, Mat& descp1,
					  vector<KeyPoint>& keys2, Mat& descp2,
					  vector<DMatch>& matches)
{
	for( int i = 0; i < (int)keys2.size(); i++){
		unsigned int min_dist = INT_MAX;
		int min_idx = -1;
		unsigned char* query_feat = descp2.ptr(i);
		for( int j = 0; j < (int)keys1.size(); j++){
			unsigned char* train_feat = descp1.ptr(j);
			unsigned int dist = hamdist2(query_feat, train_feat, 32);

			if(dist < min_dist){
				min_dist = dist;
				min_idx = j;
			}
		}

		//if(min_dist <= (unsigned int)(second_dist * 0.8)){

		if(min_dist <= 50){
			matches.push_back(DMatch(i, min_idx, 0, (float)min_dist));
		}
	}
}

void naive_nn_search2(vector<KeyPoint>& keys1, Mat& descp1,
					  vector<KeyPoint>& keys2, Mat& descp2,
					  vector<DMatch>& matches)
{
	for( int i = 0; i < (int)keys2.size(); i++){
		unsigned int min_dist = INT_MAX;
		unsigned int sec_dist = INT_MAX;
		int min_idx = -1, sec_idx = -1;
		unsigned char* query_feat = descp2.ptr(i);
		for( int j = 0; j < (int)keys1.size(); j++){
			unsigned char* train_feat = descp1.ptr(j);
			unsigned int dist = hamdist2(query_feat, train_feat, 32);

			if(dist < min_dist){
				sec_dist = min_dist;
				sec_idx = min_idx;
				min_dist = dist;
				min_idx = j;
			}else if(dist < sec_dist){
				sec_dist = dist;
				sec_idx = j;
			}
		}

		if(min_dist <= (unsigned int)(sec_dist * 0.8) && min_dist <=50){
			//if(min_dist <= 50){

			matches.push_back(DMatch(i, min_idx, 0, (float)min_dist));
		}
	}
}

int main(int argc, char* argv[])
{
	Mat img1 = imread(image_filename1, 0);
	Mat img2 = imread(image_filename2, 0);

	if (img1.empty() || img2.empty())
		return 1;
	//GaussianBlur(img1, img1, Size(5, 5), 0);

	//GaussianBlur(img2, img2, Size(5, 5), 0);

	ORB orb1(3000, ORB::CommonParams(1.2, 8));
	ORB orb2(100, ORB::CommonParams(1.2, 1));

	vector<KeyPoint> keys1, keys2;
	Mat descriptors1, descriptors2;

	orb1(img1, Mat(), keys1, descriptors1, false);
	printf("tem feat num: %d\n", keys1.size());

	int64 st, et;
	st = cvGetTickCount();
	orb2(img2, Mat(), keys2, descriptors2, false);
	et = cvGetTickCount();
	printf("orb2 extraction time: %f\n", (et-st)/(double)cvGetTickFrequency()/1000.);
	printf("query feat num: %d\n", keys2.size());

	// find matches

	vector<DMatch> matches;

	st = cvGetTickCount();
	//for(int i = 0; i < 10; i++){

	naive_nn_search2(keys1, descriptors1, keys2, descriptors2, matches);
	//}

	et = cvGetTickCount();

	printf("match time: %f\n", (et-st)/(double)cvGetTickFrequency()/1000.);
	printf("matchs num: %d\n", matches.size());

	Mat showImg;
	drawMatches(img2, keys2, img1, keys1, matches, showImg, CV_RGB(0, 255, 0), CV_RGB(0, 0, 255));
	string winName = "Matches";
	namedWindow( winName, WINDOW_AUTOSIZE );
	imshow( winName, showImg );
	waitKey();

	vector<Point2f>	pt1,pt2;

	for(int i = 0; i < (int)matches.size(); i++){
		pt1.push_back(Point2f(keys2[matches[i].queryIdx].pt.x, keys2[matches[i].queryIdx].pt.y));

		pt2.push_back(Point2f(keys1[matches[i].trainIdx].pt.x, keys1[matches[i].trainIdx].pt.y));
	}

	Mat homo;

	st = cvGetTickCount();
	homo = findHomography(pt1, pt2, Mat(), CV_RANSAC, 5);
	et = cvGetTickCount();
	printf("ransac time: %f\n", (et-st)/(double)cvGetTickFrequency()/1000.);

	printf("homo\n"
		"%f %f %f\n"
		"%f %f %f\n"
		"%f %f %f\n",
		homo.at<double>(0,0), homo.at<double>(0,1), homo.at<double>(0,2),
		homo.at<double>(1,0), homo.at<double>(1,1), homo.at<double>(1,2),
		homo.at<double>(2,0),homo.at<double>(2,1),homo.at<double>(2,2));

	vector<Point2f>	reproj;
	reproj.resize(pt1.size());

	perspectiveTransform(pt1, reproj, homo);

	Mat diff;
	diff = Mat(reproj) - Mat(pt2);

	int inlier = 0;
	double err_sum = 0;
	for(int i = 0; i < diff.rows; i++){
		float* ptr = diff.ptr<float>(i);
		float err = ptr[0]*ptr[0] + ptr[1]*ptr[1];
		if(err < 25.f){
			inlier++;
			err_sum += sqrt(err);
		}
	}
	printf("inlier num: %d\n", inlier);
	printf("ratio %f\n", inlier / (float)(diff.rows));
	printf("mean reprojection error: %f\n", err_sum / inlier);

	return 0;
}