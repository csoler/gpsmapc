#include "MapGUI.h"
#include "ui_MapGUIWindow.h"

MapGUIWindow::MapGUIWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MapGUIWindow)
{
    ui->setupUi(this);
}

MapGUIWindow::~MapGUIWindow()
{
    delete ui;
}
