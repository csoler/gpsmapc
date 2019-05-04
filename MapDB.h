#pragma once

#include <string>
#include <vector>

class QFile ;
class QString ;

class MapDB
{
	public:
		MapDB(const std::string& directory_name) ; // initializes the map. Loads the registered images entries from xml file.

		struct GPSCoord
		{
			float lon ;
			float lat ;
		};

        typedef uint64_t ImageID ;

		struct RegisteredImage
		{
			int W,H ;					// width/height of the image
			float scale;				// length of the image in degrees of longitude
			GPSCoord top_left_corner;	// lon/lat of the top-left corner of the image
		};

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
