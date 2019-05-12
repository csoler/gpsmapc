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

    mCurrentImageX = -1;
    mCurrentImageY = -1;
    mExplicitDraw = false;
    mMoving = false ;
    mMovingSelected = false ;
    mShowImagesBorder = true;
    mDisplayDescriptor=0;

    mViewScale = 1.0;		// 1 pixel = 10000/cm lat/lon
    mCenter.lon = 0.0;
    mCenter.lat = 0.0;
}

void MapViewer::setMapAccessor(MapAccessor *ma)
{
    mMA = ma ;
    mViewScale = (mMA->bottomRightCorner().lon - mMA->topLeftCorner().lon)/2.0 * 1.05;
    mCenter.lon = 0.5*(mMA->topLeftCorner().lon + mMA->bottomRightCorner().lon);
    mCenter.lat = 0.5*(mMA->topLeftCorner().lat + mMA->bottomRightCorner().lat);

    std::cerr << "Loaded new accessor. Center is " << mCenter << " scale is " << mViewScale << std::endl;
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
    case Qt::Key_W: mMA->saveMap();
        displayMessage("Map saved");
		break;

    case Qt::Key_X: mExplicitDraw = !mExplicitDraw ;
        			updateGL();
        break;

    case Qt::Key_B: mShowImagesBorder = !mShowImagesBorder ;
        			displayMessage("Toggled images borders");
        			updateGL();
        break;

	case Qt::Key_D: displayMessage("computing descriptors...");
                    computeDescriptorsForCurrentImage();
                    updateGL();
        break;

	case Qt::Key_T: displayMessage("computing transform...");
                    computeRelatedTransform();
                    updateGL();
		break;

	case Qt::Key_E: mDisplayDescriptor = (mDisplayDescriptor+1)%4;
                    updateGL();
        break;

	case Qt::Key_S: if(!mSelectedImage.isNull())
            			mLastSelectedImage = mSelectedImage;
                    updateGL();
        break;
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
			else if(mImagesToDraw[i].filename == mLastSelectedImage)
            {
				glLineWidth(5.0);
				glColor3f(0.7,0.9,0.3) ;
            }
			else
			{
				glLineWidth(1.0);
				glColor3f(1.0,1.0,1.0);
			}

            if(mShowImagesBorder)
			{
				glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

				glBegin(GL_QUADS);
				glTexCoord2f(0.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon                 , mImagesToDraw[i].top_left_corner.lat + image_lat_size );
				glTexCoord2f(1.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon + image_lon_size, mImagesToDraw[i].top_left_corner.lat + image_lat_size );
				glTexCoord2f(1.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon + image_lon_size, mImagesToDraw[i].top_left_corner.lat                  );
				glTexCoord2f(0.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.lon                 , mImagesToDraw[i].top_left_corner.lat                  );
				glEnd();
			}

            // now draw file descriptors if any

			glLineWidth(5.0);
			static const int nb_pts = 50 ;
			int H = mImagesToDraw[i].H;
			float scale = mImagesToDraw[i].lon_width / mImagesToDraw[i].W;

            for(uint32_t k=0;k<mImagesToDraw[i].descriptors.size();++k)
            {
                const MapRegistration::ImageDescriptor& desc(mImagesToDraw[i].descriptors[k]);
				float radius = mImagesToDraw[i].descriptors[k].pixel_radius;

                glColor3f(1.0,0.0,0.0);

                glBegin(GL_LINE_LOOP) ;

                for(int l=0;l<nb_pts;++l)
					glVertex2f(mImagesToDraw[i].top_left_corner.lon + (    desc.x+radius*cos(2*M_PI*l/(float)nb_pts))*scale,
                               mImagesToDraw[i].top_left_corner.lat + (H-1-desc.y+radius*sin(2*M_PI*l/(float)nb_pts))*scale);

                glEnd();
            }

            // also draw current descriptor mask around current point
			glColor3f(0.7,1.0,0.2);

            glEnable(GL_LINE_SMOOTH) ;
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA) ;

			glBegin(GL_LINE_LOOP) ;

			float radius = mCurrentDescriptor.pixel_radius;

			for(int l=0;l<nb_pts;++l)
					glVertex2f(mImagesToDraw[i].top_left_corner.lon + (    mCurrentImageX+radius*cos(2*M_PI*l/(float)nb_pts))*scale,
                               mImagesToDraw[i].top_left_corner.lat + (H-1-mCurrentImageY+radius*sin(2*M_PI*l/(float)nb_pts))*scale);

			glEnd();
			glLineWidth(1.0);
        }
		CHECK_GL_ERROR();

        for(int i=0;i<mMA->mapDB().numberOfReferencePoints();++i)
        {
            const MapDB::ReferencePoint& p = mMA->mapDB().getReferencePoint(i);
            MapDB::RegisteredImage img;

            std::cerr << "Drawing ref point " << i << " " << p.x << " " << p.y << std::endl;
			mMA->getImageParams(p.filename,img);

            float scale = img.scale / img.W;
			float radius = 100;
            int nb_pts = 50;

            glColor3f(0.1,0.3,0.9);
            glLineWidth(3.0);
            glBegin(GL_LINE_LOOP);

			for(int l=0;l<nb_pts;++l)
					glVertex2f(p.x*scale + img.top_left_corner.lon + radius*cos(2*M_PI*l/(float)nb_pts)*scale,
                               (img.H-1-p.y)*scale + img.top_left_corner.lat + radius*sin(2*M_PI*l/(float)nb_pts)*scale);

			glEnd();

            glEnable(GL_POINT_SMOOTH);
            glPointSize(10.0);

            glBegin(GL_POINTS);
			glVertex2f(p.x*scale + img.top_left_corner.lon, (img.H-1-p.y)*scale + img.top_left_corner.lat);
            glEnd();
        }
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

void MapViewer::addReferencePoint(QMouseEvent *e)
{
	float latitude, longitude;
	computeRealCoordinates(e->x(),e->y(),longitude,latitude);

	QString selection ;
	float aspect = height()/(float)width();
    int selection_X=0,selection_Y=0;

	// That could be accelerated using a KDtree

	for(int i=mImagesToDraw.size()-1;i>=0;--i)
		if(mImagesToDraw[i].top_left_corner.lon <= longitude && mImagesToDraw[i].top_left_corner.lon + mImagesToDraw[i].lon_width >= longitude
		        && mImagesToDraw[i].top_left_corner.lat <= latitude  && mImagesToDraw[i].top_left_corner.lat + mImagesToDraw[i].lon_width * aspect >= latitude )
		{
			selection = mImagesToDraw[i].filename;

			selection_X = (longitude - mImagesToDraw[i].top_left_corner.lon)/mImagesToDraw[i].lon_width*mImagesToDraw[i].W;
			selection_Y = mImagesToDraw[i].H - 1 - (latitude - mImagesToDraw[i].top_left_corner.lat)/mImagesToDraw[i].lon_width*mImagesToDraw[i].W;
		}

    if(!selection.isNull())
    {
        std::cerr << "Setting new reference point in image " << selection.toStdString() << " at point " << selection_X << " " << selection_Y << std::endl;
        mMA->setReferencePoint(selection,selection_X,selection_Y) ;
        updateGL();
    }
}

void MapViewer::mousePressEvent(QMouseEvent *e)
{
	if(e->button() == Qt::LeftButton)
	{
        if(e->modifiers() & Qt::ShiftModifier)
        {
            addReferencePoint(e) ;
            return ;
        }

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
    latitude  = ((height()-j-1)/(float)height()-0.5) * mViewScale *(height()/(float)width()) + mCenter.lat ;
#ifdef DEBUG
    std::cerr << longitude << " " << latitude << std::endl;
#endif
}

void MapViewer::mouseMoveEvent(QMouseEvent *e)
{
    if(mMoving)
    {
        if(mMovingSelected)
        {
        	float delta_lon = - (e->globalX() - mLastX) * mViewScale / width() ;
        	float delta_lat =   (e->globalY() - mLastY) * mViewScale / width() ;

            mMA->moveImage(mSelectedImage,-delta_lon,-delta_lat);

			mLastX = e->globalX();
			mLastY = e->globalY();

            updateGL();
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
            && mImagesToDraw[i].top_left_corner.lat <= latitude  && mImagesToDraw[i].top_left_corner.lat + mImagesToDraw[i].lon_width * aspect >= latitude )
            {
                new_selection = mImagesToDraw[i].filename;

#ifdef TO_REMOVE
                if(mDisplayDescriptor > 0)
				{
					mCurrentImageX = (longitude - mImagesToDraw[i].top_left_corner.lon)/mImagesToDraw[i].lon_width*mImagesToDraw[i].W;
					mCurrentImageY = mImagesToDraw[i].H - 1 - (latitude - mImagesToDraw[i].top_left_corner.lat)/mImagesToDraw[i].lon_width*mImagesToDraw[i].W;

					std::cerr << "Point " << mCurrentImageX << " " << mCurrentImageY << " ";
					QImage image = mMA->getImageData(new_selection);

					if(image.width()==0)
					{
						std::cerr << "Error: cannot load image" << std::endl;
						break;
					}

                    switch(mDisplayDescriptor)
                    {
                    case 1: MapRegistration::computeDescriptor1(image.bits(),image.width(),image.height(),mCurrentImageX,mCurrentImageY,mCurrentDescriptor); break;
                    case 2: MapRegistration::computeDescriptor2(image.bits(),image.width(),image.height(),mCurrentImageX,mCurrentImageY,mCurrentDescriptor); break;
                    case 3: MapRegistration::computeDescriptor3(image.bits(),image.width(),image.height(),mCurrentImageX,mCurrentImageY,mCurrentDescriptor); break;
                    default: break;
                    }

					std::cerr << "Descriptor: " ;
					for(i=0;i<std::min((size_t)10,mCurrentDescriptor.data.size());++i)
						std::cerr << mCurrentDescriptor.data[i] << " " ;
					std::cerr << std::endl;
				}
                else
                {
                    mCurrentImageX = -1 ;
                    mCurrentImageY = -1 ;
                }
#else
				mCurrentImageX = (longitude - mImagesToDraw[i].top_left_corner.lon)/mImagesToDraw[i].lon_width*mImagesToDraw[i].W;
				mCurrentImageY = mImagesToDraw[i].H - 1 - (latitude - mImagesToDraw[i].top_left_corner.lat)/mImagesToDraw[i].lon_width*mImagesToDraw[i].W;
#endif
				updateGL();
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

void MapViewer::computeRelatedTransform()
{
    if(mLastSelectedImage.isNull() || mSelectedImage.isNull())
    {
        std::cerr << "Error: you need to compute descriptors for 2 images." << std::endl;
        return ;
    }

    float dx,dy ;

    if(! MapRegistration::computeRelativeTransform(mMA->fullPath(mSelectedImage).toStdString(),mMA->fullPath(mLastSelectedImage).toStdString(),dx,dy))
    {
        std::cerr << "No valid transform found!" << std::endl;
        return;
    }

    // need to move the 2nd image at the right place

    MapDB::RegisteredImage img1,img2;

	mMA->getImageParams(mSelectedImage,img1);
	mMA->getImageParams(mLastSelectedImage,img2);

    MapDB::GPSCoord new_corner ;
    new_corner.lon = img2.top_left_corner.lon + dx/img2.W*img2.scale ;
    new_corner.lat = img2.top_left_corner.lat - dy/img2.W*img2.scale ;

	mMA->placeImage(mSelectedImage,new_corner);
    updateGL();
}

void MapViewer::computeDescriptorsForCurrentImage()
{
    if(! mSelectedImage.isNull())
        mMA->recomputeDescriptors(mSelectedImage);
}

