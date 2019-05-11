#pragma once

#include <string.h>

class MapRegistration
{
public:
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

private:
	static float interpolated_image_intensity(const unsigned char *data, int W, int H, float i, float j);
};
