#pragma once

#include <QImage>
#include "MapDB.h"

class MapAccessor
{
	public:
		MapAccessor(MapDB& mdb) : mDb(mdb) {}

        struct ImageData
        {
          	int W,H;
            float lon_width ;
            MapDB::GPSCoord top_left_corner ;
            const unsigned char *pixel_data;
            std::vector<MapRegistration::ImageDescriptor> descriptors;
            QString directory;
            QString filename;
        };

        const MapDB::GPSCoord& topLeftCorner() const { return mDb.topLeftCorner() ; }
        const MapDB::GPSCoord& bottomRightCorner() const { return mDb.bottomRightCorner() ; }

        void getImagesToDraw(MapDB::GPSCoord& mBottomLeftViewCorner,const MapDB::GPSCoord& mTopRightViewCorner, std::vector<MapAccessor::ImageData>& images_to_draw) const;
        QImage getImageData(const QString& image_filename);
		bool getImageParams(const QString& image_filename, MapDB::RegisteredImage &img);

		void generateImage(const MapDB::GPSCoord& top_left,const MapDB::GPSCoord& bottom_right,float scale,MapDB::RegisteredImage& img_out,unsigned char *& pixels_out) ;

		void moveImage(const QString& image_filename,float delta_lon,float delta_lat);
		void placeImage(const QString& image_filename,const MapDB::GPSCoord& new_corner);

		void recomputeDescriptors(const QString& image_filename);
		QString fullPath(const QString& image_filename);

		void saveMap();
	private:
		const unsigned char *getPixelData(const QString& filename) const;

		MapDB& mDb;
        mutable std::map<QString,QImage> mImageCache ;
};

