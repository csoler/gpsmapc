#include <iostream>

#include <QFile>

#include "QctMapDB.h"
#include "QctFile.h"

#define NOT_IMPLEMENTED std::cerr << __PRETTY_FUNCTION__ << ": not implemented yet."

QctMapDB::QctMapDB(const QString& name)
    : MapDB(name)
{
    if(!QFile(name).exists())
        throw std::runtime_error("Specified file " + name.toStdString() + " is not present on disk.");

    std::cerr << "Reading Qct file " + name.toStdString() << std::endl;

    mQctFile.readFilename(name.toStdString().c_str(),true,nullptr);

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
}

const std::map<MapDB::ImageHandle,MapDB::RegisteredImage>& QctMapDB::getFullListOfImages() const
{
    NOT_IMPLEMENTED;

    return std::map<MapDB::ImageHandle,MapDB::RegisteredImage>();
}
bool QctMapDB::getImageParams(ImageHandle h, MapDB::RegisteredImage& img) const
{
    NOT_IMPLEMENTED;

    return false;
}
QImage QctMapDB::getImageData(ImageHandle h) const
{
    NOT_IMPLEMENTED;

    return QImage();
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
int QctMapDB::numberOfReferencePoints() const { return 2 ; }
