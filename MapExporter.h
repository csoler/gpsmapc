#pragma once

#include <iostream>
#include "MapAccessor.h"

class MapExporter
{
public:
    MapExporter(MapAccessor& ma) : mA(ma) {}

    void exportMap(const MapDB::GPSCoord& top_left_corner, const MapDB::GPSCoord& bottom_right_corner, const QString &output_directory);

private:
    MapAccessor& mA;
};
