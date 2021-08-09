#include <math.h>
#include <iostream>
#include <QImage>

#include "MapAccessor.h"

MapAccessor::MapAccessor(MapDB& m) : mDb(m)
{
    if(!mDb.imagesMaskFilename().isNull())
        mImageMask = QImage(mDb.rootDirectory() + "/" + mDb.imagesMaskFilename());//.createAlphaMask(Qt::AutoColor);
}


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

        int tmp_W,tmp_H;
        id.texture_data    = getPixelDataForTextureUsage(id.directory + "/" + id.filename,tmp_W,tmp_H);
        id.descriptors     = it->second.descriptors;

        images_to_draw.push_back(id);
    }
}

bool MapAccessor::getImageParams(const QString& image_filename,MapDB::RegisteredImage& img)
{
    return mDb.getImageParams(image_filename,img) ;
}

QImage MapAccessor::getImageData(const QString& image_filename) const
{
    return QImage(mDb.rootDirectory() + "/" + image_filename);
}

bool MapAccessor::findImagePixel(const MapDB::ImageSpaceCoord& is,const std::vector<MapAccessor::ImageData>& images,float& img_x,float& img_y,QString& image_filename)
{
	for(int i=images.size()-1;i>=0;--i)
        if(	       images[i].bottom_left_corner.x               <= is.x
                && images[i].bottom_left_corner.x + images[i].W >  is.x
                && images[i].bottom_left_corner.y               <= is.y
                && images[i].bottom_left_corner.y + images[i].H >  is.y )
		{
			image_filename = images[i].filename;

            img_x = is.x - images[i].bottom_left_corner.x;
            img_y = images[i].H - 1 - (is.y - images[i].bottom_left_corner.y);

            int X = (int)floor(img_x) ;
            int Y = (int)floor(img_y) ;

            if(X < 0 || X >= images[i].W || Y < 0 || Y >= images[i].H)
                continue;

            if(mImageMask.width() == images[i].W && mImageMask.height() == images[i].H && mImageMask.pixel(X,Y) == 0)
                continue;

            return true;
		}

    return false;
}

QImage MapAccessor::extractTile(const MapDB::ImageSpaceCoord& bottom_left, const MapDB::ImageSpaceCoord& top_right, int W, int H)
{
    std::vector<ImageData> images ;
    getImagesToDraw(bottom_left,top_right,images);

    QImage img(W,H,QImage::Format_RGB32);
	float img_x,img_y;

    for(int i=0;i<W;++i)
       for(int j=0;j<H;++j)
        {
            QString filename ;
            MapDB::ImageSpaceCoord c ;

            c.x = bottom_left.x + i/(float)W*(top_right.x - bottom_left.x);
            c.y = bottom_left.y + j/(float)H*(top_right.y - bottom_left.y);

            if(! findImagePixel(c,images,img_x,img_y,filename))
            {
                img.setPixelColor(i,H-1-j,QRgb(0));
                continue;
            }

            int tmp_W,tmp_H;
            const unsigned char *data = getPixelData(filename,tmp_W,tmp_H);

            img.setPixelColor(i,H-1-j,MapRegistration::interpolated_image_color_ABGR(data,tmp_W,tmp_H,img_x,img_y));
        }

    return img;
}

const unsigned char *MapAccessor::getPixelData(const QString& filename,int& W,int& H) const
{
    auto it = mImageCache.find(filename) ;

    if(mImageCache.end() != it)
    {
        W = it->second.width();
        H = it->second.height();
        return it->second.bits() ;
    }

    QImage img = getImageData(filename);

    std::cerr << "Loading/caching image data for file " << filename.toStdString() << ", format=" << img.format() << std::endl;

    W=img.width();
    H=img.height();
    mImageCache[filename] = img;

    return mImageCache[filename].bits();
}
const unsigned char *MapAccessor::getPixelDataForTextureUsage(const QString& filename,int& W,int& H) const
{
    auto it = mImageTextureCache.find(filename) ;

    if(mImageTextureCache.end() != it)
        return it->second.bits() ;

    std::cerr << "Loading/caching image data for file " << filename.toStdString() << std::endl;

	QImage image(filename);

    if(mImageMask.width() == image.width() || mImageMask.height() == image.height())
        image.setAlphaChannel(mImageMask);

    // static bool toto=false;
    // if(!toto)
    // {
    //     image.save("toto.png");
    //     toto=true;
    // }

    mImageTextureCache[filename] = image.scaled(1024,1024,Qt::IgnoreAspectRatio,Qt::SmoothTransformation).rgbSwapped() ;
    W = H = 1024;

    return mImageTextureCache[filename].bits();
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







