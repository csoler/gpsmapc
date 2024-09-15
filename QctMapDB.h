#pragma once

#include "QctFile.h"
#include "MapDB.h"

 // Manages a collection of maps represented by a single QCT file

class QctMapDB : public MapDB
{
public:
        QctMapDB(const QString& qct_filename) ; // initializes the map. Loads the registered images entries from xml file.
        virtual ~QctMapDB() {}

        virtual const std::map<ImageHandle,MapDB::RegisteredImage>& getFullListOfImages() const override ;
        virtual bool getImageParams(ImageHandle h, MapDB::RegisteredImage& img) const override;
        virtual QImage getImageData(ImageHandle h) const override;
        virtual bool imageSpaceCoordinatesToGPSCoordinates(const MapDB::ImageSpaceCoord& ic,MapDB::GPSCoord& g) const override;
        virtual const ReferencePoint& getReferencePoint(int i) const override;
        virtual int numberOfReferencePoints() const override;

private:
        std::pair<int,int> handleToCoordinates(ImageHandle h) const;
        ImageHandle coordinatesToHandle(int x,int y) const;
        std::map<MapDB::ImageHandle,MapDB::RegisteredImage> mImages;

        mutable QctFile mQctFile;
};
