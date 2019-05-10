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

		struct RegisteredImage
		{
			int W,H ;					// width/height of the image
			float scale;				// length of the image in degrees of longitude
			GPSCoord top_left_corner;	// lon/lat of the top-left corner of the image

            std::vector<MapRegistration::ImageDescriptor> descriptors;	// images descriptors, used to match images together
		};

        // For debugging and initialization purposes

        const std::map<QString,MapDB::RegisteredImage>& getFullListOfImages() const { return mImages ; }

        // accessor methods

        const QString& rootDirectory() const { return mRootDirectory; }

        const GPSCoord& topLeftCorner() const { return mTopLeft ; }
        const GPSCoord& bottomRightCorner() const { return mBottomRight ; }

		void moveImage(const QString& mSelectedImage,float delta_lon,float delta_lat);
		void recomputeDescriptors(const QString& image_filename);

        void save();

	private:
		bool init();
		void loadDB(const QString& source_directory);
		void saveDB(const QString& source_directory);
		void createEmptyMap(QFile& map_file);
        void checkDirectory(const QString& source_directory) ;

		QString mRootDirectory ;

        bool mMapInited ;
        bool mMapChanged;

        // actual map data

		std::map<QString,MapDB::RegisteredImage> mImages ;

		GPSCoord mTopLeft ;
		GPSCoord mBottomRight ;

        QString mName ;
        time_t  mCreationTime;

};

std::ostream& operator<<(std::ostream& o, const MapDB::GPSCoord& c);
