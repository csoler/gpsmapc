#include <QApplication>

#include "config.h"
#include "MapGUIWindow.h"
#include "MapDB.h"

int main(int argc,char *argv[])
{
	QApplication IGNMapperApp(argc,argv);

	MapGUIWindow map_gui_win;
	map_gui_win.show();

    // now create the map

    MapDB mapdb(MAP_ROOT_DIRECTORY) ;

	return IGNMapperApp.exec();

}
