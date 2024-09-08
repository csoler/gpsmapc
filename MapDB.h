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
        MapDB(const QString& directory_name) ; // initializes the map. Loads the registered images entries from a xml file.

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

        const std::map<ImageHandle,MapDB::RegisteredImage>& getFullListOfImages() const { return mImages ; }
        bool getImageParams(ImageHandle h, MapDB::RegisteredImage& img) const;

        // accessor methods

        const QString& rootDirectory() const { return mRootDirectory; }
        const QString& imagesMaskFilename() const { return mImagesMask; }

        const ImageSpaceCoord& bottomLeftCorner() const { return mBottomLeft ; }
        const ImageSpaceCoord& topRightCorner() const { return mTopRight ; }

        void moveImage(ImageHandle h, float delta_is_x, float delta_is_y);
        void placeImage(ImageHandle h,const ImageSpaceCoord& new_corner);
        void setReferencePoint(ImageHandle h,int point_x,int point_y);

        void recomputeDescriptors(ImageHandle h);

        void save();

        const ReferencePoint& getReferencePoint(int i) const { return i==0?mReferencePoint1:mReferencePoint2; }
        int numberOfReferencePoints() const ;

        bool imageSpaceCoordinatesToGPSCoordinates(const MapDB::ImageSpaceCoord& ic,MapDB::GPSCoord& g) const ;

        QImage getImageData(ImageHandle h) const ;
        QString getImagePath(ImageHandle h) const;
	private:
		bool init();
		void loadDB(const QString& source_directory);
		void saveDB(const QString& source_directory);
		void createEmptyMap(QFile& map_file);
        void checkDirectory(const QString& source_directory) ;
		void includeImage(const MapDB::ImageSpaceCoord& top_left_corner,int W,int H);

		QString mRootDirectory ;

        bool mMapInited ;
        bool mMapChanged;

        // actual map data

        std::map<ImageHandle,MapDB::RegisteredImage> mImages ;
        std::vector<QString> mFilenames;

        QString mImagesMask ;

        ImageSpaceCoord mBottomLeft ;
        ImageSpaceCoord mTopRight ;

        ReferencePoint mReferencePoint1;
        ReferencePoint mReferencePoint2;

        QString mName ;
        time_t  mCreationTime;

};

// Manages a collection of maps represented by a collection of separate unregistered files

class ScreenshotCollectionMapDB: public MapDB
{
public:
        ScreenshotCollectionMapDB(const QString& directory_name) ; // initializes the map. Loads the registered images entries from a xml file.

};

// Manages a collection of maps represented by a single QCT file

class QctMapDB: public MapDB
{
public:
        QctMapDB(const QString& qct_filename) ; // initializes the map. Loads the registered images entries from xml file.

};

std::ostream& operator<<(std::ostream& o, const MapDB::GPSCoord& c);
std::ostream& operator<<(std::ostream& o, const MapDB::ImageSpaceCoord& c);
