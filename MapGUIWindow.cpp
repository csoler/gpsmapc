#include <iostream>

#include "MapViewer.h"
#include "MapGUIWindow.h"
#include "ui_MapGUIWindow.h"

MapGUIWindow::MapGUIWindow() : QMainWindow()
{
    setCentralWidget(mViewer = new MapViewer(this));
    std::cerr << "Creating a MapGUI Window" << std::endl;
}

MapGUIWindow::~MapGUIWindow()
{
}

void MapGUIWindow::setMapAccessor(MapAccessor& ma)
{
    mViewer->setMapAccessor(&ma);
}
