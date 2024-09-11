#pragma once

#include <map>
#include <vector>
#include <QString>

#include "MapRegistration.h"

class QFile ;
class QString ;

class MapDB
{
	public:
        class ImageHandle
        {
        public:
            ImageHandle() : _n(UINT_MAX) {}
            ImageHandle(uint32_t n) : _n(n) {}

            operator uint32_t() const { return _n; }
            uint32_t& operator()() { return _n; }

            bool isValid() const { return _n!=UINT_MAX; }
        private:
            uint32_t _n;
        };

        struct GPSCoord
		{
            GPSCoord(float _lon,float _lat) : lon(_lon),lat(_lat) {}
            GPSCoord() : lon(0.0f),lat(0.0f) {}

			float lon ;
			float lat ;
		};

        struct ImageSpaceCoord
        {
            ImageSpaceCoord() : x(0.0f),y(0.0f) {}
            ImageSpaceCoord(float _x,float _y) : x(_x),y(_y) {}

            float x;
            float y;
        };

        struct ReferencePoint
        {
            ReferencePoint() : x(-1),y(-1),lat(0.0),lon(0.0),handle(UINT_MAX) {}

            int x,y;
            float lat,lon;
            ImageHandle handle;
        };

		struct RegisteredImage
		{
			int W,H ;							// width/height of the image
            ImageSpaceCoord bottom_left_corner;	// coordinates of the top-left corner of the image

            std::vector<MapRegistration::ImageDescriptor> descriptors;	// images descriptors, used to match images together
		};

        // For debugging and initialization purposes

        virtual const std::map<ImageHandle,MapDB::RegisteredImage>& getFullListOfImages() const =0;
        virtual bool getImageParams(ImageHandle h, MapDB::RegisteredImage& img) const = 0;
        virtual QImage getImageData(ImageHandle h) const =0;
        virtual bool imageSpaceCoordinatesToGPSCoordinates(const MapDB::ImageSpaceCoord& ic,MapDB::GPSCoord& g) const=0;
        virtual const ReferencePoint& getReferencePoint(int i) const =0;
        virtual int numberOfReferencePoints() const =0;

        // accessor methods

        const ImageSpaceCoord& bottomLeftCorner() const { return mBottomLeft ; }
        const ImageSpaceCoord& topRightCorner() const { return mTopRight ; }

    protected:
        MapDB(const QString& name) : mName(name) {}

        void includeImage(const MapDB::ImageSpaceCoord& top_left_corner,int W,int H);

        bool mMapInited ;

        // actual map data

        ImageSpaceCoord mBottomLeft ;
        ImageSpaceCoord mTopRight ;

        QString mName ;
        time_t  mCreationTime;

};

std::ostream& operator<<(std::ostream& o, const MapDB::GPSCoord& c);
std::ostream& operator<<(std::ostream& o, const MapDB::ImageSpaceCoord& c);
