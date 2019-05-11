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

bool MapRegistration::computeRelativeTransform(const std::string& image_filename1,const std::string& image_filename2,float& dx,float& dy)
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

			good_matches.push_back( cv::Point2f(keypoints2[i2].pt.x  - keypoints1[i1].pt.x, keypoints2[i2].pt.y  - keypoints1[i1].pt.y) );
        }

    // Now perform k-means clustering to find the transformation clusters.

    std::cerr << "Found " << good_matches.size() << " good matches among " << matches.size() << std::endl;

    int clusterCount = 3;
    cv::Mat labels;
    int attempts = 5;
    cv::Mat centers;

    cv::kmeans(good_matches, clusterCount, labels, cv::TermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10000, 0.01), attempts, cv::KMEANS_PP_CENTERS, centers );

    std::cerr << "Labels found: " << labels.rows << std::endl;

	for(int i=0;i<labels.rows;++i)
        std::cerr << "[" << labels.at<int>(i,0) << std::endl;

    // Look for which label gets the more votes.

    int best_candidate=0;
    std::vector<int> votes(clusterCount,0);

    for(int i=0;i<(int)labels.rows;++i)
        ++votes[labels.at<int>(i,0)];

    int max_votes = 0;

    std::cerr << "Centers found: " << centers.rows << std::endl;

    for(int i=0;i<votes.size();++i)
    {
        if(max_votes < votes[i])
        {
            max_votes = votes[i] ;
            best_candidate = i ;
        }
        std::cerr << "Votes: " << votes[i] << " center " ;
		std::cerr << "[" ;
        for(int j=0;j<centers.cols;++j)
            std::cerr << centers.at<float>(i,j) << " " ;

        std::cerr << "]" << std::endl;
    }

 	for(uint32_t i=0;i<good_matches.size();++i)
		std::cerr << "Cluster " << labels.at<int>(i,0) << " translation: " << good_matches[i].x << ", " << good_matches[i].y << std::endl;

    std::cerr << "Best candidate: " << best_candidate << std::endl;

    dx = centers.at<float>(best_candidate,0);
    dy = centers.at<float>(best_candidate,1);

    // TODO: filter the cluster content to prune the worst elements.

    // Return the candidate

	if(fabs(dx) < 5.0 && fabs(dy) < 5.0)
		return false;

	return true;
}





