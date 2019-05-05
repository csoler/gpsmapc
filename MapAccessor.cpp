#include <iostream>
#include <QImage>

#include "MapAccessor.h"

void MapAccessor::getImagesToDraw(const MapDB::GPSCoord& mBottomLeftViewCorner, const MapDB::GPSCoord& mTopRightViewCorner, std::vector<ImageData> &images_to_draw) const
{
    // For now, just dont be subtle: return all known images.
    // To do:
    //	* only return the images that actually cross the supplied rectangle of coordinates
    //	* add a cache and load images n demand from the disk

	const std::map<QString,MapDB::RegisteredImage>& images_map = mDb.getFullListOfImages();
    images_to_draw.clear();

 	for(auto it(images_map.begin());it!=images_map.end();++it)
    {
        ImageData id ;

        id.W = it->second.W;
        id.H = it->second.H;
        id.lon_width = it->second.scale;
        id.top_left_corner = it->second.top_left_corner;
        id.filename = it->first ;
        id.pixel_data = getPixelData(id.filename);

        images_to_draw.push_back(id);
    }
}

void MapAccessor::generateImage(const MapDB::GPSCoord& top_left,const MapDB::GPSCoord& bottom_right,float scale,MapDB::RegisteredImage& img_out,unsigned char *& pixels_out)
{
    std::cerr << __PRETTY_FUNCTION__ << ": not implemented." << std::endl;
    pixels_out = NULL ;
}

const unsigned char *MapAccessor::getPixelData(const QString& filename) const
{
    auto it = mImageCache.find(filename) ;

    if(mImageCache.end() != it)
        return it->second.bits() ;

    mImageCache[filename] = QImage(filename) ;

    return mImageCache[filename].bits();
}
