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

MapDB::MapDB(const QString& directory_name)
    : mRootDirectory(directory_name)
{
    mMapInited = init();
    mMapChanged = false;
}

void MapDB::createEmptyMap(QFile& map_file)
{
    if (!map_file.open(QIODevice::WriteOnly ))
        throw std::runtime_error("cannot open file " + map_file.fileName().toStdString() + " for writing.");

    QTextStream o(&map_file);

	o << "<Map name=\"Empty map\" update_time=\"431437198057\" longitude_min=\"0.0\" longitude_max=\"1.0\"/>" << endl;

	map_file.close();
}

bool MapDB::init()
{
    try
    {
        mBottomLeft.x     =  FLT_MAX;
        mBottomLeft.y     =  FLT_MAX;
        mTopRight.x = -FLT_MAX;
        mTopRight.y = -FLT_MAX;

		QApplication::setOverrideCursor(Qt::WaitCursor);
        QCoreApplication::processEvents();

		loadDB(mRootDirectory);
        checkDirectory(mRootDirectory) ;

        std::cerr << "Database read. Top left corner: " << mBottomLeft << ", Bottom right corner: " << mTopRight << std::endl;

		QApplication::setOverrideCursor(Qt::ArrowCursor);

        if(mMapChanged)
            saveDB(mRootDirectory);
    }
    catch(std::runtime_error& e)
    {
        std::cerr << "Cannot read database file " << MAP_DEFINITION_FILE_NAME << ": " << e.what() << std::endl;
        return false;
    }
    return true;
}

void MapDB::includeImage(const MapDB::ImageSpaceCoord& top_left_corner,int W,int H)
{
    if(mBottomLeft.x > top_left_corner.x) mBottomLeft.x = top_left_corner.x;
    if(mBottomLeft.y > top_left_corner.y) mBottomLeft.y = top_left_corner.y;

    if(mTopRight.x < top_left_corner.x+W) mTopRight.x = top_left_corner.x+W;
    if(mTopRight.y < top_left_corner.y+H) mTopRight.y = top_left_corner.y+H;
}


// DB file structure:
//
//	<Map name="some name" update_time="431437198057" longitude_min="0.0" longitude_max="1.0">
//		<ImageData file="somefile.jpg" scale="22.3">
//			<DataPoint img_coord_i="34" img_coord_j="223" latitude="1.39438" longitude="4.2323"/>
//			<InterestPoint img_coord_i="34" img_coord_j="223" sift="ZGZrYXZud3dudmluaW9iO25mb3Zmczt2ZnNpZG8K="/>
//			<InterestPoint img_coord_i="14" img_coord_j="146" sift="YzkwMnV0OTBtdTkwbXR1OTBtYzBteGE5YTBtY2ltdml0bXZ3Cg=="/>
//		</ImageData>
//	</map>

void MapDB::loadDB(const QString& source_directory)
{
    QString source_file(MAP_DEFINITION_FILE_NAME);

    QDomDocument xmldoc;
    QFile f( source_directory + "/" + source_file);

    if (!f.open(QIODevice::ReadOnly ))
    {
        std::cerr << "Creating base directory " << source_directory.toStdString() << std::endl;

        if(!QDir(source_directory).exists())
            QDir(QDir::currentPath()).mkdir(source_directory);

        createEmptyMap(f) ;

		if (!f.open(QIODevice::ReadOnly ))
            throw std::runtime_error("Cannot create local map file");
    }

    xmldoc.setContent(&f);
    f.close();

    // Extract the root markup

    QDomElement root=xmldoc.documentElement();

    // Get root names and attributes
    QString type=root.tagName();

    if(type != "Map")
        throw std::runtime_error("Wong file type: \"" + type.toStdString() + "\" found, \"Map\" expected.");

    mName = root.attribute("name","[No name]");
    mCreationTime = root.attribute("update_time","0").toULongLong();

    std::cerr << "Reading map \"" << mName.toStdString() << "\" creation time: " << QDateTime::fromSecsSinceEpoch(mCreationTime).toString().toStdString() << std::endl;

    mImages.clear();
    QDomNode ep = root.firstChild();

    while(!ep.isNull())
	{
        if(ep.toElement().tagName() == "Image")
        {
            RegisteredImage image ;
            QDomElement e = ep.toElement();
            ep = ep.nextSibling();

			QString filename = e.attribute("Filename",QString());

            if(!QFile(mRootDirectory + "/" + filename).exists())
            {
                std::cerr << "Warning: database mentions file " << filename.toStdString() << " but it wasn't found." << std::endl;
                continue;
            }

			image.W = e.attribute("Width","0").toInt();
			image.H = e.attribute("Height","0").toInt();
            image.bottom_left_corner.x = e.attribute("BottomLeftImageSpaceX","0.0").toFloat();
            image.bottom_left_corner.y = e.attribute("BottomLeftImageSpaceY","0.0").toFloat();

            includeImage(image.bottom_left_corner,image.W,image.H) ;

            mImages[filename] = image;
		}

        if(ep.toElement().tagName() == "ReferencePoint")
        {
            QDomElement e = ep.toElement();
            ep = ep.nextSibling();

            mReferencePoint2 = mReferencePoint1;

            mReferencePoint1.filename = e.attribute("Filename",QString());
            mReferencePoint1.x = e.attribute("ImageX","-1").toInt();
            mReferencePoint1.y = e.attribute("ImageY","-1").toInt();
            mReferencePoint1.lat = e.attribute("Latitude","0.0").toFloat();
            mReferencePoint1.lon = e.attribute("Longitude","0.0").toFloat();
        }
	}

    // [...]
}

void MapDB::checkDirectory(const QString& source_directory)
{
    // now load all images in the source directory and see if they are already in the DB. If not, add them.

    QDir dir(source_directory) ;

    std::cerr << "Now scanning source directory \"" << source_directory.toStdString() << "\"" << std::endl;

    for(uint32_t i=0;i<dir.count();++i)
    {
        if(dir[i] == "mask.png")
        {
            std::cerr << "  Found image mask file " << dir[i].toStdString() ;

            mImagesMask = dir[i];
        }
		else if(dir[i].endsWith(".jpg"))
        {
            std::cerr << "  Checking image file " << dir[i].toStdString() ;
            auto it = mImages.find(dir[i]);

            if(mImages.end() == it)
            {
                std::cerr << "  not in the database. Adding!" << std::endl;
                QImage img_data(source_directory + "/" + dir[i]) ;

                RegisteredImage image ;
                image.W = img_data.width();
                image.H = img_data.height();
                image.bottom_left_corner = MapDB::ImageSpaceCoord(0.0,0.0);

                mImages[dir[i]] = image;

                includeImage(image.bottom_left_corner,image.W,image.H);
                mMapChanged = true;
            }
            else
                std::cerr << "  already in the database. Coordinates: " << it->second.bottom_left_corner << std::endl;
        }
    }

    if(mImagesMask.isNull())
        std::cerr << "No mask image found. If you want to add one, name is mask.png.";

    mMapChanged = false;
}

static QDomElement convertRefPointToDom(QDomDocument& doc,const MapDB::ReferencePoint& rp)
{
    QDomElement e = doc.createElement(QString("ReferencePoint"));

    e.setAttribute("Filename",rp.filename);
    e.setAttribute("ImageX",QString::number(rp.x));
    e.setAttribute("ImageY",QString::number(rp.y));
    e.setAttribute("Latitude",QString::number(rp.lat));
    e.setAttribute("Longitude",QString::number(rp.lon));

	return e;
}

void MapDB::saveDB(const QString& directory)
{
    QString filename(directory + "/" + MAP_DEFINITION_FILE_NAME);

    std::cerr << "Saving DB to file " << filename.toStdString() << std::endl;

    QDomDocument doc ;

	QDomElement e = doc.createElement(QString("Map")) ;

	e.setAttribute("creation_time",qulonglong(time(NULL)));
    e.setAttribute("longitude_min",mBottomLeft.x) ;
    e.setAttribute("longitude_max",mTopRight.x) ;
    e.setAttribute("latitude_min",mBottomLeft.y) ;
    e.setAttribute("latitude_max",mTopRight.y);

    if(!mReferencePoint1.filename.isNull()) e.appendChild(convertRefPointToDom(doc,mReferencePoint1));
    if(!mReferencePoint2.filename.isNull()) e.appendChild(convertRefPointToDom(doc,mReferencePoint2));

    for(auto it(mImages.begin());it!=mImages.end();++it)
	{
		QDomElement ep = doc.createElement(QString("Image"));

		ep.setAttribute("Filename",it->first);
		ep.setAttribute("Width",it->second.W);
		ep.setAttribute("Height",it->second.H);
        ep.setAttribute("BottomLeftImageSpaceX",it->second.bottom_left_corner.x);
        ep.setAttribute("BottomLeftImageSpaceY",it->second.bottom_left_corner.y);

		e.appendChild(ep);
	}
    doc.appendChild(e);

    QFile dataFile(filename);

    if(dataFile.open(QIODevice::WriteOnly))
	{
		QTextStream dataStream(&dataFile);
		doc.save(dataStream, 2);
		dataFile.flush();
		dataFile.close();
	}
    else
		std::cerr << "Error: cannot write to file " << filename.toStdString() << std::endl;

    mMapChanged = false ;
}

void MapDB::moveImage(const QString& filename,float delta_is_x,float delta_is_y)
{
    auto it = mImages.find(filename) ;

    if(it == mImages.end())
    {
        std::cerr << __PRETTY_FUNCTION__ << ": cannot find image " << filename.toStdString() << std::endl;
        return;
    }

    it->second.bottom_left_corner.x += delta_is_x;
    it->second.bottom_left_corner.y += delta_is_y;

    mMapChanged = true;
}

void MapDB::placeImage(const QString& image_filename, const ImageSpaceCoord &new_corner)
{
	auto it = mImages.find(image_filename) ;

    if(it == mImages.end())
    {
        std::cerr << __PRETTY_FUNCTION__ << ": cannot find image " << image_filename.toStdString() << std::endl;
        return;
    }

    it->second.bottom_left_corner = new_corner;

    mMapChanged = true;
}

void MapDB::save()
{
	saveDB(mRootDirectory);
    std::cerr << "Map saved!" << std::endl;
}

void MapDB::recomputeDescriptors(const QString& image_filename)
{
    auto it = mImages.find(image_filename);

    if(mImages.end() == it)
        return ;

    //QImage image(mRootDirectory + "/" + image_filename);
    //if(image.width() != it->second.W || image.height() != it->second.H)
    //{
    //    std::cerr << "Error: cannot load image data for " << image_filename.toStdString() << std::endl;
    //    return;
    //}

    std::cerr << "Computing descriptors for image " << image_filename.toStdString() << "..." << std::endl;

    //MapRegistration::findDescriptors(image.bits(),it->second.W,it->second.H,it->second.descriptors);
    MapRegistration::findDescriptors( (mRootDirectory+"/"+image_filename).toStdString(),it->second.descriptors);
}

bool MapDB::getImageParams(const QString& image_filename,MapDB::RegisteredImage& img) const
{
	auto it = mImages.find(image_filename);

    if(mImages.end() == it)
        return false;

    img = it->second;
    return true;
}

void MapDB::setReferencePoint(const QString& image_name,int point_x,int point_y)
{
    mReferencePoint2 = mReferencePoint1;

    mReferencePoint1.filename = image_name ;
    mReferencePoint1.x = point_x ;
    mReferencePoint1.y = point_y ;
    mReferencePoint1.lat = 0 ;
    mReferencePoint1.lon = 0 ;
}

int MapDB::numberOfReferencePoints() const
{
    if(mReferencePoint1.filename.isNull())
        return 0;

    if(mReferencePoint2.filename.isNull())
        return 1;

    return 2;
}

bool MapDB::imageSpaceCoordinatesToGPSCoordinates(const MapDB::ImageSpaceCoord& ic,MapDB::GPSCoord& g) const
{
    if(mReferencePoint1.filename.isNull() || mReferencePoint2.filename.isNull())
        return false;

    auto it1 = mImages.find(mReferencePoint1.filename) ;
    auto it2 = mImages.find(mReferencePoint2.filename) ;

    if(it1 == mImages.end() || it2 == mImages.end())
        return false;

    // Compute the image space coordinates of the two reference points.

    MapDB::ImageSpaceCoord p1(it1->second.bottom_left_corner.x + mReferencePoint1.x,it1->second.bottom_left_corner.y + it1->second.H - 1 - mReferencePoint1.y) ;
    MapDB::ImageSpaceCoord p2(it2->second.bottom_left_corner.x + mReferencePoint2.x,it2->second.bottom_left_corner.y + it1->second.H - 1 - mReferencePoint2.y) ;

    if(p1.x == p2.x || p1.y == p2.y)
        return false;

    // now compute the actual GPS coordinates of ic

    g.lon = mReferencePoint1.lon + (ic.x - p1.x)/(p2.x - p1.x)*(mReferencePoint2.lon - mReferencePoint1.lon) ;
    g.lat = mReferencePoint1.lat + (ic.y - p1.y)/(p2.y - p1.y)*(mReferencePoint2.lat - mReferencePoint1.lat) ;

    return true;
}






