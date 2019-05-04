#pragma once

#include "MapDB.h"

class MapAccessor
{
	public:
		MapAccessor(const MapDB& mdb) : mDb(mdb) {}

		void generateImage(const MapDB::GPSCoord& top_left,const MapDB::GPSCoord& bottom_right,float scale,MapDB::RegisteredImage& img_out,unsigned char *& pixels_out) ;

	private:
		const MapDB& mDb;
};

