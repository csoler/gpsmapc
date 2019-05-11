#pragma once

#include <string.h>

class MapRegistration
{
public:
    static const int DESCRIPTOR_SIZE_1 = 20;
    static const int DESCRIPTOR_SIZE_2 = 27;
    static const int DESCRIPTOR_SIZE_3 = 49;

    struct ImageDescriptor
	{
		ImageDescriptor():x(0),y(0),variance(0.0),pixel_radius(0) {}

		int x,y; // central pixel coordinates
		std::vector<float> data;
		float variance;
        int pixel_radius ;

		bool operator<(const ImageDescriptor& d) const { return variance < d.variance ; }
	};

    static void findDescriptors(const std::string& image_filename, std::vector<MapRegistration::ImageDescriptor>& descriptors);
 	static bool computeRelativeTransform(const std::string& image_filename1, const std::string& image_filename2, float &dx, float &dy);

#ifdef TO_REMOVE
    static void findDescriptors(const unsigned char *data,int W,int H,std::vector<ImageDescriptor>& descriptors);
	static bool  computeDescriptor1(const unsigned char *data,int W,int H,int i,int j,ImageDescriptor& descriptor);
	static bool  computeDescriptor2(const unsigned char *data,int W,int H,int i,int j,ImageDescriptor& descriptor);
	static bool  computeDescriptor3(const unsigned char *data,int W,int H,int i,int j,ImageDescriptor& descriptor);
#endif

private:
	static float interpolated_image_intensity(const unsigned char *data, int W, int H, float i, float j);
};
