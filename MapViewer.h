#include <QGLViewer/qglviewer.h>

class MapViewer: public QGLViewer
{
public:
	MapViewer(QWidget *parent) ;

protected:
	void dropEvent(QDropEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
};


