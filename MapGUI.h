#pragma once

#include <QMainWindow>
#include "ui_MapGUIWindow.h"


class MapGUIWindow : public QMainWindow, Ui::MapGUIWindow
{
    Q_OBJECT

public:
    MapGUIWindow();
    virtual ~MapGUIWindow();
};



