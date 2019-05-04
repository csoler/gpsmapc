#include <GL/glut.h>
#include <QMimeData>

#include "MapViewer.h"

MapViewer::MapViewer(QWidget *parent)
    : QGLViewer(parent)
{
    std::cerr << "Creating a MapViewer..."<< std::endl;
    setAcceptDrops(true);
    mViewScale = 1.0;
    mCurrentSlice_data = NULL;
    mCurrentSlice_W = width();
    mCurrentSlice_H = height();
}

void MapViewer::setMapAccessor(const MapAccessor *ma)
{
    mMA = ma ;
    updateSlice();
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

void MapViewer::resizeEvent(QResizeEvent *)
{
    mSliceUpdateNeeded = true;
}

void MapViewer::draw()
{
    std::cerr << "Drawing..." << std::endl;

    if(mSliceUpdateNeeded)
    {
        computeSlice();
        mSliceUpdateNeeded = false ;
    }

    glClearColor(0,0,0,0) ;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) ;

    glDisable(GL_DEPTH_TEST) ;

    glMatrixMode(GL_MODELVIEW) ;
    glLoadIdentity() ;

    glMatrixMode(GL_PROJECTION) ;
    glLoadIdentity() ;

    glOrtho(0,1,0,1,1,-1) ;

    glRasterPos2f(0,0) ;

    glPixelTransferf(GL_RED_SCALE  ,100.0) ;
    glPixelTransferf(GL_GREEN_SCALE,100.0) ;
    glPixelTransferf(GL_BLUE_SCALE ,100.0) ;

    glDrawPixels(mCurrentSlice_W,mCurrentSlice_H,GL_RGB,GL_FLOAT,(GLvoid*)mCurrentSlice_data) ;

}

void MapViewer::forceUpdate()
{
    updateSlice() ;
    updateGL();
}

void MapViewer::updateSlice()
{
    mSliceUpdateNeeded = true ;
}

void MapViewer::mouseMoveEvent(QMouseEvent *e)
{
    if(e->modifiers() & Qt::ControlModifier)
    {
        //updateViewingParameter(std::min(height()-1,std::max(0,height() - 1 - e->y()))/(float)height()) ;
        updateGL() ;
    }
}

void MapViewer::wheelEvent(QWheelEvent *e)
{
	if(e->delta() > 0)
		mViewScale *= 1.05 ;
	else
		mViewScale /= 1.05 ;

	displayMessage("Image scale: "+QString::number(mViewScale)) ;

	updateGL() ;
}

void MapViewer::computeSlice()
{
	//float aspect_ratio = height()/(float)width();
	//float window_width = mViewScale ;
	//float window_height= mViewScale*aspect_ratio ;

    std::cerr << "recompuging slice" << std::endl;

    if(mCurrentSlice_data == NULL || mCurrentSlice_W != width() || mCurrentSlice_H != height())
    {
    std::cerr << "re-allocating memory" << std::endl;
        delete[] mCurrentSlice_data ;
        mCurrentSlice_W = width();
        mCurrentSlice_H = height();

        mCurrentSlice_data = new float[3*mCurrentSlice_H*mCurrentSlice_W];
    }

#pragma omp parallel for
	for(int i=0;i<mCurrentSlice_W;++i)
		for(int j=0;j<mCurrentSlice_H;++j)
		{
			mCurrentSlice_data[3*(i+mCurrentSlice_W*j) + 0] = cos(12*i/(float)mCurrentSlice_W*M_PI) ;
			mCurrentSlice_data[3*(i+mCurrentSlice_W*j) + 1] = sin(15*j/(float)mCurrentSlice_W*M_PI) ;
			mCurrentSlice_data[3*(i+mCurrentSlice_W*j) + 2] = cos(i*j/100.0/100.0);
		}

	mSliceUpdateNeeded = false ;
}

static void StrokeText(const char *text)
{
    glLineWidth(1.5);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);

    for(const char* p = text; *p; ++p)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);

    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.0);
}

static void drawTextAtLocation(float tX,float tY,float tZ,const char* text,float aspect=1.0f,float scale=1.0f)
{
    const float ox = 0.01 ;
    const float oy = 0.0 ;
    const float text_height = 0.04 ;

    GLdouble modl[16] ;
    GLdouble proj[16] ;
    GLint viewport[4] ;

    glGetDoublev(GL_MODELVIEW_MATRIX,modl) ;
    glGetDoublev(GL_PROJECTION_MATRIX,proj) ;
    glGetIntegerv(GL_VIEWPORT,viewport) ;

    GLdouble wx,wy,wz ;
    gluProject(tX,tY,tZ,modl,proj,viewport,&wx,&wy,&wz) ;

    glMatrixMode(GL_PROJECTION) ;
    glPushMatrix() ;
    glLoadIdentity() ;
    glOrtho(0,1,0,aspect,-1,1) ;

    glMatrixMode(GL_MODELVIEW) ;
    glPushMatrix() ;
    glDisable(GL_DEPTH_TEST) ;

    const char *p = text ;
    int n=0 ;

    char *buff = new char[1+strlen(text)] ;

    while(true)
    {
        const char *pp = p ;
        int i=0 ;

        while((*pp != '\n')&&(*pp != 0))
        {
            buff[i++] = *pp ;
            pp++ ;
        }

        glLoadIdentity() ;
        glTranslatef(wx/(float)viewport[2]+ox,(wy/(float)viewport[3]+oy)*aspect-n*text_height,0.0) ;
        glScalef(0.00021*scale,0.0002*scale,0.0003*scale) ;

        buff[i] = 0 ;
        StrokeText(buff) ;

        if(*pp != 0)
            pp++ ;
        else
            break ;

        p = pp ;
    }

    delete[] buff ;
    glMatrixMode(GL_MODELVIEW) ;
    glPopMatrix() ;
    glMatrixMode(GL_PROJECTION) ;
    glPopMatrix() ;
}

