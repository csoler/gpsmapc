#pragma once

#include <QImage>
#include "MapDB.h"

class MapAccessor
{
	public:
		MapAccessor(MapDB& mdb) ;

        struct ImageData
        {
          	int W,H;
            MapDB::ImageSpaceCoord bottom_left_corner ;
            const unsigned char *texture_data;
            std::vector<MapRegistration::ImageDescriptor> descriptors;
            MapDB::ImageHandle handle;
        };

        const MapDB::ImageSpaceCoord& topRightCorner() const { return mDb.topRightCorner() ; }
        const MapDB::ImageSpaceCoord& BottomLeftCorner()     const { return mDb.bottomLeftCorner() ; }

        void getImagesToDraw(const MapDB::ImageSpaceCoord &mBottomLeftViewCorner, const MapDB::ImageSpaceCoord& mTopRightViewCorner, std::vector<MapAccessor::ImageData>& images_to_draw) const;
        QImage getImageData(MapDB::ImageHandle h) const;
        bool getImageParams(MapDB::ImageHandle h, MapDB::RegisteredImage& img);
        const QImage& imageMask() const { return mImageMask ;}

        void setReferencePoint(MapDB::ImageHandle h, int point_x, int point_y) ;
        /*!
         * \brief extractTile 	Extracts an image of size WxH pixels, that covers the given rectangle in GPS coords.
         * \param bottom_left	Bottom left corner of the region to extract
         * \param top_right 	Top right corner of the region to extract
         * \param W				Width of the output image
         * \param H				Height of the output image
         * \return
         */
		QImage extractTile(const MapDB::ImageSpaceCoord& bottom_left, const MapDB::ImageSpaceCoord& top_right, int W, int H) ;

        void moveImage(MapDB::ImageHandle h, float delta_lon, float delta_lat);
        void placeImage(MapDB::ImageHandle h, const MapDB::ImageSpaceCoord& new_corner);

        void recomputeDescriptors(MapDB::ImageHandle h);
        QString fullPath(MapDB::ImageHandle h);

		void saveMap();

        const MapDB& mapDB() const { return mDb ; }

        bool findImagePixel(const MapDB::ImageSpaceCoord& is, const std::vector<MapAccessor::ImageData>& images, float& img_x, float& img_y, MapDB::ImageHandle &h);

	private:
        const unsigned char *getPixelDataForTextureUsage(MapDB::ImageHandle, int &W, int &H) const;
        const unsigned char *getPixelData(MapDB::ImageHandle h,int& W,int& H) const;

		QRgb computeInterpolatedPixelValue(const MapDB::ImageSpaceCoord& is) const;

		MapDB& mDb;
        mutable std::map<MapDB::ImageHandle,QImage> mImageTextureCache ;
        mutable std::map<MapDB::ImageHandle,QImage> mImageCache ;
        mutable QImage mImageMask;
};

