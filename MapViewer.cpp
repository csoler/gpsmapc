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

	for(QStringList::const_iterator it(list.begin());it!=list.end();++it)
		std::cerr << "  " << (*it).toStdString() << "\t\tValue: " << QString(event->mimeData()->data(*it)).toStdString() << std::endl;

	if (event->mimeData()->hasFormat("text/plain"))
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
	if (event->mimeData()->hasFormat("STRING"))
	{
		QString filename(QUrl(event->mimeData()->data("STRING")).path() );
#ifdef DEBUG
		for(int i=0;i<filename.length();++i)
			std::cerr << "char " << i << ":'" << (char)filename[i].toAscii() << "'" << std::endl;
#endif
		while(!filename[filename.length()-1].isLetter())
		{
			std::cerr << "Chopping one char." << std::endl;
			filename.chop(1) ;
		}

#ifdef DEBUG
		std::cerr << "Dropping image \"" << filename.toStdString() << "\" at pos " << event->pos().x() << " " << event->pos().y() << std::endl;

		std::cerr << "File: " << QUrl(filename).path().toStdString() << std::endl;
#endif
		std::cerr << "Added image " << 	QUrl(filename).path().toStdString() << std::endl;
		event->accept();
	}
	else
	{
		event->ignore();
		std::cerr << "Ignoring drop. Wrong format." << std::endl;
	}
	updateGL() ;
}
