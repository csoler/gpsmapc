#pragma once

#include <QMainWindow>

class MapAccessor ;
class MapViewer ;

class MapGUIWindow : public QMainWindow
{
    Q_OBJECT

public:
    MapGUIWindow();
    virtual ~MapGUIWindow();

    void setMapAccessor(MapAccessor &ma);

private:
    MapViewer *mViewer ;
};



