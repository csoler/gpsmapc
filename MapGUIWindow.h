#ifndef MAPGUIWINDOW_H
#define MAPGUIWINDOW_H

#include <QMainWindow>

namespace Ui {
class MapGUIWindow;
}

class MapGUIWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MapGUIWindow(QWidget *parent = 0);
    ~MapGUIWindow();

private:
    Ui::MapGUIWindow *ui;
};

#endif // MAPGUIWINDOW_H
