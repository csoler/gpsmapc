#include <iostream>

#include <QFile>

#include "QctMapDB.h"
#include "QctFile.h"

#define NOT_IMPLEMENTED std::cerr << __PRETTY_FUNCTION__ << ": not implemented yet."

QctMapDB::QctMapDB(const QString& name)
    : MapDB(name), mQctFile(name.toStdString())
{
    if(!QFile(name).exists())
        throw std::runtime_error("Specified file " + name.toStdString() + " is not present on disk.");

    std::cerr << "Reading Qct file " + name.toStdString() << std::endl;

    double lat_00, lon_00, lat_10, lon_10, lat_01, lon_01, lat_11, lon_11 ;
    mQctFile.computeLatLonLimits(lat_00, lon_00, lat_10, lon_10, lat_01, lon_01, lat_11, lon_11 );

    std::cerr << "Limits: (" << lat_00 << "," << lon_00 << ")" << std::endl;
    std::cerr << "Limits: (" << lat_01 << "," << lon_01 << ")" << std::endl;
    std::cerr << "Limits: (" << lat_10 << "," << lon_10 << ")" << std::endl;
    std::cerr << "Limits: (" << lat_11 << "," << lon_11 << ")" << std::endl;

    if(lat_00 == lat_01 && lon_00 == lon_10)
        std::cerr << "Area is axis aligned. Good." << std::endl;
    else
        std::cerr << "Area is not axis aligned. Not good." << std::endl;

    mBottomLeft.x = 0;
    mBottomLeft.y = 0;
    mTopRight.x = mQctFile.QCT_TILE_SIZE * mQctFile.sizeX();
    mTopRight.y = mQctFile.QCT_TILE_SIZE * mQctFile.sizeY();

    for(int i=0;i<mQctFile.sizeX();++i)
        for(int j=0;j<mQctFile.sizeY();++j)
        {
            MapDB::RegisteredImage registered_img;
            registered_img.W = mQctFile.QCT_TILE_SIZE ;
            registered_img.H = mQctFile.QCT_TILE_SIZE ;
            registered_img.bottom_left_corner.x = mQctFile.QCT_TILE_SIZE * i ;
            registered_img.bottom_left_corner.y = mQctFile.QCT_TILE_SIZE * j ;

            mImages.insert(std::make_pair(MapDB::ImageHandle(i+mQctFile.sizeX()*j),registered_img));
        }
}

const std::map<MapDB::ImageHandle,MapDB::RegisteredImage>& QctMapDB::getFullListOfImages() const
{
    return mImages;
}
bool QctMapDB::getImageParams(ImageHandle h, MapDB::RegisteredImage& img) const
{
    NOT_IMPLEMENTED;

    return false;
}
QImage QctMapDB::getImageData(ImageHandle h) const
{
    int x,y;

    auto p = handleToCoordinates(h);
    return mQctFile.getTileImage(p.first,p.second);
}
bool QctMapDB::imageSpaceCoordinatesToGPSCoordinates(const MapDB::ImageSpaceCoord& ic,MapDB::GPSCoord& g) const
{
    NOT_IMPLEMENTED;

    return false;
}

const MapDB::ReferencePoint& QctMapDB::getReferencePoint(int i) const
{
    NOT_IMPLEMENTED;
    return ReferencePoint();
}
int QctMapDB::numberOfReferencePoints() const { return 0 ; }

std::pair<int,int> QctMapDB::handleToCoordinates(ImageHandle h) const
{
    return std::make_pair( int(h)%mQctFile.sizeX(), int(h)/mQctFile.sizeY());
}

MapDB::ImageHandle QctMapDB::coordinatesToHandle(int x,int y) const
{
    return ImageHandle(mQctFile.sizeX()*y + x);
}

