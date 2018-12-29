// COMPILE_LINE: g++ -g -o qct2png qct2png.cpp -D_FILE_OFFSET_BITS=64 -lstdc++
//
/* > qct.cpp
 */

#include <iostream>
#include <vector>
#include <assert.h>
#include <fstream>

static const char SCCSid[] = "@(#)qct.c         1.02 (C) 2010 arb Convert QCT map to PNG";

/*
 * Byte order
 */
#if defined( SUNOS ) || defined( SOLARIS ) || defined( SUN4 ) || defined( solaris ) || defined( sun4 ) || defined( sun )
#include <sys/isa_defs.h>
#elif defined( MSDOS ) || defined( WIN32 )
#define _LITTLE_ENDIAN
#elif defined ( linux )
#include <endian.h>
# if __BYTE_ORDER == __LITTLE_ENDIAN
# define _LITTLE_ENDIAN
# else
# define _BIG_ENDIAN
# endif
#elif defined ( __CYGWIN__ )
#define _LITTLE_ENDIAN
#else
#error Unknown byte sex
#endif


/*
 * 64-bit file I/O
 * Not specifically needed for the QCT decoder since it uses 32-bit
 * file offsets inside the file so can't be more than 2GB anyway.
 */
#ifdef unix
#define HAS_FOPEN64
#endif

#ifdef HAS_FOPEN64
# ifndef _LARGEFILE64_SOURCE
#  define _LARGEFILE64_SOURCE // see lf64 manual
# endif
# define FOPEN  fopen64
# define FSEEKO fseeko64
# define FTELLO ftello64
# define OFF_T  off64_t
#else
# define FOPEN  fopen
# define FSEEKO fseeko
# define FTELLO ftello
# define OFF_T  off_t
#endif


/*
 * Get command-line options
 */
#ifndef GETOPT
#include <getopt.h>
#define GETOPT(c, options) c = optind = 1; while (c && ((c=getopt(argc,argv,options))!=-1)) switch(c)
#endif /*GETOPT*/
#ifndef GETOPT_LOOP_REST
#define GETOPT_LOOP_REST(argp) \
	while((optind < argc)? (argp = argv[optind++]): (argp = NULL))
#endif


/*
 * Includes
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>  // for strerror
#include <time.h>    // for ctime
#include <math.h>    // for log2
#include <errno.h>   // for errno

#ifdef USE_GIFLIB
#include "gif/gif.h"
#endif

#ifdef USE_PNG
#include <png.h>
#endif

#ifdef USE_TIFF
#endif


// QCT file format
// as documented in:
// The_Quick_Chart_File_Format_Specification_1.01.pdf
// All words are little-endian
// Header: 24 words
// Georef: 40 doubles
// Palette: 256 words of RGB where blue is LSByte
// Interp: 128x128 bytes
// Index: w*h words
// Data... tiles are 64x64 pixels

#define QCT_MAGIC 0x1423D5FF
#define QCT_TILE_SIZE 64
#define QCT_TILE_PIXELS (QCT_TILE_SIZE*QCT_TILE_SIZE)
// RGB stored packed in an int, Blue is LSB
#define PAL_RED(c)   ((c>>16)&255)
#define PAL_GREEN(c) ((c>>8)&255)
#define PAL_BLUE(c)  ((c)&255)


/* -------------------------------------------------------------------------
 * Generic little-endian file-reading
 */
int
readInt(FILE *fp)
{
	int vv;
	vv = fgetc(fp);
	vv |= (fgetc(fp) << 8);
	vv |= (fgetc(fp) << 16);
	vv |= (fgetc(fp) << 24);
	if (feof(fp)) return(-1);
	return(vv);
}

static bool writePNGFile(FILE *, const unsigned char *data, const int *palette, int W, int H);
static bool writePNGFilename(const char *filename, const unsigned char *data, const int *palette, int W, int H);

class KmzFile
{
public:
    struct LayerData
    {
        float north_limit;	// limits of the zone
        float south_limit;
        float east_limit;
        float west_limit;

        float rotation ; 	// rotation angle of the zone
    };

    void writeToFile(const std::string& fname) const ;

    std::vector<LayerData> layers ;
};

void KmzFile::writeToFile(const std::string& fname) const
{
    std::ofstream o(fname);

    o << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
    o << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">" << std::endl;

    o << "  <Folder>" << std::endl;
    o << "    <name>Layer</name>" << std::endl;

    for(uint32_t i=0;i<layers.size();++i)
    {
        char output[50] ;
        sprintf(output,"%04d",i) ;

    	o << "      <GroundOverlay>" << std::endl;
        o << "        <name>Layer " << i+1 << "</name>" << std::endl;
        o << "        <Icon>" << std::endl;
        o << "          <href>files/image_data_" << output << ".jpg</href>" << std::endl;
        o << "          <drawOrder>0<drawOrder>" << std::endl;
        o << "        </Icon>" << std::endl;
        o << "        <LatLonBox>" << std::endl;
        o << "          <north>" << layers[i].north_limit << "</north>" << std::endl;
        o << "          <south>" << layers[i].south_limit << "</south>" << std::endl;
        o << "          <east>" << layers[i].east_limit << "</east>" << std::endl;
        o << "          <west>" << layers[i].west_limit << "</west>" << std::endl;
        o << "          <rotation>" << layers[i].rotation << "</rotation>" << std::endl;
        o << "        </LatLonBox>" << std::endl;
    	o << "      </GroundOverlay>" << std::endl;
    }

    o << "    <open>1</open>" << std::endl;
    o << "  </Folder>" << std::endl;
    o << "</kml>" << std::endl;
    o.close();
}

double
readDouble(FILE *fp)
{
	double dd;
	unsigned char *pp = (unsigned char*)&dd;
#ifdef _LITTLE_ENDIAN
	pp[0] = fgetc(fp);
	pp[1] = fgetc(fp);
	pp[2] = fgetc(fp);
	pp[3] = fgetc(fp);
	pp[4] = fgetc(fp);
	pp[5] = fgetc(fp);
	pp[6] = fgetc(fp);
	pp[7] = fgetc(fp);
#else
	pp[7] = fgetc(fp);
	pp[6] = fgetc(fp);
	pp[5] = fgetc(fp);
	pp[4] = fgetc(fp);
	pp[3] = fgetc(fp);
	pp[2] = fgetc(fp);
	pp[1] = fgetc(fp);
	pp[0] = fgetc(fp);
#endif
	return(dd);
}


/* -------------------------------------------------------------------------
 * Returns a string (nul-terminated) allocated from malloc
 * obtained by reading an index pointer then following it
 * by seeking into the file.  NB. file pointer upon return
 * will be after the index pointer not after the string.
 * If index pointer is nul then an empty string is returned
 * (rather than a null pointer).
 */
char *
readString(FILE *fp)
{
	OFF_T current_offset, string_offset;
	int ii;
	char *string;
	int string_length = 0, buf_length = 1024;

	string = (char*)malloc(buf_length);
	ii = readInt(fp);
	if (ii == 0)
		return(strdup(""));
	current_offset = FTELLO(fp);
	string_offset = ii; // yes, offsets are limited to 32-bits :-(
	FSEEKO(fp, string_offset, SEEK_SET);
	while (1)
	{
		ii = fgetc(fp);
		string[string_length] = ii;
		if (ii == 0)
			break;
		string_length++;
		if (string_length > buf_length)
		{
			buf_length += 1024;
			string = (char*)realloc(string, buf_length);
		}
	}
	string = (char*)realloc(string, string_length+1);
	FSEEKO(fp, current_offset, SEEK_SET);
	return(string);
}


/* -------------------------------------------------------------------------
 */
int
bits_per_pixel(int num_colours)
{
	return (int)(log2(num_colours)+0.999);
	/* Alrernatively: */
	if (num_colours <=   2) return 1;
	if (num_colours <=   4) return 2;
	if (num_colours <=   8) return 3;
	if (num_colours <=  16) return 4;
	if (num_colours <=  32) return 5;
	if (num_colours <=  64) return 6;
	if (num_colours <= 128) return 7;
}


/* -------------------------------------------------------------------------
 * Class to read a QCT map image.
 */
class QCT
{
public:
	QCT();
	~QCT();

	void message(const char *fmt, ...);

    // compute latitude/longitude of a four corners

	int computeLatLonLimits(double& lat_00, double& lon_00,
                            double& lat_10, double& lon_10,
                            double& lat_01, double& lon_01,
                            double& lat_11, double& lon_11 );

	void debugmsg(const char *fmt, ...);
protected:
	void throwError(const char *fmt, ...);
	void readTile(FILE *, int tile_x, int tile_y,unsigned char *data);
	int xy_to_latlon(int pixel_x, int pixel_y, double *lat, double *lon);
public:
	void setDebug(int d)     { debug = d; }
	void setVerbose(int v)   { verbose = v; }
	void setBounds(int _top_left_x,int _top_left_y,int _size_x,int _size_y)   { top_left_x=_top_left_x; top_left_y=_top_left_y;size_x=_size_x;size_y=_size_y; }
	bool readFile(FILE *, int headeronly, unsigned char *data);
	bool readFilename(const char *filename, int headeronly, unsigned char *image_data);
	void printMetadata(FILE *fp);
	//bool writePPMFile(FILE *);
	//bool writePPMFilename(const char *filename);
	bool writeKMZFile();

    const int *getPalette() const { return palette ; }

private:
	int width, height;         // size in tiles (of 64x64 each)
	int top_left_x,top_left_y,size_x,size_y;	// subparts of the tiles that will be output into an image
	int palette[256];          // combined RGB in each int
	unsigned char *image_data; // one pixel per byte
	// Metadata
	struct
	{
		int  version;
		char *title, *name, *ident, *edition, *revision;
		char *keywords, *copyright, *scale, *datum;
		char *depths, *heights, *projection;
		int  flags;
		char *origfilename;
		int  origfilesize;
		time_t origfiletime;
		int  unknown1;
		char *maptype, *diskname;
		int  unknown2, unknown3, unknown4, unknown5, unknown6;
	} metadata;
	// Outline
	int num_outline;
	double *outline_lat, *outline_lon;
	// Georeferencing coefficients
	double eas, easY, easX, easYY, easXY, easXX, easYYY, easYYX, easYXX, easXXX;
	double nor, norY, norX, norYY, norXY, norXX, norYYY, norYYX, norYXX, norXXX;
	double lat, latX, latY, latXX, latXY, latYY, latXXX, latXXY, latXYY, latYYY;
	double lon, lonX, lonY, lonXX, lonXY, lonYY, lonXXX, lonXXY, lonXYY, lonYYY;
	double datum_shift_north, datum_shift_east;
	// Program options
	int verbose, debug, debug_kml_outline, debug_kml_boundary;
	FILE *dfp; // debug output goes here
};


/* -------------------------------------------------------------------------
 */
QCT::QCT()
{
	width = height = 0;
	image_data = NULL;

	top_left_x=0;
	top_left_y=0;
	size_x=0;
	size_y=0;	

	// Metadata
	metadata.title = metadata.name = metadata.ident = metadata.edition = metadata.revision = NULL;
	metadata.keywords = metadata.copyright = metadata.scale = metadata.datum = NULL;
	metadata.depths = metadata.heights = metadata.projection = NULL;
	metadata.origfilename = NULL;
	metadata.maptype = metadata.diskname = NULL;

	// Outline
	num_outline = 0;
	outline_lat = outline_lon = NULL;

	// Program options
	verbose = debug = debug_kml_outline = debug_kml_boundary = 0;
	dfp = stdout;
}


QCT::~QCT()
{
	// Image data
	if (image_data) free(image_data);
	// Metadata
	if (metadata.title) free(metadata.title);
	if (metadata.name) free(metadata.name);
	if (metadata.ident) free(metadata.ident);
	if (metadata.edition) free(metadata.edition);
	if (metadata.revision) free(metadata.revision);
	if (metadata.keywords) free(metadata.keywords);
	if (metadata.copyright) free(metadata.copyright);
	if (metadata.scale) free(metadata.scale);
	if (metadata.datum) free(metadata.datum);
	if (metadata.depths) free(metadata.depths);
	if (metadata.heights) free(metadata.heights);
	if (metadata.projection) free(metadata.projection);
	if (metadata.origfilename) free(metadata.origfilename);
	if (metadata.maptype) free(metadata.maptype);
	if (metadata.diskname) free(metadata.diskname);
	// Map outline
	if (outline_lat) free(outline_lat);
	if (outline_lon) free(outline_lon);
}


/* -------------------------------------------------------------------------
 */
void
QCT::throwError(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}


void
QCT::message(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (dfp && (debug|verbose))
	{
		vfprintf(dfp, fmt, ap);
		fprintf(dfp, "\n");
	}
	va_end(ap);
}


void
QCT::debugmsg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (debug)
	{
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
	}
	va_end(ap);
}


/* -------------------------------------------------------------------------
 */
void
QCT::readTile(FILE *fp, int tile_xx, int tile_yy,unsigned char *data)
{
	unsigned char tile_data[QCT_TILE_PIXELS];
	unsigned char *row_ptr[QCT_TILE_SIZE];
	int packing;
	unsigned long row, bytes_per_row;
	int pixelnum = 0;
	int ii;
	// Rows are interleaved in this order (reverse binary)
	static int row_seq[] =
	{
		0,  32, 16, 48,  8, 40, 24, 56,  4, 36, 20, 52, 12, 44, 28, 60, 2,
		34, 18, 50, 10, 42, 26, 58,  6, 38, 22, 54, 14, 46, 30, 62, 1,
		33, 17, 49,  9, 41, 25, 57,  5, 37, 21, 53, 13, 45, 29, 61, 3, 35,
		19, 51, 11, 43, 27, 59,  7, 39, 23, 55, 15, 47, 31, 63
	};


	debugmsg("Tile %d, %d starts at file offset 0x%x", tile_xx, tile_yy, FTELLO(fp));

	// Determine which method was used to pack this tile
	packing = fgetc(fp);

	debugmsg("Reading tile %d, %d; packed using %s", tile_xx, tile_yy, ((packing==0||packing==255)?"huffman":(packing>127?"pixel":"RLE")));

	memset(tile_data, 0, QCT_TILE_PIXELS);

	// Size for one whole row in data
	bytes_per_row = size_x * QCT_TILE_SIZE;

	assert(tile_xx >= top_left_x) ;
	assert(tile_yy >= top_left_y) ;

	// Calculate pointer into data for each row in this tile
	for (row=0; row<QCT_TILE_SIZE; row++)
	{
		// Top left corner of tile within image
		row_ptr[row] = data + ( (tile_yy-top_left_y) * QCT_TILE_SIZE * bytes_per_row) + ((tile_xx-top_left_x) * QCT_TILE_SIZE);
		// Interleaved rows in the above sequence
		row_ptr[row] += row_seq[row] * bytes_per_row;
	}

	// Uncompress each row
	if (packing == 0 || packing == 255)
	{
		// Huffman
		//debugmsg("Huffman");
		int huff_size = 256;
		unsigned char *huff = (unsigned char *)malloc(huff_size);
		unsigned char *huff_ptr = huff;
		int huff_idx = 0;
		int num_colours = 0;
		int num_branches = 0;
		while (num_colours <= num_branches)
		{
			huff[huff_idx] = getc(fp);
			// Relative jump further than 128 needs two more bytes
			if (huff[huff_idx] == 128)
			{
				huff[++huff_idx] = getc(fp);
				huff[++huff_idx] = getc(fp);
				num_branches++;
			}
			// Relative jump nearer is encoded directly
			else if (huff[huff_idx] > 128)
			{
				// Count number of branches so we know when tree is built
				num_branches++;
			}
			// Otherwise it's a colour index into the palette
			else
			{
				// Count number of colours so we know when tree is built
				num_colours++;
			}
			huff_idx++;
			// Make space when Huffman table runs out of space
			if (huff_idx > huff_size)
			{
				huff_size += 256;
				huff = (unsigned char*)realloc(huff, huff_size);
			}
		}
		// If only 1 colour then tile is solid colour so no data follows
		if (num_colours == 1)
		{
			memset(tile_data, huff[0], QCT_TILE_PIXELS);
		}
		else
		{
			// Validate Huffman table by ensuring all branches are within table
			// (if not just return so tile will be not be unpacked, ie. blank)
			{
				int ii, delta;
				for (ii=0; ii<huff_idx; ii++)
				{
					if (huff[ii] < 128)
						continue;
					else if (huff[ii] == 128)
					{
						if (ii+2 >= huff_idx)
							return;
						delta = 65537 - (256 * huff[ii+2] + huff[ii+1]) + 2;
						if (ii+delta >= huff_idx)
							return;
						ii += 2;
					}
					else
					{
						delta = 257 - huff[ii];
						if (ii+delta >= huff_idx)
							return;
					}
				}
			}
			// Read tile data one bit at a time following branches in Huffman tree
			int bits_left = 8;
			int bit_value;
			ii = fgetc(fp);
			while (pixelnum < QCT_TILE_PIXELS)
			{
				// If entry is a colour then output it
				if (*huff_ptr < 128)
				{
					tile_data[pixelnum++] = *huff_ptr;
					// Go back to top of tree for next pixel
					huff_ptr = huff;
					continue;
				}
				// Get the next "bit"
				bit_value = (ii & 1);
				// Prepare for the following one
				ii >>= 1;
				bits_left--;
				if (bits_left == 0)
				{
					ii = fgetc(fp);
					bits_left = 8;
				}
				// Now check value to see whether to follow branch or not
				if (bit_value == 0)
				{
					// Don't jump just proceed to next entry in Huffman table
					if (*huff_ptr == 128) huff_ptr+=2;
					huff_ptr++;
				}
				else
				{
					// Follow branch to different part of Huffman table
					if (*huff_ptr > 128)
					{
						// Near jump
						huff_ptr += 257 - (*huff_ptr);
					}
					else if (*huff_ptr == 128)
					{
						// Far jump needs two more bytes
						int delta = 65537 - (256 * huff_ptr[2] + huff_ptr[1]) + 2;
						huff_ptr += delta;
					}
					// NB < 128 already handled above
				}
			}
		}
	}

	else if (packing > 127)
	{
		// Pixel packing
		int num_sub_colours = 256 - packing;
		int shift = bits_per_pixel(num_sub_colours);
		int mask = (1 << shift) - 1;
		int num_pixels_per_word = 32 / shift;
		int palette_index[256];
		debugmsg("PACKED: sub-palette size is %d (%d bits)", num_sub_colours, shift);
		// Read the sub-palette
		for (ii=0; ii<num_sub_colours; ii++)
		{
			palette_index[ii] = fgetc(fp);
			debugmsg("PACKED: palette %d = %d", ii, palette_index[ii]);
		}
		// Read the pixels in 4-byte words and unpack the bits from each
		while (pixelnum < QCT_TILE_PIXELS)
		{
			int colour, runs;
			ii = readInt(fp);
			for (runs = 0; runs < num_pixels_per_word; runs++)
		 	{
				colour = ii & mask;
				ii = ii >> shift;
				tile_data[pixelnum++] = palette_index[colour];
			}
		}
	}

	else
	{
		// Run-length Encoding
		int num_sub_colours = packing;
		int num_low_bits = bits_per_pixel(num_sub_colours);
		int pal_mask = (1 << num_low_bits)-1;
		int palette_index[256];
		//debugmsg("RLE: sub-palette size is %d (uses %d bits) mask 0x%x", num_sub_colours, num_low_bits, pal_mask);
		for (ii=0; ii<num_sub_colours; ii++)
		{
			palette_index[ii] = fgetc(fp);
			//debugmsg("RLE palette %d = %d", ii, palette_index[ii]);
		}
		while (pixelnum < QCT_TILE_PIXELS)
		{
			int colour, runs;
			ii = fgetc(fp);
			colour = ii & pal_mask;
			runs = ii >> num_low_bits;
			//debugmsg("RLE value 0x%x is colour %d for %d runs [%d..%d]", ii, colour, runs, pixelnum, pixelnum+runs);
			while (runs-- > 0)
			{
				tile_data[pixelnum++] = palette_index[colour];
			}
		}
	}

	// Decommutate the interleaved rows and copy into the image
	{
		int yy;
		for (yy=0; yy<QCT_TILE_SIZE; yy++)
		{
			memcpy(row_ptr[yy], tile_data+yy*QCT_TILE_SIZE, QCT_TILE_SIZE);
		}
	}
}


bool
QCT::readFile(FILE *fp, int headeronly,unsigned char *data)
{
	int ii;

	// Read Metadata
	ii = readInt(fp);
	if (ii != QCT_MAGIC)
	{
		throwError("Not a QCT file (%x != %x)\n",ii,QCT_MAGIC);
		return false;
	}

	metadata.version = readInt(fp);
	width  = readInt(fp);
	height = readInt(fp);
	metadata.title      = readString(fp);
	metadata.name       = readString(fp);
	metadata.ident      = readString(fp);
	metadata.edition    = readString(fp);
	metadata.revision   = readString(fp);
	metadata.keywords   = readString(fp);
	metadata.copyright  = readString(fp);
	metadata.scale      = readString(fp);
	metadata.datum      = readString(fp);
	metadata.depths     = readString(fp);
	metadata.heights    = readString(fp);
	metadata.projection = readString(fp);
	metadata.flags      = readInt(fp);
	metadata.origfilename = readString(fp);
	metadata.origfilesize = readInt(fp);
	metadata.origfiletime = readInt(fp);
	metadata.unknown1     = readInt(fp);

	// Pointer to extended metadata
	{
		OFF_T current_position, extended_position;
		extended_position = readInt(fp);
		current_position = FTELLO(fp);
		FSEEKO(fp, extended_position, SEEK_SET);
		metadata.maptype = readString(fp);
		// Pointer to datum shift
		{
			OFF_T current_position, datum_shift_position;
			datum_shift_position = readInt(fp);
			current_position = FTELLO(fp);
			FSEEKO(fp, datum_shift_position, SEEK_SET);
			datum_shift_north = readDouble(fp);
			datum_shift_east  = readDouble(fp);
			FSEEKO(fp, current_position, SEEK_SET);
		}
		metadata.diskname = readString(fp);
		metadata.unknown2 = readInt(fp);
		metadata.unknown3 = readInt(fp);
		metadata.unknown4 = readInt(fp);
		metadata.unknown5 = readInt(fp);
		metadata.unknown6 = readInt(fp);
		FSEEKO(fp, current_position, SEEK_SET);
	}

	// Map outline
	num_outline = readInt(fp);  // number of map outline points
	outline_lat = (double*)calloc(num_outline, sizeof(double));
	outline_lon = (double*)calloc(num_outline, sizeof(double));
	// Pointer to map outline
	{
		int outline;
		OFF_T current_position, outline_position;
		outline_position = readInt(fp);
		current_position = FTELLO(fp);
		FSEEKO(fp, outline_position, SEEK_SET);
		for (outline=0; outline<num_outline; outline++)
		{
			outline_lat[outline] = readDouble(fp);
			outline_lon[outline] = readDouble(fp);
		}
		FSEEKO(fp, current_position, SEEK_SET);
	}

	// Georeferencing ceofficients

	eas = readDouble(fp);
	easY = readDouble(fp);
	easX = readDouble(fp);
	easYY = readDouble(fp);
	easXY = readDouble(fp);
	easXX = readDouble(fp);
	easYYY = readDouble(fp);
	easYYX = readDouble(fp);
	easYXX = readDouble(fp);
	easXXX = readDouble(fp);

	nor = readDouble(fp);
	norY = readDouble(fp);
	norX = readDouble(fp);
	norYY = readDouble(fp);
	norXY = readDouble(fp);
	norXX = readDouble(fp);
	norYYY = readDouble(fp);
	norYYX = readDouble(fp);
	norYXX = readDouble(fp);
	norXXX = readDouble(fp);

	lat = readDouble(fp);
	latX = readDouble(fp);
	latY = readDouble(fp);
	latXX = readDouble(fp);
	latXY = readDouble(fp);
	latYY = readDouble(fp);
	latXXX = readDouble(fp);
	latXXY = readDouble(fp);
	latXYY = readDouble(fp);
	latYYY = readDouble(fp);

	lon = readDouble(fp);
	lonX = readDouble(fp);
	lonY = readDouble(fp);
	lonXX = readDouble(fp);
	lonXY = readDouble(fp);
	lonYY = readDouble(fp);
	lonXXX = readDouble(fp);
	lonXXY = readDouble(fp);
	lonXYY = readDouble(fp);
	lonYYY = readDouble(fp);

	// Palette
	for (ii=0; ii<256; ii++)
	{
		palette[ii] = readInt(fp);
	}

	// Interpolation matrix (128 x 128)
	for (ii=0; ii<128*128; ii++)
	{
		fgetc(fp);
	}

	// Don't read and unpack image data if not required
	if (headeronly)
		return true;

	if(size_x == 0) size_x = width ;
	if(size_y == 0) size_y = height ;


	{
		int xx, yy;
		for (yy=0; yy<height; yy++)
		{
			debugmsg("  handling tiles (*,%d/%d)",yy,height) ;

			for (xx=0; xx<width; xx++)
			{
				OFF_T current_offset, tile_offset;
				tile_offset = readInt(fp);
				current_offset = FTELLO(fp);
					
				if(xx >= top_left_x && xx < top_left_x + size_x && yy >= top_left_y && yy < top_left_y + size_y)
				{
					FSEEKO(fp, tile_offset, SEEK_SET);
					//debugmsg("Would read tile %d, %d packing %d at offset %x", xx, yy, fgetc(fp), tile_offset);
					readTile(fp, xx, yy,data);

					FSEEKO(fp, current_offset, SEEK_SET);
				}
			}
		}
	}

	return(true);
}


bool
QCT::readFilename(const char *filename, int headeronly,unsigned char *data)
{
	FILE *fp;
	bool truth;

	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		throwError("cannot open %s (%s)", filename, strerror(errno));
		return false;
	}
	truth = readFile(fp, headeronly,data);
	fclose(fp);
	return(truth);
}


/* -------------------------------------------------------------------------
 */
void
QCT::printMetadata(FILE *fp)
{
	int ii;

	dfp = fp;
	verbose++;

	message("Version     %d", metadata.version);
	message("Width:      %d tiles (%d pixels)", width, width*QCT_TILE_SIZE);
	message("Height:     %d tiles (%d pixels)", height, height*QCT_TILE_SIZE);
	message("Title:      %s", metadata.title);
	message("Name:       %s", metadata.name);
	message("Identifier: %s", metadata.ident);
	message("Edition:    %s", metadata.edition);
	message("Revision:   %s", metadata.revision);
	message("Keywords:   %s", metadata.keywords);
	message("Copyright:  %s", metadata.copyright);
	message("Scale:      %s", metadata.scale);
	message("Datum:      %s", metadata.datum);
	message("Depths:     %s", metadata.depths);
	message("Heights:    %s", metadata.heights);
	message("Projection: %s", metadata.projection);
	message("Flags:      0x%x", metadata.flags);
	message("OriginalFileName:    %s", metadata.origfilename);
	message("OriginalFileSize     %d bytes", metadata.origfilesize);
	message("OriginalFileCreation %s", ctime(&metadata.origfiletime));
	message("MapType:    %s", metadata.maptype);
	message("DiskName:   %s", metadata.diskname);
	message("Unknown:    %d", metadata.unknown1);
	message("Unknown:    %d", metadata.unknown2);
	message("Unknown:    %d", metadata.unknown3);
	message("Unknown:    %d", metadata.unknown4);
	message("Unknown:    %d", metadata.unknown5);
	message("Unknown:    %d", metadata.unknown6);

	// Palette
	for (ii=0; ii<256; ii++)
	{
		if (palette[ii]) message("Colour %d = %6x", ii, palette[ii]);
	}

	// Outline
	message("OutlinePts: %d", num_outline);
	{
		double outline_lat_min = 99, outline_lat_max = -99;
		double outline_lon_min = 399, outline_lon_max = -399;
		FILE *kml = NULL;
		if (debug_kml_outline) kml = fopen("outline.kml", "w");
		if (kml) fprintf(kml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<kml xmlns=\"http://earth.google.com/kml/2.0\">\n"
			"<Document>\n"
			"<name>Outline</name>\n"
			"<description>Outline</description>\n"
			"<Style><LineStyle><color>ffffff00</color><width>6</width></LineStyle></Style>\n"
			"<Placemark>\n"
			"<name>Outline</name>\n"
			"<description>Outline</description>\n"
			"<LineString>\n"
			"<coordinates>");
		for (ii=0; ii<num_outline; ii++)
		{
			message(" %3.9f %3.9f", outline_lat[ii], outline_lon[ii]);
			if (kml) fprintf(kml, "%f,%f,%f ", outline_lon[ii], outline_lat[ii], 0.0);
			if (outline_lat[ii] > outline_lat_max) outline_lat_max = outline_lat[ii];
			if (outline_lat[ii] < outline_lat_min) outline_lat_min = outline_lat[ii];
			if (outline_lon[ii] > outline_lon_max) outline_lon_max = outline_lon[ii];
			if (outline_lon[ii] < outline_lon_min) outline_lon_min = outline_lon[ii];
		}
		if (kml)
		{
			fprintf(kml, "</coordinates>\n</LineString>\n</Placemark>\n</Document>\n</kml>\n");
			fclose(kml);
		}
		message("OutlineLat %f to %f", outline_lat_min, outline_lat_max);
		message("OutlineLon %f to %f", outline_lon_min, outline_lon_max);
	}

	// Georeferencing
	message("GeoTopLeftLonLat:    %f %f", lon, lat);
	message("GeoTopLeftEastNorth: %f %f", eas, nor);
	message("DatumShiftEastNorth: %f %f\n", datum_shift_east, datum_shift_north);

	// Calculate corner coordinates
	{
		double lat, lon;
		xy_to_latlon(0, 0, &lat, &lon);
		message("TL  %f, %f", lat, lon);
		xy_to_latlon(width*QCT_TILE_SIZE-1, 0, &lat, &lon);
		message("TR  %f, %f", lat, lon);
		xy_to_latlon(0, height*QCT_TILE_SIZE-1, &lat, &lon);
		message("BL  %f, %f", lat, lon);
		xy_to_latlon(width*QCT_TILE_SIZE-1, height*QCT_TILE_SIZE-1, &lat, &lon);
		message("BR  %f, %f", lat, lon);
	}
}


/* -------------------------------------------------------------------------
 */
#ifdef TO_REMOVE
bool
QCT::writePPMFile(FILE *fp)
{
	int xx, yy, colour;
	unsigned char *image_ptr = image_data;

	// PPM file header (for raw data not ASCII)
	fprintf(fp, "P6 %d %d 255\n",
		size_x*QCT_TILE_SIZE,
		size_y*QCT_TILE_SIZE);

	// Expand palette to R,G,B for each pixel
	for (yy=0; yy<size_y*QCT_TILE_SIZE; yy++)
	{
		for (xx=0; xx<size_x*QCT_TILE_SIZE; xx++)
		{
			colour = palette[*image_ptr++];
			fputc(PAL_RED(colour), fp);
			fputc(PAL_GREEN(colour), fp);
			fputc(PAL_BLUE(colour), fp);
		}
	}

	if (ferror(fp))
		return false;

	return(true);
}


bool
QCT::writePPMFilename(const char *filename)
{
	FILE *fp;
	bool truth;

	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		throwError("cannot open %s (%s)", filename, strerror(errno));
		return false;
	}
	truth = writePPMFile(fp);
	if (fclose(fp))
	{
		throwError("cannot write %s (%s)", filename, strerror(errno));
		truth = false;
	}
	return(truth);
}


/* -------------------------------------------------------------------------
 */
bool
QCT::writeGIFFile(FILE *fp)
{
#ifdef USE_GIFLIB
	unsigned char cmap[3][256];
	unsigned char *image_data_ptr = image_data;
	int background = 0;
	int transparent = -1;
	int xx, yy;

	for (xx=0; xx<256; xx++)
	{
		cmap[0][xx] = PAL_RED(palette[xx]);
		cmap[1][xx] = PAL_GREEN(palette[xx]);
		cmap[2][xx] = PAL_BLUE(palette[xx]);
	}

	gifout_open_file(fp, size_x*QCT_TILE_SIZE, size_y*QCT_TILE_SIZE, 256, cmap, background, transparent);
	gifout_open_image(0, 0, size_x*QCT_TILE_SIZE, size_y*QCT_TILE_SIZE);

	for (yy=0; yy<size_y*QCT_TILE_SIZE; yy++)
	{
		for (xx=0; xx<size_x*QCT_TILE_SIZE; xx++)
			gifout_put_pixel(*image_data_ptr++);
	}

	gifout_close_image();
	gifout_close_file();
	return true;
#else
	throwError("cannot write file (GIF not supported)");
	return false;
#endif
}


bool
QCT::writeGIFFilename(const char *filename)
{
	FILE *fp;
	bool truth;

	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		throwError("cannot open %s (%s)", filename, strerror(errno));
		return false;
	}
	truth = writeGIFFile(fp);
	if (fclose(fp))
	{
		throwError("cannot write %s (%s)", filename, strerror(errno));
		truth = false;
	}
	return(truth);
}

/* -------------------------------------------------------------------------
 */
bool
QCT::writeTIFFFile(FILE *fp)
{
#ifdef USE_TIFF
	throwError("cannot write file (TIFF not implemented)");
	return false;
#else
	throwError("cannot write file (TIFF not supported)");
	return false;
#endif
}


bool
QCT::writeTIFFFilename(const char *filename)
{
	FILE *fp;
	bool truth;

	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		throwError("cannot open %s (%s)", filename, strerror(errno));
		return false;
	}
	truth = writeTIFFFile(fp);
	if (fclose(fp))
	{
		throwError("cannot write %s (%s)", filename, strerror(errno));
		truth = false;
	}
	return(truth);
}


#endif

/* -------------------------------------------------------------------------
 */
static bool writePNGFile(FILE *fp, const unsigned char *data, const int *palette, int W, int H)
{
#ifdef USE_PNG
	int ii;

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr)
		return(false);

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return (false);
	}

	// Any errors inside png_ functions will return here
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		throw std::runtime_error("PNG file write error");
		return false;
	}

	png_init_io(png_ptr, fp);

	int bit_depth = 8;
	png_set_IHDR(png_ptr, info_ptr, W,H, bit_depth, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	int num_palette = 256;
	png_color pal[num_palette];
	for (ii=0; ii<256; ii++)
	{
		pal[ii].red   = PAL_RED(palette[ii]);
		pal[ii].green = PAL_GREEN(palette[ii]);
		pal[ii].blue  = PAL_BLUE(palette[ii]);
	}
	png_set_PLTE(png_ptr, info_ptr, pal, num_palette);

	png_byte *row_pointers[H];

	for (ii=0; ii<H; ii++)
		row_pointers[ii]=static_cast<png_byte*>(const_cast<unsigned char *>(data + W*ii));

	png_set_rows(png_ptr, info_ptr, row_pointers);

	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return true;
#else
	throwError("cannot write file (PNG not supported)");
	return false;
#endif
}


static bool writePNGFilename(const char *filename, const unsigned char *data, const int *palette, int W, int H)
{
	FILE *fp;
	bool truth;

	fp = fopen(filename, "wb");
	if (fp == NULL)
		throw std::runtime_error(std::string("cannot open ") + filename + " for writing.");

	truth = writePNGFile(fp,data,palette,W,H);

	if (fclose(fp))
		throw std::runtime_error(std::string("cannot write to file ") + filename + " (out of space?)");

	return(truth);
}


/* -------------------------------------------------------------------------
 * Convert pixel (x,y) in image into (longitude,latitude)
 * x,y from top left
 * latitude,longitude in degrees WGS84.
 * clips out of range x,y rather than blindly converting.
 */
int
QCT::xy_to_latlon(int x, int y, double *latitude, double *longitude)
{
	int x2, y2, x3, y3, xy;

	// Range check - clip to image limits, or allow extrapolation?
	if (x<0) x=0;
	if (y<0) y=0;
	if (x>=width*QCT_TILE_SIZE) x=width*QCT_TILE_SIZE-1;
	if (y>=height*QCT_TILE_SIZE) y=height*QCT_TILE_SIZE;

	x2 = x * x;
	x3 = x2 * x;
	y2 = y * y;
	y3 = y2 * y;
	xy = x * y;

	// Python:
	// coeffs in order are: c, cy, cx, cy2, cxy, cx2, cy3, cy2x, cyx2, cx3
	// There are coefficients for x and for y
	// each coordinate is calculated as
	// c + cy*y + cx*x + cy2*y^2 + cxy*x*y + cx2*x^2 + cy3*y^3 + cy2x*(y^2)*x + cyx2*y*x^2 + cx3*x^3
	// c0 + c1*y + c2*x + c3*y^2 + c4*x*y + c5*x^2 + c6*y^3 + c7*(y^2)*x + c8*y*x^2 + c9*x^3

	// Touratech:
	// Substitute into a and b as follows:
	// 	x and y to latitude or longitude: x and y
	// 	lat and lon to x or y: lat and lon

	// x = c0 + c1*x + c2*y + c3*x2 + c4*x*y + c5*y2 + c6*x3 + c7*x2*y + c8*x*y2 + c9*y3
	// y = c0 + c1*x + c2*y + c3*x2 + c4*x*y + c5*y2 + c6*x3 + c7*x2*y + c8*x*y2 + c9*y3
	// lat = c0 + c1*x + c2*y + c3*x2 + c4*x*y + c5*y2 + c6*x3 + c7*x2*y + c8*x*y2 + c9*y3
	// lon = c0 + c1*x + c2*y + c3*x2 + c4*x*y + c5*y2 + c6*x3 + c7*x2*y + c8*x*y2 + c9*y3

	// New equations (1.01) :
	// eas, easY, easX, easYY, easXY, easXX, easYYY, easYYX, easYXX, easXXX;
	// nor, norY, norX, norYY, norXY, norXX, norYYY, norYYX, norYXX, norXXX;
	// lat, latX, latY, latXX, latXY, latYY, latXXX, latXXY, latXYY, latYYY;
	// lon, lonX, lonY, lonXX, lonXY, lonYY, lonXXX, lonXXY, lonXYY, lonYYY;
	*longitude = lon + lonX * x + lonY * y + lonXX * x2 + lonXY * x * y + lonYY * y2 + lonXXX * x3 + lonXXY * x2 * y + lonXYY * x * y2 + lonYYY * y3;
	*latitude  = lat + latX * x + latY * y + latXX * x2 + latXY * x * y + latYY * y2 + latXXX * x3 + latXXY * x2 * y + latXYY * x * y2 + latYYY * y3;

	// Add the datum shift
	*longitude += datum_shift_east;
	*latitude  += datum_shift_north;

	return(0);
}

int QCT::computeLatLonLimits(double& lat_00, double& lon_00,
                            double& lat_10, double& lon_10,
                            double& lat_01, double& lon_01,
                            double& lat_11, double& lon_11 )
{
	xy_to_latlon( top_left_x          *QCT_TILE_SIZE, top_left_y          *QCT_TILE_SIZE, &lat_01, &lon_01);
	xy_to_latlon((top_left_x + size_x)*QCT_TILE_SIZE, top_left_y          *QCT_TILE_SIZE, &lat_11, &lon_11);
	xy_to_latlon( top_left_x          *QCT_TILE_SIZE,(top_left_y + size_y)*QCT_TILE_SIZE, &lat_00, &lon_00);
	xy_to_latlon((top_left_x + size_x)*QCT_TILE_SIZE,(top_left_y + size_y)*QCT_TILE_SIZE, &lat_10, &lon_10);

    return true;
}

/* -------------------------------------------------------------------------
 * Test program.
 */
int
main(int argc, char *argv[])
{
	char *prog;
	const char *options = "dVqu:v:x:y:i:o:k:b:";
	const char *usage = "usage: %s [-d] [-v] [-q] [-x SIZE] [-y SIZE] -i map.qct [-o map.png] [-k map.kmz]\n"
		"-d\tdebug\n"
		"-V\tverbose\n"
		"-q\tquery metadata only, no image extracted\n"
		"-u,v\tcoordinate of top left tile to start from (default: 0,0)\n"
		"-x,y\tmaximum size of the output in tiles (0 means unlimited)\n"
		"-i\tinput filename (qct format)\n"
		"-o\toutput filename (png format)\n"
		"-k\toutput filename (kmz format)\n";
	int debug = 0;
	int verbose = 0;
	int query = 0;
	int top_left_tile_x=0,top_left_tile_y=0;
	int size_tile_x=0,size_tile_y=0;
    int block_size=0;
	char *inputfile = NULL;
	char *outputfile = NULL;
    char *outputKMZfile = NULL;
	int c;

	prog = argv[0];
	GETOPT(c, options)
	{
		case 'd': debug++; break;
		case 'V': verbose++; break;
		case 'q': query++; break;
		case 'i': inputfile = optarg; break;
		case 'o': outputfile = optarg; break;
		case 'k': outputKMZfile = optarg; break;
		case 'u': sscanf(optarg,"%d",&top_left_tile_x) ; break ;
		case 'v': sscanf(optarg,"%d",&top_left_tile_y) ; break ;
		case 'x': sscanf(optarg,"%d",&size_tile_x) ; break ;
		case 'y': sscanf(optarg,"%d",&size_tile_y) ; break ;
		case 'b': sscanf(optarg,"%d",&block_size) ; break ;
		default: fprintf(stderr, usage, prog); exit(1);
	}

	if (inputfile == NULL)
	{
		fprintf(stderr, "%s: missing input file\n", prog);
		fprintf(stderr, usage, prog);
		exit(1);
	}
	if (!query && !outputfile)
	{
		fprintf(stderr, "%s: missing -q or -o option\n", prog);
		fprintf(stderr, usage, prog);
		exit(1);
	}

	QCT qct;
	qct.setDebug(debug);
	qct.setVerbose(verbose);
	qct.setBounds(top_left_tile_x,top_left_tile_y,size_tile_x,size_tile_y);

    // read header so as to init tile size etc.

	qct.readFilename(inputfile,true,NULL) ;

    if(query)
        return 1;

	if(outputfile)
		qct.message("Reading tiles (%d, %d) + %dx%d into image \"%s\"\n", top_left_tile_x,top_left_tile_y, size_tile_x,size_tile_y,outputfile);

	// Image index (width * height pointers)
    // Allocate image data once for all

	unsigned char *image_data = (unsigned char*)calloc(size_tile_y*QCT_TILE_SIZE, size_tile_x*QCT_TILE_SIZE);

	if (query)
	{
		qct.printMetadata(stdout);
	}
	else if (outputfile)
		writePNGFilename(outputfile,image_data,qct.getPalette(),size_tile_x*QCT_TILE_SIZE,size_tile_y*QCT_TILE_SIZE);

	if (outputKMZfile)
    {
        KmzFile kmz ;

        uint32_t block_index = 0 ;

        const int block_size_x = (block_size>0)?block_size:size_tile_x ;
        const int block_size_y = (block_size>0)?block_size:size_tile_y ;

        int n_maps_i = (size_tile_x + block_size_x - 1)/block_size_x ;
        int n_maps_j = (size_tile_y + block_size_y - 1)/block_size_y ;

		for(uint32_t j=0;j<n_maps_j;++j)
			for(uint32_t i=0;i<n_maps_i;++i)
			{
				double lat_00, lon_00;
				double lat_10, lon_10;
				double lat_01, lon_01;
				double lat_11, lon_11;

				qct.setBounds(top_left_tile_x + block_size_x*i,top_left_tile_y + block_size_y*j,block_size_x,block_size_y);

				qct.computeLatLonLimits(
				            lat_00, lon_00,
				            lat_10, lon_10,
				            lat_01, lon_01,
				            lat_11, lon_11
				            );
#ifdef DEBUG
				std::cerr << "BLock " << block_index << ": got corners:" << std::endl;
				std::cerr << "  " << lat_00 << " " << lon_00 << std::endl;
				std::cerr << "  " << lat_10 << " " << lon_10 << std::endl;
				std::cerr << "  " << lat_11 << " " << lon_11 << std::endl;
				std::cerr << "  " << lat_01 << " " << lon_01 << std::endl;
#endif

				// compute width and height for testing

				float x1 = 0.5*(lat_11+lat_10) - 0.5*(lat_01+lat_00);
				float y1 = 0.5*(lon_11+lon_10) - 0.5*(lon_01+lon_00);
				float x2 =-0.5*(lat_00+lat_10) + 0.5*(lat_01+lat_11);
				float y2 =-0.5*(lon_00+lon_10) + 0.5*(lon_01+lon_11);

				// compute rotation of the overlay

				float center_lat = 0.25*(lat_00+lat_01+lat_10+lat_11) ;
				float center_lon = 0.25*(lon_00+lon_01+lon_10+lon_11) ;

				qct.debugmsg("width: %f, height: %f\n",sqrtf(x1*x1+y1*y1) * cos(center_lat*M_PI/180), sqrtf(x2*x2+y2*y2));

#ifdef DO_ROTATION
				float angle = atan2( 0.5*(lat_11+lat_10) - center_lat, 0.5*(lon_11+lon_10) - center_lon);

				// we make a rotation of -angle

				float cos_angle = cos(-angle);
				float sin_angle = sin(-angle);

				float new_lon_00 = center_lon + cos_angle * (lon_00 - center_lon) - sin_angle * (lat_00 - center_lat) ;
				float new_lat_00 = center_lat + sin_angle * (lon_00 - center_lon) + cos_angle * (lat_00 - center_lat) ;

				float new_lon_10 = center_lon + cos_angle * (lon_10 - center_lon) - sin_angle * (lat_10 - center_lat) ;
				float new_lat_10 = center_lat + sin_angle * (lon_10 - center_lon) + cos_angle * (lat_10 - center_lat) ;

				float new_lon_11 = center_lon + cos_angle * (lon_11 - center_lon) - sin_angle * (lat_11 - center_lat) ;
				float new_lat_11 = center_lat + sin_angle * (lon_11 - center_lon) + cos_angle * (lat_11 - center_lat) ;

				float new_lon_01 = center_lon + cos_angle * (lon_01 - center_lon) - sin_angle * (lat_01 - center_lat) ;
				float new_lat_01 = center_lat + sin_angle * (lon_01 - center_lon) + cos_angle * (lat_01 - center_lat) ;

				std::cerr << "Angle: " << 180 /M_PI * angle << " deg." << std::endl;

				std::cerr << "After rotation: " << new_lat_00 << " " << new_lon_00 << std::endl;
				std::cerr << "After rotation: " << new_lat_10 << " " << new_lon_10 << std::endl;
				std::cerr << "After rotation: " << new_lat_11 << " " << new_lon_11 << std::endl;
				std::cerr << "After rotation: " << new_lat_01 << " " << new_lon_01 << std::endl;

				KmzFile::LayerData ld ;
				ld.rotation = -180/M_PI*angle ;					// angles are counter clockwise, so we flip sign here
				ld.north_limit = 0.5*(new_lat_01+new_lat_11) ;
				ld.south_limit = 0.5*(new_lat_00+new_lat_10) ;
				ld.east_limit  = 0.5*(new_lon_10+new_lon_11) ;
				ld.west_limit  = 0.5*(new_lon_00+new_lon_01) ;
#else
				KmzFile::LayerData ld ;
				ld.rotation = 0.0;
				ld.north_limit = 0.5*(lat_01+lat_11) ;
				ld.south_limit = 0.5*(lat_00+lat_10) ;
				ld.east_limit  = 0.5*(lon_10+lon_11) ;
				ld.west_limit  = 0.5*(lon_00+lon_01) ;
#endif

                // read image data

				if(!qct.readFilename(inputfile,query,image_data))
					return 0;

                // save it into a proper png file

                char output_template[100];
                sprintf(output_template,"image_data_%04d.png",block_index);

				writePNGFilename(output_template,image_data,qct.getPalette(),block_size_x*QCT_TILE_SIZE,block_size_y*QCT_TILE_SIZE);

                std::cerr << "Block " << block_index << " ("<< i << ","<< j << ") : [" << ld.south_limit << "," << ld.north_limit << "] x [" << ld.west_limit << "," << ld.east_limit << "]" << std::endl;
#ifdef DEBUG
				std::cerr << "north-south in km: " << (ld.north_limit - ld.south_limit)*M_PI/180*6378 << std::endl;
				std::cerr << "east-west   in km: " << (ld.east_limit  - ld.west_limit )*M_PI/180*6378 * cos(center_lat*M_PI/180) << std::endl;
#endif
				kmz.layers.push_back(ld);

                ++block_index;
			}

        kmz.writeToFile("doc.kml");
    }

	return(0);
}
