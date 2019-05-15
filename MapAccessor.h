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
            MapDB::ImageSpaceCoord bottom_left_corner ;
            const unsigned char *pixel_data;
            std::vector<MapRegistration::ImageDescriptor> descriptors;
            QString directory;
            QString filename;
        };

        const MapDB::ImageSpaceCoord& topRightCorner() const { return mDb.topRightCorner() ; }
        const MapDB::ImageSpaceCoord& BottomLeftCorner()     const { return mDb.bottomLeftCorner() ; }

        void getImagesToDraw(const MapDB::ImageSpaceCoord &mBottomLeftViewCorner, const MapDB::ImageSpaceCoord& mTopRightViewCorner, std::vector<MapAccessor::ImageData>& images_to_draw) const;
        QImage getImageData(const QString& image_filename);
		bool getImageParams(const QString& image_filename, MapDB::RegisteredImage &img);

        void setReferencePoint(const QString& image_name, int point_x, int point_y) ;
        /*!
         * \brief extractTile 	Extracts an image of size WxH pixels, that covers the given rectangle in GPS coords.
         * \param top_left 		Top left corner of the region to extract
         * \param bottom_right 	Bottom right corner of the region to extract
         * \param W				Width of the output image
         * \param H				Height of the output image
         * \return
         */
		QImage extractTile(const MapDB::ImageSpaceCoord& top_left,const MapDB::ImageSpaceCoord& bottom_right,int W,int H) ;

		void moveImage(const QString& image_filename,float delta_lon,float delta_lat);
		void placeImage(const QString& image_filename,const MapDB::ImageSpaceCoord& new_corner);

        // Returns the number of pixels per degree of longitude/latitude
        float pixelsPerAngle() const ;

		void recomputeDescriptors(const QString& image_filename);
		QString fullPath(const QString& image_filename);

		void saveMap();

        const MapDB& mapDB() const { return mDb ; }

		static bool findImagePixel(const MapDB::ImageSpaceCoord& is,const std::vector<MapAccessor::ImageData>& images,float& img_x,float& img_y,QString& image_filename);

	private:
		const unsigned char *getPixelData(const QString& filename) const;
		QRgb computeInterpolatedPixelValue(const MapDB::ImageSpaceCoord& is) const;

		MapDB& mDb;
        mutable std::map<QString,QImage> mImageCache ;
};

