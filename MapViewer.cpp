#include <GL/glut.h>
#include <QMimeData>
#include <QToolTip>

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
    mCenter.x = 0.0;
    mCenter.y = 0.0;
}

void MapViewer::setMapAccessor(MapAccessor *ma)
{
    mMA = ma ;
    mViewScale = (mMA->bottomRightCorner().x - mMA->topLeftCorner().y)/2.0 * 1.05;
    mCenter.x = 0.5*(mMA->topLeftCorner().x + mMA->bottomRightCorner().x);
    mCenter.y = 0.5*(mMA->topLeftCorner().y + mMA->bottomRightCorner().y);

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

	case Qt::Key_P: displayMessage("computing all positions...");
                    computeAllPositions();
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

    glOrtho(mCenter.x - mViewScale/2.0,mCenter.x + mViewScale/2.0,mCenter.y - mViewScale/2.0*aspect_ratio,mCenter.y + mViewScale/2.0*aspect_ratio,1,-1) ;

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

        MapDB::ImageSpaceCoord bottomLeftViewCorner(  mCenter.x - mViewScale/2.0, mCenter.y + mViewScale/2.0*aspect_ratio );
        MapDB::ImageSpaceCoord topRightViewCorner  (  mCenter.x + mViewScale/2.0, mCenter.y - mViewScale/2.0*aspect_ratio );

        mMA->getImagesToDraw(bottomLeftViewCorner,topRightViewCorner,mImagesToDraw);

		glPixelTransferf(GL_RED_SCALE  ,1.0) ;
		glPixelTransferf(GL_GREEN_SCALE,1.0) ;
		glPixelTransferf(GL_BLUE_SCALE ,1.0) ;

        glDisable(GL_DEPTH_TEST);

		CHECK_GL_ERROR();

        for(uint32_t i=0;i<mImagesToDraw.size();++i)
        {
            float image_lon_size = mImagesToDraw[i].W;
            float image_lat_size = mImagesToDraw[i].H;

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

            glTexCoord2f(0.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.x                 , mImagesToDraw[i].top_left_corner.y + image_lat_size );
            glTexCoord2f(1.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.x + image_lon_size, mImagesToDraw[i].top_left_corner.y + image_lat_size );
            glTexCoord2f(1.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.x + image_lon_size, mImagesToDraw[i].top_left_corner.y                  );
            glTexCoord2f(0.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.x                 , mImagesToDraw[i].top_left_corner.y                  );

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
				glTexCoord2f(0.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.x                 , mImagesToDraw[i].top_left_corner.y + image_lat_size );
				glTexCoord2f(1.0,0.0); glVertex2f( mImagesToDraw[i].top_left_corner.x + image_lon_size, mImagesToDraw[i].top_left_corner.y + image_lat_size );
				glTexCoord2f(1.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.x + image_lon_size, mImagesToDraw[i].top_left_corner.y                  );
				glTexCoord2f(0.0,1.0); glVertex2f( mImagesToDraw[i].top_left_corner.x                 , mImagesToDraw[i].top_left_corner.y                  );
				glEnd();
			}

            // now draw file descriptors if any

			glLineWidth(5.0);
			static const int nb_pts = 50 ;
			int H = mImagesToDraw[i].H;

            for(uint32_t k=0;k<mImagesToDraw[i].descriptors.size();++k)
            {
                const MapRegistration::ImageDescriptor& desc(mImagesToDraw[i].descriptors[k]);
				float radius = mImagesToDraw[i].descriptors[k].pixel_radius;

                glColor3f(1.0,0.0,0.0);

                glBegin(GL_LINE_LOOP) ;

                for(int l=0;l<nb_pts;++l)
					glVertex2f(mImagesToDraw[i].top_left_corner.x + (    desc.x+radius*cos(2*M_PI*l/(float)nb_pts)),
                               mImagesToDraw[i].top_left_corner.y + (H-1-desc.y+radius*sin(2*M_PI*l/(float)nb_pts)));

                glEnd();
            }

            // also draw current descriptor mask around current point
			glColor3f(0.7,1.0,0.2);

            glEnable(GL_LINE_SMOOTH) ;
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA) ;

			glBegin(GL_LINE_LOOP) ;

			float radius = mCurrentDescriptor.pixel_radius;

			for(int l=0;l<nb_pts;++l)
					glVertex2f(mImagesToDraw[i].top_left_corner.x + (    mCurrentImageX+radius*cos(2*M_PI*l/(float)nb_pts)),
                               mImagesToDraw[i].top_left_corner.y + (H-1-mCurrentImageY+radius*sin(2*M_PI*l/(float)nb_pts)));

			glEnd();
			glLineWidth(1.0);
        }
		CHECK_GL_ERROR();

        for(int i=0;i<mMA->mapDB().numberOfReferencePoints();++i)
        {
            const MapDB::ReferencePoint& p = mMA->mapDB().getReferencePoint(i);
            MapDB::RegisteredImage img;

			mMA->getImageParams(p.filename,img);

			float radius = 100;
            int nb_pts = 50;

            glColor3f(0.1,0.3,0.9);
            glLineWidth(3.0);
            glBegin(GL_LINE_LOOP);

			for(int l=0;l<nb_pts;++l)
					glVertex2f(p.x + img.top_left_corner.x + radius*cos(2*M_PI*l/(float)nb_pts),
                              (img.H-1-p.y) + img.top_left_corner.y + radius*sin(2*M_PI*l/(float)nb_pts));

			glEnd();

            glEnable(GL_POINT_SMOOTH);
            glPointSize(10.0);

            glBegin(GL_POINTS);
			glVertex2f(p.x + img.top_left_corner.x, (img.H-1-p.y) + img.top_left_corner.y);
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

bool MapViewer::computeImagePixelAtScreenPosition(int px,int py,int& img_x,int& img_y,QString& image_filename)
{
	float is_y,is_x;
	screenCoordinatesToImageSpaceCoordinates(px,py,is_x,is_y);

	QString selection ;
	float aspect = height()/(float)width();

	// That could be accelerated using a KDtree

	for(int i=mImagesToDraw.size()-1;i>=0;--i)
		if(	       mImagesToDraw[i].top_left_corner.x                      <= is_x
                && mImagesToDraw[i].top_left_corner.x + mImagesToDraw[i].W >= is_x
		        && mImagesToDraw[i].top_left_corner.y                      <= is_y
                && mImagesToDraw[i].top_left_corner.y + mImagesToDraw[i].H >= is_y )
		{
			image_filename = mImagesToDraw[i].filename;

			img_x = is_x - mImagesToDraw[i].top_left_corner.x;
			img_y = mImagesToDraw[i].H - 1 - (is_y - mImagesToDraw[i].top_left_corner.y);

            return true;
		}

    return false;
}

void MapViewer::screenCoordinatesToImageSpaceCoordinates(int i,int j,float& is_x,float& is_y) const
{
    is_x = (i/(float)width() -0.5) * mViewScale + mCenter.x ;
    is_y = ((height()-j-1)/(float)height()-0.5) * mViewScale *(height()/(float)width()) + mCenter.y ;
#ifdef DEBUG
    std::cerr << is_x << " " << is_y << std::endl;
#endif
}

void MapViewer::addReferencePoint(QMouseEvent *e)
{
    int x,y;
    QString selection;

    if(!computeImagePixelAtScreenPosition(e->x(),e->y(),x,y,selection))
        return ;

    if(!selection.isNull())
    {
        std::cerr << "Setting new reference point in image " << selection.toStdString() << " at point " << x << " " << y << std::endl;
        mMA->setReferencePoint(selection,x,y) ;
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
		mLastX = e->x();
		mLastY = e->y();
#ifdef DEBUG
		std::cerr << "Moving started current_pos = " << mLastX << "," << mLastY << std::endl;
#endif
	}
	QGLViewer::mousePressEvent(e) ;
}

void MapViewer::mouseMoveEvent(QMouseEvent *e)
{
    if(mMoving)
    {
        if(mMovingSelected)
        {
        	float delta_lon = - (e->x() - mLastX) * mViewScale / width() ;
        	float delta_lat =   (e->y() - mLastY) * mViewScale / width() ;

            mMA->moveImage(mSelectedImage,-delta_lon,-delta_lat);

			mLastX = e->x();
			mLastY = e->y();

            updateGL();
            return;
        }

        mCenter.x -= (e->x() - mLastX) * mViewScale / width() ;
        mCenter.y += (e->y() - mLastY) * mViewScale / width() ;
#ifdef DEBUG
        std::cerr << "Current x=" << e->x() << ", New center = " << mCenter << " mLastX=" << mLastX << " delta = " << e->x() - mLastX << ", " << e->y() - mLastY << std::endl;
#endif

        mLastX = e->x();
        mLastY = e->y();

        updateGL() ;
    }
    else // enter image selection mode
    {
        QString new_selection ;
        int new_x,new_y;

        if(computeImagePixelAtScreenPosition(e->x(),e->y(),new_x,new_y,new_selection))
		{
			mCurrentImageX = new_x;
			mCurrentImageY = new_y;

			QString displayText ;
			displayText += QString::number(e->x()) + "," +QString::number(e->y());
			displayText += "  I: " + new_selection + " at " + QString::number(new_x) + "," + QString::number(new_y);

            float global_x,global_y ;

			screenCoordinatesToImageSpaceCoordinates(e->x(),e->y(),global_x,global_y);

            MapDB::GPSCoord g ;

            if(mMA->mapDB().viewCoordinatesToGPSCoordinates(MapDB::ImageSpaceCoord(global_x,global_y),g))
                displayText += "\nLat: " + QString::number(g.lat) + " Lon: " + QString::number(g.lon);

			QToolTip::showText(QPoint(20+e->globalX() - x(),20+e->globalY() - y()),displayText);
		}

		//QToolTip::showText(QCursor::pos() + QPoint(20,20),displayText);

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

    MapDB::ImageSpaceCoord new_corner ;
    new_corner.x = img2.top_left_corner.x + dx;
    new_corner.y = img2.top_left_corner.y - dy;

	mMA->placeImage(mSelectedImage,new_corner);
    updateGL();
}

void MapViewer::computeAllPositions()
{
    std::vector<std::pair<float,float> > coords ;
    std::vector<std::string> images_full_paths ;

    auto images_map = mMA->mapDB().getFullListOfImages();

    for(auto it(images_map.begin());it!=images_map.end();++it)
    	images_full_paths.push_back(mMA->fullPath(it->first).toStdString());

	if(! MapRegistration::computeAllImagesPositions(images_full_paths,coords))
    {
        std::cerr << "No global transform found!" << std::endl;
        return;
    }

    // now move all images at the right place

    int i=0;
    for(auto it(images_map.begin());it!=images_map.end();++it,++i)
		mMA->placeImage(it->first,MapDB::ImageSpaceCoord(coords[i].first,coords[i].second));

    updateGL();
}

void MapViewer::computeDescriptorsForCurrentImage()
{
    if(! mSelectedImage.isNull())
        mMA->recomputeDescriptors(mSelectedImage);
}

