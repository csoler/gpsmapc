#include <fstream>

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
        o << "          <drawOrder>0<drawOrder>" << std::endl;
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

void MapExporter::exportMap(const MapDB::ImageSpaceCoord& top_left_corner,const MapDB::ImageSpaceCoord& bottom_right_corner,const QString& output_directory)
{
    std::cerr << "Exporting map to Kmz file. Top left: " << top_left_corner << ", bottom right: " << bottom_right_corner << std::endl;

    // 1 - determine how many tiles we need using the following constraints:
    //     	* each tile should be 1024x1024 at most
    //		* the map accessor will teel us what is the approximate virtual resolution of the window

    KmzFile kmzfile ;

    uint32_t total_map_W = (bottom_right_corner.x - top_left_corner.x) * mA.pixelsPerAngle();
    uint32_t total_map_H = (bottom_right_corner.y - top_left_corner.y) * mA.pixelsPerAngle();

    int n_tiles_x = (total_map_W + 1024)/1024;
    int n_tiles_y = (total_map_H + 1024)/1024;

    float tile_angular_size = 1024 / mA.pixelsPerAngle();

    std::cerr << "  Total map resolution: " << total_map_W << " x " << total_map_H << " which means " << n_tiles_x << " x " << n_tiles_y << " tiles" << std::endl;
    std::cerr << "  Tile angular size   : " << tile_angular_size << std::endl;

    // 2 - re-sample each tile, write it into a file in a temporary directory
    int n=0;

    for(int i=0;i<n_tiles_x;++i)
        for(int j=0;j<n_tiles_y;++j,++n)
		{
			KmzFile::LayerData ld ;

            MapDB::GPSCoord g1 ;
            MapDB::GPSCoord g2 ;

            MapDB::ImageSpaceCoord top_left    (top_left_corner.x + (i+1) * tile_angular_size,top_left_corner.y +  j    * tile_angular_size);
            MapDB::ImageSpaceCoord bottom_right(top_left_corner.x +  i    * tile_angular_size,top_left_corner.y + (j+1) * tile_angular_size);

            mA.mapDB().imageSpaceCoordinatesToGPSCoordinates(top_left,g1) ;
            mA.mapDB().imageSpaceCoordinatesToGPSCoordinates(bottom_right,g2) ;

			ld.east_limit  = g2.lon;
			ld.west_limit  = g1.lon;
			ld.north_limit = g1.lat;	// limits of the zone
			ld.south_limit = g2.lat;

			ld.rotation = 0.0; 	// rotation angle of the zone

			QImage img = mA.extractTile(top_left,bottom_right,1024,1024);

            ld.image_name = "image_data_" + QString::number(n,10,'0')+".jpg";

         	img.save(output_directory + "/" + ld.image_name,"JPEG");

            kmzfile.layers.push_back(ld);
		}

    // 3 - make the zip file

    kmzfile.writeToFile(output_directory + "/doc.kml");
}


























