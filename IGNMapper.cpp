#include <GL/glut.h>
#include <QApplication>

#include "config.h"
#include "MapGUIWindow.h"
#include "MapDB.h"
#include "MapAccessor.h"

int main(int argc,char *argv[])
{
	QApplication IGNMapperApp(argc,argv);

	MapGUIWindow map_gui_win;
	map_gui_win.show();

    glutInit(&argc,argv);

    // now create the map

    MapDB mapdb(MAP_ROOT_DIRECTORY) ;
    MapAccessor ma(mapdb) ;

    map_gui_win.setMapAccessor(ma);

	return IGNMapperApp.exec();
}
