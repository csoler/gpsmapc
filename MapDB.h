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
		MapDB(const QString& directory_name) ; // initializes the map. Loads the registered images entries from xml file.

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
            ReferencePoint() : x(-1),y(-1),lat(0.0),lon(0.0) {}

            int x,y;
            float lat,lon;
            QString filename;
        };

		struct RegisteredImage
		{
			int W,H ;							// width/height of the image
			ImageSpaceCoord top_left_corner;	// coordinates of the top-left corner of the image

            std::vector<MapRegistration::ImageDescriptor> descriptors;	// images descriptors, used to match images together
		};

        // For debugging and initialization purposes

        const std::map<QString,MapDB::RegisteredImage>& getFullListOfImages() const { return mImages ; }
    	bool getImageParams(const QString& image_filename,MapDB::RegisteredImage& img) const;

        // accessor methods

        const QString& rootDirectory() const { return mRootDirectory; }

        const ImageSpaceCoord& topLeftCorner() const { return mTopLeft ; }
        const ImageSpaceCoord& bottomRightCorner() const { return mBottomRight ; }

		void moveImage(const QString& mSelectedImage, float delta_is_x, float delta_is_y);
    	void placeImage(const QString& image_filename,const ImageSpaceCoord& new_corner);
		void setReferencePoint(const QString& image_name,int point_x,int point_y);

		void recomputeDescriptors(const QString& image_filename);

        void save();

        const ReferencePoint& getReferencePoint(int i) const { return i==0?mReferencePoint1:mReferencePoint2; }
        int numberOfReferencePoints() const ;

        bool viewCoordinatesToGPSCoordinates(const MapDB::ImageSpaceCoord& ic,MapDB::GPSCoord& g) const ;

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

		std::map<QString,MapDB::RegisteredImage> mImages ;

		ImageSpaceCoord mTopLeft ;
		ImageSpaceCoord mBottomRight ;

        ReferencePoint mReferencePoint1;
        ReferencePoint mReferencePoint2;

        QString mName ;
        time_t  mCreationTime;

};

std::ostream& operator<<(std::ostream& o, const MapDB::GPSCoord& c);
std::ostream& operator<<(std::ostream& o, const MapDB::ImageSpaceCoord& c);
