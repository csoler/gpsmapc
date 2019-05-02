#include <QMimeData>

#include "MapViewer.h"

MapViewer::MapViewer(QWidget *parent)
    : QGLViewer(parent)
{
    std::cerr << "Creating a MapViewer..."<< std::endl;
    setAcceptDrops(true);
}

void MapViewer::dragEnterEvent(QDragEnterEvent *event)
{
	std::cerr << "Drag enter event: "<< std::endl;

	QStringList list = event->mimeData()->formats() ;

#ifdef DEBUG
	for(QStringList::const_iterator it(list.begin());it!=list.end();++it)
		std::cerr << "  " << (*it).toStdString() << "\t\tValue: " << QString(event->mimeData()->data(*it)).toStdString() << std::endl;
#endif

	if (event->mimeData()->hasFormat("text/uri-list"))
		event->accept();
	else
		event->ignore();
}

// void MapViewer::dragLeaveEvent(QDragLeaveEvent *event)
// {
// 	_selected_type = SELECTED_TYPE_NONE ;
// 	updateGL() ;
// }
//
// void MapViewer::dragMoveEvent(QDragMoveEvent *event)
// {
// 	_selected_node = _graph.findSplitNode(event->pos().x()/(float)width(),1.0f - event->pos().y()/(float)height(),_about_to_split_orient,_about_to_split_side) ;
// 	_selected_type = SELECTED_TYPE_ABOUT_TO_SPLIT ;
// 	updateGL() ;
// }

void MapViewer::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls())
	{
        QList<QUrl> urls = event->mimeData()->urls();

        foreach(QUrl url,urls)
        {
            std::cerr << "Adding file from url " << url.toString().toStdString() << std::endl;
        }

		event->accept();
	}
	else
	{
		event->ignore();
		std::cerr << "Ignoring drop. Wrong format." << std::endl;
	}
	updateGL() ;
}
