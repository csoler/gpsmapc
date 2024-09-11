#include <GL/glut.h>
#include <QApplication>

#include "config.h"
#include "MapGUIWindow.h"
#include "ScreenshotCollectionMapDB.h"
#include "MapAccessor.h"

int main(int argc,char *argv[])
{
    glutInit(&argc,argv);
	QApplication IGNMapperApp(argc,argv);

	MapGUIWindow map_gui_win;
	map_gui_win.show();

    // now create the map

    ScreenshotCollectionMapDB mapdb(MAP_ROOT_DIRECTORY) ;
    MapAccessor ma(mapdb) ;

    map_gui_win.setMapAccessor(ma);

	return IGNMapperApp.exec();
}
