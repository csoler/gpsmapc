#include <math.h>

#include "opencv_nonfree/xfeatures2d.hpp"
#include "opencv_nonfree/nonfree.hpp"
#include "opencv_nonfree/surf.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "MaxHeap.h"
#include "MapRegistration.h"

float MapRegistration::interpolated_image_intensity(const unsigned char *data,int W,int H,float i,float j)
{
    int I = (int)floor(i) ;
    int J = (int)floor(j) ;

    float di = i - I;
    float dj = j - J;

    int index = i+W*j ;

    float d_00 = 0.30 * data[3*(index+0+0) + 0] + 0.59 * data[3*(index+0+0) + 1] + 0.11 * data[3*(index+0+0)+2] ;
    float d_10 = 0.30 * data[3*(index+1+0) + 0] + 0.59 * data[3*(index+1+0) + 1] + 0.11 * data[3*(index+1+0)+2] ;
    float d_11 = 0.30 * data[3*(index+1+W) + 0] + 0.59 * data[3*(index+1+W) + 1] + 0.11 * data[3*(index+1+W)+2] ;
    float d_01 = 0.30 * data[3*(index+0+W) + 0] + 0.59 * data[3*(index+0+W) + 1] + 0.11 * data[3*(index+0+W)+2] ;

    return ((1-di)*((1-dj)*d_00 + dj*d_01) + di*((1-dj)*d_10 + dj*d_11))/255.0 ;
}

#ifdef TO_REMOVE
bool  MapRegistration::computeDescriptor1(const unsigned char *data,int W,int H,int i,int j,ImageDescriptor& descriptor)
{
	static const int max_size = 50 ; // size in pixels
	static const int nb_pts   = 100 ; // size in pixels

    if(i < max_size || i >= W-1-max_size || j < max_size || j >= H-1-max_size)
        return false ;

    descriptor.pixel_radius = 50;
    descriptor.x = i ;
    descriptor.y = j ;
    descriptor.data.clear();
    descriptor.data.resize(DESCRIPTOR_SIZE_1,0.0);

#pragma omp parallel for
    for(int k=0;k<DESCRIPTOR_SIZE_1;++k)
    {
        float res = 0.0;
        float radius = max_size*(k+1)/(float)DESCRIPTOR_SIZE_1;

        for(int l=0;l<nb_pts;++l)
            res += cos(2*M_PI * (k+1) * l/(float)nb_pts) * interpolated_image_intensity(data,W,H, i+radius*cos(2*M_PI*l/(float)nb_pts),j+radius*sin(2*M_PI*l/(float)nb_pts)) ;

        descriptor.data[k] = res ;
    }
    descriptor.variance = 0.0 ;

    for(int k=0;k<DESCRIPTOR_SIZE_1;++k)
        descriptor.variance += descriptor.data[k]*descriptor.data[k];

    return true;
}
bool  MapRegistration::computeDescriptor2(const unsigned char *data,int W,int H,int i,int j,ImageDescriptor& descriptor)
{
	static const int max_size = 50 ; // size in pixels
	static const int nb_pts   = 100 ; // size in pixels

    if(i < max_size || i >= W-1-max_size || j < max_size || j >= H-1-max_size)
        return false ;

    descriptor.pixel_radius = 20;
    descriptor.x = i ;
    descriptor.y = j ;
    descriptor.data.clear();
    descriptor.data.resize(DESCRIPTOR_SIZE_2,0.0);

    for(int k=-20;k<20;k+=2)
		for(int l=-20;l<20;l+=2)
		{
            float R = 0.30*data[3*( (i+k) + W*(j+l) )+0]/255.0 ;
            float G = 0.59*data[3*( (i+k) + W*(j+l) )+1]/255.0 ;
            float B = 0.11*data[3*( (i+k) + W*(j+l) )+2]/255.0 ;
            int n=0;
            float ck = 1.0/(1.0+k*k/20.0) ;
            float cl = 1.0/(1.0+l*l/20.0) ;

            for(int a=0;a<3;++a)
				for(int b=0;b<3;++b)
					for(int c=0;c<3;++c,++n)
                        descriptor.data[n] += k*l*pow(R,a)*pow(G,b)*pow(B,c) * ck*cl;
		}
    descriptor.variance = 0.0 ;

    for(int k=0;k<DESCRIPTOR_SIZE_2;++k)
        descriptor.variance += descriptor.data[k]*descriptor.data[k];

    return true;
}
bool  MapRegistration::computeDescriptor3(const unsigned char *data,int W,int H,int i,int j,ImageDescriptor& descriptor)
{
	static const int max_size = 100 ; // size in pixels

    if(i < max_size || i >= W-1-max_size || j < max_size || j >= H-1-max_size)
        return false ;

    descriptor.pixel_radius = 60;
    descriptor.x = i ;
    descriptor.y = j ;
    descriptor.data.clear();
    descriptor.data.resize(DESCRIPTOR_SIZE_3,0.0);

    int square_size = 20;
    int p=0;

    for(int k=-3;k<4;k++)
		for(int l=-3;l<4;l++,++p)
		{
            for(int m=0;m<square_size;++m)
				for(int n=0;n<square_size;++n)
                {

					float R = 0.30*data[3*( (i+k*square_size+m) + W*(j+l*square_size+n) )+0]/255.0 ;
					float G = 0.30*data[3*( (i+k*square_size+m) + W*(j+l*square_size+n) )+1]/255.0 ;
					float B = 0.30*data[3*( (i+k*square_size+m) + W*(j+l*square_size+n) )+2]/255.0 ;

                    descriptor.data[p] += pow(R-G,3)+2*(G-B) + pow(B-R,5) ;
                }
        }

    descriptor.variance = 0.0 ;

    for(int k=0;k<DESCRIPTOR_SIZE_3;++k)
        descriptor.variance += descriptor.data[k]*descriptor.data[k];

    return true;
}

void  MapRegistration::findDescriptors(const unsigned char *data,int W,int H,std::vector<ImageDescriptor>& descriptors)
{
    MaxHeap<ImageDescriptor> descriptor_queue(30);
    ImageDescriptor desc;

	for(int i=0;i<W;i+=4)
    {
        fprintf(stderr,"%2.2f %% completed\r",i/(float)W*100) ;
        fflush(stderr);

        for(int j=0;j<H;j+=4)
            if(computeDescriptor3(data,W,H,i,j,desc))
                descriptor_queue.push(desc);
    }

   	descriptors.clear();

    for(int i=0;i<descriptor_queue.size();++i)
    {
        descriptors.push_back(descriptor_queue[i]);

        std::cerr << "Descriptor #" << i << ": variance=" << descriptor_queue[i].variance <<  " x=" << descriptor_queue[i].x << " y=" << descriptor_queue[i].y << std::endl;
    }
}
#endif

void  MapRegistration::findDescriptors(const std::string& image_filename,std::vector<MapRegistration::ImageDescriptor>& descriptors)
{
    cv::Mat img = cv::imread( image_filename.c_str(), CV_LOAD_IMAGE_GRAYSCALE );

	if( !img.data )
		throw std::runtime_error("Cannot reading image " + image_filename);

	//-- Step 1: Detect the keypoints using SURF Detector
    int minHessian = 30000;

    cv::xfeatures2d::SURF_Impl detector(minHessian,4,2,true,true);

    std::vector<cv::KeyPoint> keypoints;

	detector.detect( img, keypoints );

    descriptors.clear();

    for(uint32_t i=0;i<keypoints.size();++i)
    {
        MapRegistration::ImageDescriptor desc ;

        desc.x = keypoints[i].pt.x ;
        desc.y = keypoints[i].pt.y ;
        desc.pixel_radius = keypoints[i].size/2.0;
        desc.variance = keypoints[i].response;

        descriptors.push_back(desc);
    }
}

void MapRegistration::computeRelativeTransform(const std::string& image_filename1,const std::string& image_filename2,float& dx,float& dy)
{
	cv::Mat img1 = cv::imread( image_filename1.c_str(), CV_LOAD_IMAGE_GRAYSCALE );
	if( !img1.data ) throw std::runtime_error("Cannot reading image " + image_filename1);

	cv::Mat img2 = cv::imread( image_filename2.c_str(), CV_LOAD_IMAGE_GRAYSCALE );
	if( !img2.data ) throw std::runtime_error("Cannot reading image " + image_filename2);

    int minHessian = 30000;

    cv::xfeatures2d::SURF_Impl detector(minHessian,4,2,true,true);

    std::vector<cv::KeyPoint> keypoints1,keypoints2;
    cv::Mat descriptors_1,descriptors_2;

	detector.detectAndCompute( img1, cv::Mat(), keypoints1, descriptors_1 );
	detector.detectAndCompute( img2, cv::Mat(), keypoints2, descriptors_2 );

    //-- Step 2: Matching descriptor vectors using FLANN matcher
    cv::FlannBasedMatcher matcher;
    std::vector<cv::DMatch> matches;

    matcher.match(descriptors_1, descriptors_2, matches);

    double max_dist = 0; double min_dist = 100;

    //-- Quick calculation of max and min distances between keypoints
    for( int i = 0; i < descriptors_1.rows; i++ )
    {
        double dist = matches[i].distance;
		if( dist < min_dist ) min_dist = dist;
		if( dist > max_dist ) max_dist = dist;
	}
    printf("-- Max dist : %f \n", max_dist );
    printf("-- Min dist : %f \n", min_dist );

    //-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
    //-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
    //-- small)
    //-- PS.- radiusMatch can also be used here.

    std::vector<cv::Point2f> good_matches;

    for( int i = 0; i<descriptors_1.rows; i++ )
		if( matches[i].distance <= std::max(2*min_dist, 0.10) )
        {
            int i1 = matches[i].queryIdx ;
            int i2 = matches[i].trainIdx ;

            std::cerr << "Dist = " << matches[i].distance
                      << " pt1: " << keypoints1[i1].pt.x << ", " << keypoints1[i1].pt.y
                      << " pt2: " << keypoints2[i2].pt.x << ", " << keypoints2[i2].pt.y
                      << " translation: " << keypoints2[i2].pt.x  - keypoints1[i1].pt.x
                      << ", "             << keypoints2[i2].pt.y  - keypoints1[i1].pt.y
                      << std::endl;

			good_matches.push_back( cv::Point2f(keypoints2[i2].pt.x  - keypoints1[i1].pt.x, keypoints2[i2].pt.y  - keypoints1[i1].pt.y) );
        }

    std::cerr << "Found " << good_matches.size() << " good matches among " << matches.size() << std::endl;

    int clusterCount = 10;
    cv::Mat labels;
    int attempts = 5;
    cv::Mat centers;

    cv::kmeans(good_matches, clusterCount, labels, cv::TermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10000, 0.0001), attempts, cv::KMEANS_PP_CENTERS, centers );

    std::cerr << "Centers found: " << centers.rows << std::endl;

	for(int i=0;i<centers.rows;++i)
    {
        std::cerr << "[" ;
        for(int j=0;j<centers.cols;++j)
            std::cerr << centers.at<float>(i,j) << " " ;

        std::cerr << "]" << std::endl;
    }

    dx = centers.at<float>(0,0);
    dy = centers.at<float>(0,1);

//    std::cerr << "Labels found: " << labels.rows << std::endl;
//
//	for(int i=0;i<labels.rows;++i)
//    {
//        std::cerr << "[" ;
//        for(int j=0;j<labels.cols;++j)
//            std::cerr << labels.at<float>(i,j) << " " ;
//
//        std::cerr << "]" << std::endl;
//    }

}





