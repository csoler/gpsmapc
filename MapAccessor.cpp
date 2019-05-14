#include <iostream>
#include <QImage>

#include "MapAccessor.h"

void MapAccessor::getImagesToDraw(MapDB::ImageSpaceCoord& mBottomLeftViewCorner, const MapDB::ImageSpaceCoord& mTopRightViewCorner, std::vector<ImageData> &images_to_draw) const
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

QRgb MapAccessor::computeInterpolatedPixelValue(const MapDB::ImageSpaceCoord& is) const
{
	QString selection ;

	// That could be accelerated using a KDtree

#warning Code missing here: TODO
// 	for(int i=mImagesToDraw.size()-1;i>=0;--i)
// 		if(	       mImagesToDraw[i].top_left_corner.x                      <= is_x
// 		           && mImagesToDraw[i].top_left_corner.x + mImagesToDraw[i].W >= is_x
// 		           && mImagesToDraw[i].top_left_corner.y                      <= is_y
// 		           && mImagesToDraw[i].top_left_corner.y + mImagesToDraw[i].H >= is_y )
// 		{
// 			image_filename = mImagesToDraw[i].filename;
//
// 			float img_x =                           is_x - mImagesToDraw[i].top_left_corner.x;
// 			float img_y = mImagesToDraw[i].H - 1 - (is_y - mImagesToDraw[i].top_left_corner.y);
//
// 			return true;
// 		}

	return 0;
}

QImage MapAccessor::extractTile(const MapDB::ImageSpaceCoord& top_left, const MapDB::ImageSpaceCoord& bottom_right, int W, int H)
{
    QImage img(W,H,QImage::Format_RGB32);

    for(int i=0;i<W;++i)
        for(int j=0;j<H;++j)
        {
            QString filename ;

            float cx = top_left.x + i/(float)W*(bottom_right.x - top_left.x);
            float cy = top_left.y + j/(float)W*(bottom_right.y - top_left.y);

            img.setPixel(i,j,computeInterpolatedPixelValue(MapDB::ImageSpaceCoord(cx,cy)));
        }

    return QImage();
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

void MapAccessor::placeImage(const QString& image_filename,const MapDB::ImageSpaceCoord& new_corner)
{
    return mDb.placeImage(image_filename,new_corner);
}

float MapAccessor::pixelsPerAngle() const
{
    // TODO
#warning TODO
    return 1.0;
}

void MapAccessor::setReferencePoint(const QString& image_name,int point_x,int point_y)
{
    mDb.setReferencePoint(image_name,point_x,point_y);
}







