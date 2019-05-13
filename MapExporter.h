#pragma once

#include <iostream>
#include "MapAccessor.h"

class MapExporter
{
public:
    MapExporter(MapAccessor& ma) : mA(ma) {}

    void exportMap(const MapDB::ImageSpaceCoord &top_left_corner, const MapDB::ImageSpaceCoord &bottom_right_corner, const QString &output_directory);

private:
    MapAccessor& mA;
};
