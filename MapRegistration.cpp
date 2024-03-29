#include <math.h>

#include "MapDB.h"

#include "opencv_nonfree/xfeatures2d.hpp"
#include "opencv_nonfree/nonfree.hpp"
#include "opencv_nonfree/surf.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "opencv2/imgproc/imgproc.hpp"

#include "MaxHeap.h"
#include "MapRegistration.h"

static const int MIN_HAESSIAN    = 35000;
static const int N_OCTAVES       = 8;
static const int N_OCTAVE_LAYERS = 4;

QColor MapRegistration::interpolated_image_color_BGR(const unsigned char *data,int W,int H,float i,float j)
{
    int I = (int)floor(i) ;
    int J = (int)floor(j) ;

    float di = i - I;
    float dj = j - J;

    int index = I+W*J ;

    int r = (1-di)*((1-dj)*data[3*(index+0+0) + 2] + dj*data[3*(index+0+W) + 2]) + di*((1-dj)*data[3*(index+1+0) + 2] + dj*data[3*(index+1+W) + 2]);
    int g = (1-di)*((1-dj)*data[3*(index+0+0) + 1] + dj*data[3*(index+0+W) + 1]) + di*((1-dj)*data[3*(index+1+0) + 1] + dj*data[3*(index+1+W) + 1]);
    int b = (1-di)*((1-dj)*data[3*(index+0+0) + 0] + dj*data[3*(index+0+W) + 0]) + di*((1-dj)*data[3*(index+1+0) + 0] + dj*data[3*(index+1+W) + 0]);

    return QColor(r,g,b);
}
QColor MapRegistration::interpolated_image_color_ABGR(const unsigned char *data,int W,int H,float i,float j)
{
    int I = (int)floor(i) ;
    int J = (int)floor(j) ;

    float di = i - I;
    float dj = j - J;

    int index = I+W*J ;

    int r = (1-di)*((1-dj)*data[4*(index+0+0) + 2] + dj*data[4*(index+0+W) + 2]) + di*((1-dj)*data[4*(index+1+0) + 2] + dj*data[4*(index+1+W) + 2]);
    int g = (1-di)*((1-dj)*data[4*(index+0+0) + 1] + dj*data[4*(index+0+W) + 1]) + di*((1-dj)*data[4*(index+1+0) + 1] + dj*data[4*(index+1+W) + 1]);
    int b = (1-di)*((1-dj)*data[4*(index+0+0) + 0] + dj*data[4*(index+0+W) + 0]) + di*((1-dj)*data[4*(index+1+0) + 0] + dj*data[4*(index+1+W) + 0]);

    return QColor(r,g,b);
}

float MapRegistration::interpolated_image_intensity(const unsigned char *data,int W,int H,float i,float j)
{
    int I = (int)floor(i) ;
    int J = (int)floor(j) ;

    float di = i - I;
    float dj = j - J;

    int index = I+W*J ;

    float d_00 = 0.30 * data[4*(index+0+0) + 2] + 0.59 * data[4*(index+0+0) + 1] + 0.11 * data[4*(index+0+0)+0] ;
    float d_10 = 0.30 * data[4*(index+1+0) + 2] + 0.59 * data[4*(index+1+0) + 1] + 0.11 * data[4*(index+1+0)+0] ;
    float d_11 = 0.30 * data[4*(index+1+W) + 2] + 0.59 * data[4*(index+1+W) + 1] + 0.11 * data[4*(index+1+W)+0] ;
    float d_01 = 0.30 * data[4*(index+0+W) + 2] + 0.59 * data[4*(index+0+W) + 1] + 0.11 * data[4*(index+0+W)+0] ;

    return ((1-di)*((1-dj)*d_00 + dj*d_01) + di*((1-dj)*d_10 + dj*d_11))/255.0 ;
}

void  MapRegistration::findDescriptors(const std::string& image_filename,const QImage& mask,std::vector<MapRegistration::ImageDescriptor>& descriptors)
{
    cv::Mat img = cv::imread( image_filename.c_str(), cv::IMREAD_GRAYSCALE );

	if( !img.data )
		throw std::runtime_error("Cannot reading image " + image_filename);

	//-- Step 1: Detect the keypoints using SURF Detector
    int minHessian = MIN_HAESSIAN;

    cv::xfeatures2d::SURF_Impl detector(minHessian,N_OCTAVES,N_OCTAVE_LAYERS,true,true);

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

        if(mask.width() == 0 || mask.height() == 0 || mask.pixel(desc.x,desc.y) != 0)
			descriptors.push_back(desc);
    }
}

static bool bruteForceCheckMatchConsistency(const QImage& mask,const std::string& image_filename1,const std::string& image_filename2,double delta_x,double delta_y,bool verbose=false)
{
    cv::Mat tmp = cv::imread( image_filename1.c_str(), cv::IMREAD_COLOR);
    if( !tmp.data ) throw std::runtime_error("Cannot reading image " + image_filename1);

    cv::Mat img1;
    cv::GaussianBlur( tmp, img1, cv::Size( 11, 11), 0, 0 );//applying Gaussian filter
    //img1 = tmp;

    tmp = cv::imread( image_filename2.c_str(), cv::IMREAD_COLOR);
    if( !tmp.data ) throw std::runtime_error("Cannot reading image " + image_filename2);

    cv::Mat img2;
    cv::GaussianBlur( tmp, img2, cv::Size( 11, 11), 0, 0 );//applying Gaussian filter
    //img2 = tmp;

    int W1 = img1.size[1];
    int H1 = img1.size[0];
    int W2 = img2.size[1];
    int H2 = img2.size[0];

    if(verbose)
        std::cerr << "Checking match between image " << image_filename1 << " (" << W1 << " x " << H1 <<
                     ") and " << image_filename2 << " (" << W2 << " x " << H2 << ") dx=" << delta_x << ", dy=" << delta_y << std::endl;

    int common_region_size=0;
    int matching_pixels=0;

    for(int i=0;i<W1;++i)
        for(int j=0;j<H1;++j)
        {
            float x1 = i;
            float y1 = j;

            float x2 = i-delta_x;
            float y2 = j-delta_y;

            if(x2 >= 0.0 && x2 < W2 && y2 >= 0.0 && y2 < H2 && (  (mask.width() == 0 || mask.height() == 0) || (mask.pixel(x1,y1) != 0 && mask.pixel(x2,y2) != 0)) )
            {
                common_region_size++;

                QColor c1 = MapRegistration::interpolated_image_color_BGR(img1.data,W1,H1,x1,y1);
                QColor c2 = MapRegistration::interpolated_image_color_BGR(img2.data,W2,H2,x2,y2);

                double dist = sqrt(pow(c1.redF() - c2.redF(),2) + pow(c1.greenF() - c2.greenF(),2) + pow(c1.blueF() - c2.blueF(),2));

                if(verbose)
                {
                    std::cerr << "Comparing pixel (" << x1 << "," << y1 << ") color ( " << c1.redF() << "," << c1.greenF() << "," << c1.blueF() << ") of " << image_filename1 <<
                                 " and pixel (" << x2 << "," << y2 << ") color ( " << c2.redF() << "," << c2.greenF() << "," << c2.blueF() << ") of " << image_filename2 ;
                    std::cerr << " dist = " << dist << std::endl;
                }
                if(dist < 0.02)
                    ++matching_pixels;
            }
        }

    std::cerr << " common: " << common_region_size << " matching: "<< matching_pixels ;

    if(common_region_size > 0.05*std::min(W1,H1)*std::min(W2,H2) && matching_pixels > 0.5*common_region_size)
        return true;
    else
        return false;
}

static bool computeTransform(const QImage& mask,const std::vector<cv::KeyPoint>& keypoints1,const std::vector<cv::KeyPoint>& keypoints2,const cv::Mat& descriptors_1,const cv::Mat& descriptors_2,float& dx,float& dy,bool verbose=false)
{
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

    if(verbose)
    {
        printf("-- Max dist : %f \n", max_dist );
        printf("-- Min dist : %f \n", min_dist );
    }

	//-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
	//-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
	//-- small)
	//-- PS.- radiusMatch can also be used here.

	std::vector<cv::Point2f> good_matches;

	for( int i = 0; i<descriptors_1.rows; i++ )
	{
		float delta_x,delta_y ;

		int i1 = matches[i].queryIdx ;
		int i2 = matches[i].trainIdx ;

		if( mask.width() !=0 && mask.height()!=0 && mask.pixel((int)keypoints1[i1].pt.x,(int)keypoints1[i1].pt.y)==0)
            continue;
		if( mask.width() !=0 && mask.height()!=0 && mask.pixel((int)keypoints2[i2].pt.x,(int)keypoints2[i2].pt.y)==0)
            continue;

		if( matches[i].distance <= std::max(2*min_dist, 0.10) )
			good_matches.push_back( cv::Point2f(keypoints2[i2].pt.x  - keypoints1[i1].pt.x, keypoints2[i2].pt.y  - keypoints1[i1].pt.y) );
	}

	// Now perform k-means clustering to find the transformation clusters.

    if(verbose)
        std::cerr << "Found " << good_matches.size() << " good matches among " << matches.size() << std::endl;

    if(good_matches.size() < 3)
        return false;

	int clusterCount = 3;
	cv::Mat labels;
	int attempts = 5;
	cv::Mat centers;

    cv::kmeans(good_matches, clusterCount, labels, cv::TermCriteria(cv::TermCriteria::EPS,10000, 0.01), attempts, cv::KMEANS_PP_CENTERS, centers );

    if(verbose)
	{
		std::cerr << "Labels found: " << labels.rows << std::endl;

		for(int i=0;i<labels.rows;++i)
			std::cerr << "[" << labels.at<int>(i,0) << std::endl;
	}

	// Look for which label gets the more votes.

	int best_candidate=0;
	std::vector<int> votes(clusterCount,0);

	for(int i=0;i<(int)labels.rows;++i)
		++votes[labels.at<int>(i,0)];

	int max_votes = 0;

    if(verbose)
		std::cerr << "Centers found: " << centers.rows << std::endl;

	for(int i=0;i<votes.size();++i)
	{
		if(max_votes < votes[i])
		{
			max_votes = votes[i] ;
			best_candidate = i ;
		}

        if(verbose)
		{
			std::cerr << "Votes: " << votes[i] << " center " ;
			std::cerr << "[" ;
			for(int j=0;j<centers.cols;++j)
				std::cerr << centers.at<float>(i,j) << " " ;

			std::cerr << "]" << std::endl;
		}
	}

    if(verbose)
	{
		for(uint32_t i=0;i<good_matches.size();++i)
			std::cerr << "Cluster " << labels.at<int>(i,0) << " translation: " << good_matches[i].x << ", " << good_matches[i].y << std::endl;

		std::cerr << "Best candidate: " << best_candidate << std::endl;
	}

	dx = centers.at<float>(best_candidate,0);
	dy = centers.at<float>(best_candidate,1);

	// TODO: filter the cluster content to prune the worst elements.

	// Return the candidate

	if(fabs(dx) < 5.0 && fabs(dy) < 5.0)
		return false;

	return true;
}

bool MapRegistration::computeRelativeTransform(const QImage& mask,const std::string& image_filename1,const std::string& image_filename2,float& dx,float& dy)
{
	cv::Mat img1 = cv::imread( image_filename1.c_str(), cv::IMREAD_GRAYSCALE);
	if( !img1.data ) throw std::runtime_error("Cannot reading image " + image_filename1);

	cv::Mat img2 = cv::imread( image_filename2.c_str(), cv::IMREAD_GRAYSCALE);
	if( !img2.data ) throw std::runtime_error("Cannot reading image " + image_filename2);

    int minHessian = MIN_HAESSIAN;

    cv::xfeatures2d::SURF_Impl detector(minHessian,N_OCTAVES,N_OCTAVE_LAYERS,true,true);

    std::vector<cv::KeyPoint> keypoints1,keypoints2;
    cv::Mat descriptors_1,descriptors_2;

	detector.detectAndCompute( img1, cv::Mat(), keypoints1, descriptors_1 );
	detector.detectAndCompute( img2, cv::Mat(), keypoints2, descriptors_2 );

    return computeTransform(mask,keypoints1,keypoints2,descriptors_1,descriptors_2,dx,dy,true);
}

bool MapRegistration::computeAllImagesPositions(const QImage& mask,const std::vector<std::string>& image_filenames,std::vector<std::pair<float,float> >& top_left_corners)
{
    if(image_filenames.empty())
        return false ;

    std::vector<std::vector<cv::KeyPoint> >keypoints(image_filenames.size());
    std::vector<cv::Mat> descriptors(image_filenames.size());

    // compute descriptors for all images

#pragma omp parallel for
    for(uint32_t i=0;i<image_filenames.size();++i)
    {
        std::cerr << "  computing keypoints for " << image_filenames[i] << std::endl;

		cv::Mat img = cv::imread( image_filenames[i].c_str(), cv::IMREAD_GRAYSCALE);

		if( !img.data ) throw std::runtime_error("Cannot reading image " + image_filenames[i]);

        int minHessian = MIN_HAESSIAN;
        cv::xfeatures2d::SURF_Impl detector(minHessian,N_OCTAVES,N_OCTAVE_LAYERS,true,true);

		detector.detectAndCompute( img, cv::Mat(), keypoints[i], descriptors[i] );
    }

    top_left_corners.clear();
    top_left_corners.resize(image_filenames.size(),std::make_pair(0.0,0.0));

#ifdef OLD_CODE
    // now go through each image and try to match it to at least one image with known position

    std::vector<bool> has_coords(image_filenames.size(),false);

    has_coords[0] = true;
    int n=0;

    while(true)
    {
        bool finished = true ;

        for(uint32_t i=1;i<image_filenames.size();++i)
            if(!has_coords[i])
			{
				// try to match to one of the previous images
                float delta_x,delta_y;

				for(uint32_t j=0;j<i;++j)
                {
                    std::cerr << "  testing " << i << " vs. " << j << std::endl;

					if(has_coords[j] && computeTransform(mask,keypoints[j],keypoints[i],descriptors[j],descriptors[i],delta_x,delta_y))
					{
                        std::cerr << "Found new coordinates for image " << i << " w.r.t. image " << j << ": delta=" << delta_x << ", " << delta_y << std::endl;
						top_left_corners[i] = std::make_pair(top_left_corners[j].first - delta_x, top_left_corners[j].second + delta_y);
						has_coords[i] = true ;
						break;
					}
                }

                if(!has_coords[i])
                    finished = false;
			}

        if(finished || ++n > 20)
            break;
    }
#endif
    // new global registration method:
    //	1 - compute image graph based on matches.

    struct NStruct
    {
        int j;
        float delta_x;
        float delta_y;
    };

    std::vector<std::list<NStruct> > neighbours(image_filenames.size());

    for(int i=0;i<(int)image_filenames.size();++i)
    {
        for(int j=i+1;j<(int)image_filenames.size();++j)
        {
            // try to match to one of the previous images
            float delta_x,delta_y;

            if(computeTransform(mask,keypoints[j],keypoints[i],descriptors[j],descriptors[i],delta_x,delta_y))
            {
                std::cerr << " Image " << i << " is neighbour to image " << j << ": delta=" << delta_x << ", " << delta_y ;
                std::cerr.flush();
                std::cerr << ". Checking consistency..." ;
                std::cerr.flush();

                //	2 - test consistency of translations between images: for each image pair, translate the files and check how much pixels actually match

                if(bruteForceCheckMatchConsistency(mask,image_filenames[i],image_filenames[j],delta_x,delta_y))
                {
                    std::cerr << " OK" << std::endl;

                    NStruct S;
                    S.j = j;
                    S.delta_x = delta_x;
                    S.delta_y = delta_y;

                    neighbours[i].push_back(S);

                    // This needs to be done both ways. Otherwise the graph is not bi-connected and some deadends may appear in the algorithm below.

                    S.j = i;
                    S.delta_x = -delta_x;
                    S.delta_y = -delta_y;

                    neighbours[j].push_back(S);
                }
                else
                    std::cerr << " REJECTED" << std::endl;
            }
        }
    }

    //	3 - test connexity, and compute connex components

    int next = 0;
    int nb_cnx_components = 0;
    std::vector<bool> has_coords(image_filenames.size(),false);
    float max_y = 0.0;

    while(next >= 0)
    {
        std::cerr << "Starting connex component " << nb_cnx_components << " at point " << next << std::endl;

        // Do the next connex component.

        has_coords[next] = true;
        top_left_corners[next] = std::make_pair(0,max_y + 50);
        std::list<int> to_do = { next };

        while(!to_do.empty())
        {
            int i = to_do.front();
            to_do.pop_front();

            std::cerr << "  popped i=" << i << " from the queue." << std::endl;

            for(auto& S: neighbours[i])
                if(!has_coords[S.j])
                {
                    top_left_corners[S.j] = std::make_pair(top_left_corners[i].first + S.delta_x, top_left_corners[i].second - S.delta_y);
                    has_coords[S.j] = true;
                    to_do.push_back(S.j);

                    max_y = std::max(max_y,top_left_corners[S.j].second - mask.height());

                    std::cerr << "    pushing j=" << S.j << " to the queue with coordinates " << top_left_corners[S.j].first << " , " << top_left_corners[S.j].second << std::endl;
                }
        }
        ++nb_cnx_components;

        next = -1;

        for(int i=0;i<(int)image_filenames.size();++i)
            if(!has_coords[i])
            {
                next=i;
                break;
            }
    }

    std::cerr << "Number of connex components: " << nb_cnx_components << std::endl;
    return true;
}






