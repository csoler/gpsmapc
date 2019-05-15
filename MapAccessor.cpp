#include <iostream>
#include <QImage>

#include "MapAccessor.h"

void MapAccessor::getImagesToDraw(const MapDB::ImageSpaceCoord& mBottomLeftViewCorner, const MapDB::ImageSpaceCoord& mTopRightViewCorner, std::vector<ImageData> &images_to_draw) const
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
        id.bottom_left_corner = it->second.bottom_left_corner;
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

bool MapAccessor::findImagePixel(const MapDB::ImageSpaceCoord& is,const std::vector<MapAccessor::ImageData>& images,float& img_x,float& img_y,QString& image_filename)
{
	for(int i=images.size()-1;i>=0;--i)
        if(	       images[i].bottom_left_corner.x               <= is.x
                && images[i].bottom_left_corner.x + images[i].W >= is.x
                && images[i].bottom_left_corner.y               <= is.y
                && images[i].bottom_left_corner.y + images[i].H >= is.y )
		{
			image_filename = images[i].filename;

            img_x = is.x - images[i].bottom_left_corner.x;
            img_y = images[i].H - 1 - (is.y - images[i].bottom_left_corner.y);

            return true;
		}

    return false;
}

QImage MapAccessor::extractTile(const MapDB::ImageSpaceCoord& top_left, const MapDB::ImageSpaceCoord& bottom_right, int W, int H)
{
    std::vector<ImageData> images ;
    getImagesToDraw(MapDB::ImageSpaceCoord(top_left.x,bottom_right.y),MapDB::ImageSpaceCoord(bottom_right.x,top_left.y),images);

    QImage img(W,H,QImage::Format_RGB32);
	float img_x,img_y;

    QString last_image_loaded ;
    QImage current_image;

    for(int i=0;i<W;++i)
        for(int j=0;j<H;++j)
        {
            QString filename ;
            MapDB::ImageSpaceCoord c ;

            c.x = top_left.x + i/(float)W*(bottom_right.x - top_left.x);
            c.y = top_left.y + j/(float)W*(bottom_right.y - top_left.y);

            findImagePixel(c,images,img_x,img_y,filename) ;

            if(last_image_loaded != filename)
            {
                last_image_loaded = filename ;
                current_image = QImage(filename);
            }
            img.setPixelColor(i,j,MapRegistration::interpolated_image_color(current_image.bits(),current_image.width(),current_image.height(),img_x,img_y));
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







