#pragma once

#include <iostream>
#include "MapAccessor.h"

class MapExporter
{
public:
    MapExporter(MapAccessor& ma) : mA(ma) {}

    bool exportMap(const MapDB::ImageSpaceCoord &bottom_left_corner, const MapDB::ImageSpaceCoord &top_right_corner, const QString &output_directory, void (*progress)(float, void *), void *data);

private:
    MapAccessor& mA;
};
