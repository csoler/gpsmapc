#include <iostream>
#include <QImage>

#include "MapAccessor.h"

void MapAccessor::getImagesToDraw(MapDB::GPSCoord &mBottomLeftViewCorner, const MapDB::GPSCoord& mTopRightViewCorner, std::vector<ImageData> &images_to_draw) const
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

        id.W               = it->second.W;
        id.H               = it->second.H;
        id.lon_width       = it->second.scale;
        id.top_left_corner = it->second.top_left_corner;
        id.directory       = mDb.rootDirectory() ;
        id.filename        = it->first ;
        id.pixel_data      = getPixelData(id.directory + "/" + id.filename);
        id.descriptors     = it->second.descriptors;

        images_to_draw.push_back(id);
    }
}

bool MapAccessor::getImageParams(const QString& image_filename,MapDB::RegisteredImage& img)
{
    return mDb.getImageParams(image_filename,img) ;
}

QImage MapAccessor::getImageData(const QString& image_filename)
{
    return QImage(mDb.rootDirectory() + "/" + image_filename);
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

    std::cerr << "Loading/caching image data for file " << filename.toStdString() << std::endl;

    mImageCache[filename] = QImage(filename).scaled(1024,1024,Qt::IgnoreAspectRatio,Qt::SmoothTransformation).rgbSwapped() ;

    return mImageCache[filename].bits();
}

void MapAccessor::moveImage(const QString& image_filename,float delta_lon,float delta_lat)
{
    mDb.moveImage(image_filename,delta_lon,delta_lat);
}

void MapAccessor::recomputeDescriptors(const QString& image_filename)
{
    mDb.recomputeDescriptors(image_filename);
}

void MapAccessor::saveMap()
{
	mDb.save();
}

QString MapAccessor::fullPath(const QString& image_filename)
{
    return mDb.rootDirectory() + "/" + image_filename;
}

void MapAccessor::placeImage(const QString& image_filename,const MapDB::GPSCoord& new_corner)
{
    return mDb.placeImage(image_filename,new_corner);
}









