#include <iostream>

#include <QFile>
#include <QTextStream>
#include <QtXml>
#include <QImage>

#include "config.h"
#include "MapDB.h"

std::ostream& operator<<(std::ostream& o, const MapDB::GPSCoord& c)
{
    return o << "(" << c.lon << "," << c.lat << ")" ;
}

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
		loadDB(mRootDirectory);

        checkDirectory(mRootDirectory) ;

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

    mTopLeft.lon = root.toElement().attribute("longitude_min","0.0").toFloat();
    mTopLeft.lat = root.toElement().attribute("latitude_min","0.0").toFloat();
    mBottomRight.lon = root.toElement().attribute("longitude_max","0.0").toFloat();
    mBottomRight.lat = root.toElement().attribute("latitude_max","0.0").toFloat();

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
			image.top_left_corner.lon = e.attribute("CornerLon","0.0").toFloat();
			image.top_left_corner.lat = e.attribute("CornerLat","0.0").toFloat();
			image.scale = e.attribute("Scale","10000").toInt();

            mImages[filename] = image;
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
		if(dir[i].endsWith(".jpg"))
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
                image.scale = 10000 ;
                image.top_left_corner.lon = 0.0 ;
                image.top_left_corner.lat = 0.0 ;

                mImages[dir[i]] = image;

                mMapChanged = true;
            }
            else
                std::cerr << "  already in the database. Coordinates: " << it->second.top_left_corner << std::endl;
        }

    mMapChanged = false;
}

void MapDB::saveDB(const QString& directory)
{
    QString filename(directory + "/" + MAP_DEFINITION_FILE_NAME);

    std::cerr << "Saving DB to file " << filename.toStdString() << std::endl;

    QDomDocument doc ;

	QDomElement e = doc.createElement(QString("Map")) ;

	e.setAttribute("creation_time",qulonglong(time(NULL)));
	e.setAttribute("longitude_min",mTopLeft.lon) ;
	e.setAttribute("longitude_max",mBottomRight.lon) ;
	e.setAttribute("latitude_min",mTopLeft.lat) ;
	e.setAttribute("latitude_max",mBottomRight.lat);

    for(auto it(mImages.begin());it!=mImages.end();++it)
	{
		QDomElement ep = doc.createElement(QString("Image"));

		ep.setAttribute("Filename",it->first);
		ep.setAttribute("Width",it->second.W);
		ep.setAttribute("Height",it->second.H);
		ep.setAttribute("CornerLon",it->second.top_left_corner.lon);
		ep.setAttribute("CornerLat",it->second.top_left_corner.lat);
		ep.setAttribute("Scale",it->second.scale);

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









