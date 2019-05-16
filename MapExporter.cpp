#include <fstream>
#include <QImageWriter>
#include <QUrl>

#include "MapDB.h"
#include "config.h"
#include "MapExporter.h"

class KmzFile
{
public:
    struct LayerData
    {
        float north_limit;	// limits of the zone
        float south_limit;
        float east_limit;
        float west_limit;

        float rotation ; 	// rotation angle of the zone

        QString image_name;
    };

    void writeToFile(const QString &fname) const ;

    std::vector<LayerData> layers ;
};

void KmzFile::writeToFile(const QString& fname) const
{
    std::ofstream o(fname.toStdString().c_str());

    o << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
    o << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">" << std::endl;

    o << "  <Folder>" << std::endl;
    o << "    <name>Layer</name>" << std::endl;

    for(uint32_t i=0;i<layers.size();++i)
    {
        char output[50] ;
        sprintf(output,"%04d",i) ;

    	o << "      <GroundOverlay>" << std::endl;
        o << "        <name>Layer " << i+1 << "</name>" << std::endl;
        o << "        <Icon>" << std::endl;
        o << "          <href>files/" << layers[i].image_name.toStdString() << "</href>" << std::endl;
        o << "          <drawOrder>50<drawOrder>" << std::endl;			// means the map will be drawn *on top* of the existing maps
        o << "        </Icon>" << std::endl;
        o << "        <LatLonBox>" << std::endl;
        o << "          <north>" << layers[i].north_limit << "</north>" << std::endl;
        o << "          <south>" << layers[i].south_limit << "</south>" << std::endl;
        o << "          <east>" << layers[i].east_limit << "</east>" << std::endl;
        o << "          <west>" << layers[i].west_limit << "</west>" << std::endl;
        o << "          <rotation>" << layers[i].rotation << "</rotation>" << std::endl;
        o << "        </LatLonBox>" << std::endl;
    	o << "      </GroundOverlay>" << std::endl;
    }

    o << "    <open>1</open>" << std::endl;
    o << "  </Folder>" << std::endl;
    o << "</kml>" << std::endl;
    o.close();
}

bool MapExporter::exportMap(const MapDB::ImageSpaceCoord& bottom_left_corner,const MapDB::ImageSpaceCoord& top_right_corner,const QString& output_directory)
{
    std::cerr << "Exporting map to Kmz file. Bottom left: " << bottom_left_corner << ", top right: " << top_right_corner << std::endl;

    // 1 - determine how many tiles we need using the following constraints:
    //     	* each tile should be 1024x1024 at most
    //		* the map accessor will teel us what is the approximate virtual resolution of the window

    if(top_right_corner.x <= bottom_left_corner.x) return false ;
    if(top_right_corner.y <= bottom_left_corner.y) return false ;

    KmzFile kmzfile ;

    uint32_t total_map_W = top_right_corner.x - bottom_left_corner.x ;
    uint32_t total_map_H = top_right_corner.y - bottom_left_corner.y ;

    float tile_size = 1024 ;

    int n_tiles_x = (total_map_W + tile_size)/tile_size;
    int n_tiles_y = (total_map_H + tile_size)/tile_size;

    std::cerr << "  Total map resolution: " << total_map_W << " x " << total_map_H << " which means " << n_tiles_x << " x " << n_tiles_y << " tiles" << std::endl;
    std::cerr << "  Tile size           : " << tile_size << std::endl;

    // 2 - re-sample each tile, write it into a file in a temporary directory
    int n=0;

    for(int i=0;i<n_tiles_x;++i)
        for(int j=0;j<n_tiles_y;++j,++n)
		{
			KmzFile::LayerData ld ;

            MapDB::GPSCoord g1 ;
            MapDB::GPSCoord g2 ;

            MapDB::ImageSpaceCoord bottom_left(bottom_left_corner.x +  i    * tile_size,bottom_left_corner.y +  j    * tile_size);
            MapDB::ImageSpaceCoord top_right  (bottom_left_corner.x + (i+1) * tile_size,bottom_left_corner.y + (j+1) * tile_size);

            mA.mapDB().imageSpaceCoordinatesToGPSCoordinates(bottom_left,g1) ;
            mA.mapDB().imageSpaceCoordinatesToGPSCoordinates(top_right  ,g2) ;

			ld.west_limit  = g1.lon;	// limits of the zone
			ld.east_limit  = g2.lon;
			ld.south_limit = g1.lat;
			ld.north_limit = g2.lat;

			ld.rotation = 0.0; 	// rotation angle of the zone

			QImage img = mA.extractTile(bottom_left,top_right,1024,1024);

            std::cerr << "Extracted " << img.width() << " x " << img.height() << " tile " << i << "," << j << " : Lat: " << g2.lat << " -> " << g1.lat << " Lon: " << g1.lon << " -> " << g2.lon << std::endl;

            ld.image_name = QString("image_data_%1.jpg").arg(static_cast<int>(n),4,10,QChar('0'));
            QString path = QUrl("file://"+ output_directory + "/" + ld.image_name).path();

            if(! img.save(path,"jpg"))
                std::cerr << "ERROR: Cannot save file " << path.toStdString() << std::endl;
            kmzfile.layers.push_back(ld);
		}

    // 3 - make the zip file

    kmzfile.writeToFile(output_directory + "/doc.kml");

    return true;
}


























