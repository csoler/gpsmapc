#include <values.h>
#include <iostream>

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QtXml>
#include <QImage>

#include "config.h"
#include "MapDB.h"
#include "MapRegistration.h"

std::ostream& operator<<(std::ostream& o, const MapDB::GPSCoord& c) { return o << "(" << c.lon << "," << c.lat << ")" ; }
std::ostream& operator<<(std::ostream& o, const MapDB::ImageSpaceCoord& c) { return o << "(" << c.x << "," << c.y << ")" ; }

void MapDB::includeImage(const MapDB::ImageSpaceCoord& top_left_corner,int W,int H)
{
    if(mBottomLeft.x > top_left_corner.x) mBottomLeft.x = top_left_corner.x;
    if(mBottomLeft.y > top_left_corner.y) mBottomLeft.y = top_left_corner.y;

    if(mTopRight.x < top_left_corner.x+W) mTopRight.x = top_left_corner.x+W;
    if(mTopRight.y < top_left_corner.y+H) mTopRight.y = top_left_corner.y+H;
}




