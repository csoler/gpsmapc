#include <iostream>

#include <QFile>
#include <QTextStream>
#include <QtXml>

#include "config.h"
#include "MapDB.h"

MapDB::MapDB(const std::string& directory_name)
    : mRootDirectory(QString::fromStdString(directory_name))
{
    mMapInited = init();
}

void MapDB::createEmptyMap(QFile& map_file)
{
    if (!map_file.open(QIODevice::WriteOnly ))
        throw std::runtime_error("cannot open file " + map_file.fileName().toStdString() + " for writing.");

    QTextStream o(&map_file);

	o << "<xml>" << endl;
	o << "	<Map name=\"Empty map\" update_time=\"431437198057\" longitude_min=\"0.0\" longitude_max=\"1.0\"/>" << endl;
	o << "</xml>" << endl;

	map_file.close();
}

bool MapDB::init()
{
    try
    {
		loadDB(QString(mRootDirectory),QString(MAP_DEFINITION_FILE_NAME));
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
// <xml>
//		<Map name="some name" update_time="431437198057" longitude_min="0.0" longitude_max="1.0">
//			<ImageData file="somefile.jpg" scale="22.3">
//				<DataPoint img_coord_i="34" img_coord_j="223" latitude="1.39438" longitude="4.2323"/>
//				<InterestPoint img_coord_i="34" img_coord_j="223" sift="ZGZrYXZud3dudmluaW9iO25mb3Zmczt2ZnNpZG8K="/>
//				<InterestPoint img_coord_i="14" img_coord_j="146" sift="YzkwMnV0OTBtdTkwbXR1OTBtYzBteGE5YTBtY2ltdml0bXZ3Cg=="/>
//			</ImageData>
//		</map>
// </xml>

void MapDB::loadDB(const QString& source_directory,const QString& source_file)
{
    QDomDocument xmldoc;
    QFile f( source_directory + "/" + source_file);

    if (!f.open(QIODevice::ReadOnly ))
    {
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
        throw std::runtime_error("Wong file type: \"Map\" expected.");

    mName = root.attribute("nname","[No name]");
    mCreationTime = root.attribute("update_time","0").toULongLong();

    std::cerr << "Reading map \"" << mName.toStdString() << "\" creation time: " << QDateTime::fromSecsSinceEpoch(mCreationTime).toString().toStdString() << std::endl;
}
