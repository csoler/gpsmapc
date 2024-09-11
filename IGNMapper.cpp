#include <GL/glut.h>
#include <QApplication>

#include "argstream.h"
#include "config.h"
#include "MapGUIWindow.h"
#include "ScreenshotCollectionMapDB.h"
#include "QctMapDB.h"
#include "MapAccessor.h"

int main(int argc,char *argv[])
{
    argstream as(argc,argv);

    std::string qct_file;

    as >> parameter('q',"qct",qct_file,"Qct IGN file",false)
       >> help();

    as.defaultErrorHandling();

    glutInit(&argc,argv);
	QApplication IGNMapperApp(argc,argv);

	MapGUIWindow map_gui_win;
	map_gui_win.show();

    // now create the map

    if(!qct_file.empty())
    {
        QctMapDB mapdb(QString::fromStdString(qct_file)) ;
        MapAccessor ma(mapdb) ;
        map_gui_win.setMapAccessor(ma);
    }
    else
    {
        ScreenshotCollectionMapDB mapdb(MAP_ROOT_DIRECTORY) ;
        MapAccessor ma(mapdb) ;
        map_gui_win.setMapAccessor(ma);
    }

	return IGNMapperApp.exec();
}
