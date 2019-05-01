#include <QApplication>

#include "MapGUI.h"

int main(int argc,char *argv[])
{
	QApplication IGNMapperApp(argc,argv);

	MapGUIWindow map_gui_win;
	map_gui_win.show();

	return IGNMapperApp.exec();
}
