#pragma once

#include "MapDB.h"

class MapAccessor
{
	public:
		MapAccessor(const MapDB& mdb) : mDb(mdb) {}

        struct ImageData
        {
          	int W,H;
            float lon_width ;
            MapDB::GPSCoord top_left_corner ;
            const unsigned char *pixel_data;
            QString filename;
        };

        void getImagesToDraw(const MapDB::GPSCoord& mBottomLeftViewCorner,const MapDB::GPSCoord& mTopRightViewCorner, std::vector<MapAccessor::ImageData>& images_to_draw) const;

		void generateImage(const MapDB::GPSCoord& top_left,const MapDB::GPSCoord& bottom_right,float scale,MapDB::RegisteredImage& img_out,unsigned char *& pixels_out) ;

	private:
		const unsigned char *getPixelData(const QString& filename) const;

		const MapDB& mDb;
        mutable std::map<QString,QImage> mImageCache ;
};

