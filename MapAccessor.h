#pragma once

#include "MapDB.h"

class MapAccessor
{
	public:
		MapAccessor(const MapDB& mdb);

		void generateImage(const GPSCoord& top_left,const GPSCoord& bottom_right,float scale,MapDB::RegisteredImage& img_out,unsigned char *& pixels_out) ;

	private:
		const MapDB& mDb;
};

