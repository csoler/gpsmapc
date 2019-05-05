#include <GL/glut.h>
#include <QMimeData>

#include "MapAccessor.h"
#include "MapViewer.h"
#include "MapDB.h"

MapViewer::MapViewer(QWidget *parent)
    : QGLViewer(parent)
{
    std::cerr << "Creating a MapViewer..."<< std::endl;

    setAcceptDrops(true);
    setMouseTracking(true);

    mCurrentSlice_data = NULL;
    mCurrentSlice_W = width();
    mCurrentSlice_H = height();

    mExplicitDraw = true;
    mMoving = false ;
    mMovingSelected = false ;

    mViewScale = 1.0;		// 1 pixel = 10000/cm lat/lon
    mCenter.lon = 0.0;
    mCenter.lat = 0.0;
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

void MapViewer::keyPressEvent(QKeyEvent *e)
{
    switch(e->key())
    {
    case Qt::Key_X: mExplicitDraw = !mExplicitDraw ;
        			updateGL();
    default:
        QGLViewer::keyPressEvent(e);
    }
}

void MapViewer::resizeEvent(QResizeEvent *e)
{
    mSliceUpdateNeeded = true;

    QGLViewer::resizeEvent(e);
}

static void checkGLError(const std::string& place,uint32_t line)
{
    GLuint e = 0 ;

    while( 0 != (e=glGetError()))
           std::cerr << "!!!!!!!!!!!! glGetError() returns code 0x" << std::hex << e << std::dec << " in " << place << ", line " << line << std::endl;
}
#define CHECK_GL_ERROR() { checkGLError(__PRETTY_FUNCTION__,__LINE__); }

void MapViewer::draw()
{
    if(mSliceUpdateNeeded && mExplicitDraw)
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

    float aspect_ratio = height() / (float)width() ;

    glOrtho(mCenter.lon - mViewScale/2.0,mCenter.lon + mViewScale/2.0,mCenter.lat - mViewScale/2.0*aspect_ratio,mCenter.lat + mViewScale/2.0*aspect_ratio,1,-1) ;

    if(mExplicitDraw)
	{
		// Not the best way to do this: we compute the image manually. However, this
		// method is the one we're going to use for exporting the data, so it's important to be able to visualize its output as well.

		glRasterPos2f(0,0) ;

		glPixelTransferf(GL_RED_SCALE  ,100.0) ;
		glPixelTransferf(GL_GREEN_SCALE,100.0) ;
		glPixelTransferf(GL_BLUE_SCALE ,100.0) ;

		glDrawPixels(mCurrentSlice_W,mCurrentSlice_H,GL_RGB,GL_FLOAT,(GLvoid*)mCurrentSlice_data) ;
	}
    else
	{
		// Here we use the graphics card for the texture mapping, so as to use the hardware to perform filtering.
        // Obviously that prevents us to do some more fancy image treatment such as selective blending etc. so this
        // method is only fod quick display purpose.

        MapDB::GPSCoord bottomLeftViewCorner(  mCenter.lon - mViewScale/2.0, mCenter.lat + mViewScale/2.0*aspect_ratio );
        MapDB::GPSCoord topRightViewCorner  (  mCenter.lon + mViewScale/2.0, mCenter.lat - mViewScale/2.0*aspect_ratio );

        mMA->getImagesToDraw(bottomLeftViewCorner,topRightViewCorner,mImagesToDraw);

		glPixelTransferf(GL_RED_SCALE  ,1.0) ;
		glPixelTransferf(GL_GREEN_SCALE,1.0) ;
		glPixelTransferf(GL_BLUE_SCALE ,1.0) ;

        glDisable(GL_DEPTH_TEST);

		CHECK_GL_ERROR();

        for(uint32_t i=0;i<mImagesToDraw.size();++i)
        {
            float image_lon_size = mImagesToDraw[i].lon_width;
            float image_lat_size = mImagesToDraw[i].lon_width * mImagesToDraw[i].H/(float)mImagesToDraw[i].W;

            glDisable(GL_LIGHTING);

			GLuint tex_id = getTextureId(mImagesToDraw[i].filename,mImagesToDraw[i]) ;

            glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,tex_id);

            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

			glBindTexture(GL_TEXTURE_2D,tex_id);
            CHECK_GL_ERROR();

            glColor3f(1,1,1);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glBegin(GL_QUADS);

            glTexCoord2f(0.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon                 , mImagesToDraw[i].top_left_corner.lat + image_lat_size );
            glTexCoord2f(1.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon + image_lon_size, mImagesToDraw[i].top_left_corner.lat + image_lat_size );
            glTexCoord2f(1.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon + image_lon_size, mImagesToDraw[i].top_left_corner.lat                  );
            glTexCoord2f(0.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon                 , mImagesToDraw[i].top_left_corner.lat                  );

            glEnd();
			glDisable(GL_TEXTURE_2D);

            CHECK_GL_ERROR();

            if(mImagesToDraw[i].filename == mSelectedImage)
            {
				glLineWidth(5.0);
				glColor3f(1.0,0.7,0.2) ;
			}
			else
			{
				glLineWidth(1.0);
				glColor3f(1.0,1.0,1.0);
			}

            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

            glBegin(GL_QUADS);
            glTexCoord2f(0.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon                 , mImagesToDraw[i].top_left_corner.lat + image_lat_size );
            glTexCoord2f(1.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon + image_lon_size, mImagesToDraw[i].top_left_corner.lat + image_lat_size );
            glTexCoord2f(1.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon + image_lon_size, mImagesToDraw[i].top_left_corner.lat                  );
            glTexCoord2f(0.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon                 , mImagesToDraw[i].top_left_corner.lat                  );
            glEnd();
        }
		CHECK_GL_ERROR();
    }
}

GLuint MapViewer::getTextureId(const QString& texture_filename,const MapAccessor::ImageData& img_data)
{
	static std::map<QString,GLuint> ids ;

	auto it = ids.find(texture_filename) ;

	if(it == ids.end())
	{
		GLuint tex_id = 0;
		glGenTextures(1,&tex_id);

        ids[texture_filename] = tex_id ;

		glBindTexture(GL_TEXTURE_2D,tex_id);
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		glPixelStorei(GL_PACK_ALIGNMENT ,1);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S    , GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T    , GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R    , GL_CLAMP);

		glEnable(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB32F,1024,1024,0,GL_RGBA,GL_UNSIGNED_BYTE,img_data.pixel_data);
		glDisable(GL_TEXTURE_2D);

		CHECK_GL_ERROR();

        std::cerr << "Allocating new texture ID " << tex_id << " for texture " << texture_filename.toStdString() << std::endl;
	}
	else
		return it->second;
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

void MapViewer::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {
        mMoving = false ;
#ifdef DEBUG
        std::cerr << "Moving stopped" << std::endl;
#endif
    }

    QGLViewer::mouseReleaseEvent(e);
}

void MapViewer::mousePressEvent(QMouseEvent *e)
{
	if(e->button() == Qt::LeftButton)
	{
		mMovingSelected = (e->modifiers() & Qt::ControlModifier);

		mMoving = true ;
		mLastX = e->globalX();
		mLastY = e->globalY();
#ifdef DEBUG
		std::cerr << "Moving started current_pos = " << mLastX << "," << mLastY << std::endl;
#endif
	}
	QGLViewer::mousePressEvent(e) ;
}

void MapViewer::computeRealCoordinates(int i,int j,float& longitude,float& latitude) const
{
    longitude = (i/(float)width() -0.5) * mViewScale + mCenter.lon ;
    latitude  =-(j/(float)height()-0.5) * mViewScale *(height()/(float)width()) + mCenter.lat ;

    std::cerr << longitude << " " << latitude << std::endl;
}

void MapViewer::mouseMoveEvent(QMouseEvent *e)
{
    if(mMoving)
    {
        if(mMovingSelected)
        {
            std::cerr << "moving selected image" << std::endl;
            return;
        }

        mCenter.lon -= (e->globalX() - mLastX) * mViewScale / width() ;
        mCenter.lat += (e->globalY() - mLastY) * mViewScale / width() ;
#ifdef DEBUG
        std::cerr << "Current x=" << e->globalX() << ", New center = " << mCenter << " mLastX=" << mLastX << " delta = " << e->globalX() - mLastX << ", " << e->globalY() - mLastY << std::endl;
#endif

        mLastX = e->globalX();
        mLastY = e->globalY();
        updateGL() ;
    }
    else // enter image selection mode
    {
        float latitude, longitude;
        computeRealCoordinates(e->x(),e->y(),longitude,latitude);

        QString new_selection ;
        float aspect = height()/(float)width();

        // that could be accelerated using a KDtree
        for(int i=mImagesToDraw.size()-1;i>=0;--i)
            if(mImagesToDraw[i].top_left_corner.lon <= longitude && mImagesToDraw[i].top_left_corner.lon + mImagesToDraw[i].lon_width >= longitude
            && mImagesToDraw[i].top_left_corner.lat >= latitude  && mImagesToDraw[i].top_left_corner.lat - mImagesToDraw[i].lon_width * aspect <= latitude )
            {
                new_selection = mImagesToDraw[i].filename;
                break;
            }

        if(new_selection != mSelectedImage)
        {
            mSelectedImage = new_selection;
            updateGL();
        }
    }

    QGLViewer::mouseMoveEvent(e);
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

