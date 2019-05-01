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

		struct RegisteredImage
		{
			std::string filename ;		// name of the image in the database directory
			int W,H ;					// width/height of the image
			float scale;				// length of the image in degrees of longitude
			GPSCoord top_left_corner;	// lon/lat of the top-left corner of the image
		};

	private:
		bool init();
		void loadDB(const QString& source_directory, const QString& source_file);
		void createEmptyMap(QFile& map_file);

		QString mRootDirectory ;

        bool mMapInited ;

        // actual map data

		std::vector<MapDB::RegisteredImage> mImages ;

		GPSCoord mTopLeft ;
		GPSCoord mBottomRight ;

        QString mName ;
        time_t  mCreationTime;

};
