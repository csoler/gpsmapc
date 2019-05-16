#include "MapDB.h"
#include "MapAccessor.h"
#include <QGLViewer/qglviewer.h>

class MapAccessor;

class MapViewer: public QGLViewer
{
public:
	MapViewer(QWidget *parent) ;

    void setMapAccessor(MapAccessor *ma);

	virtual void draw() override ;

	void forceUpdate();

protected:
	void computeSlice();
	void updateSlice();
	void exportMap();

	void dropEvent(QDropEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	GLuint getTextureId(const QString& filename,const MapAccessor::ImageData& img_data) ;
	void screenCoordinatesToImageSpaceCoordinates(int i, int j, MapDB::ImageSpaceCoord &is) const;
	void computeDescriptorsForCurrentImage();
	void computeRelatedTransform();
	void computeAllPositions();
	void addReferencePoint(QMouseEvent *e);
	bool screenPositionToSingleImagePixelPosition(int px, int py, float &img_x, float &img_y, QString& image_filename);
	void moveFromKeyboard(int key);

	virtual void resizeEvent(QResizeEvent *) override;
	virtual void wheelEvent(QWheelEvent *e) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent *e) override;
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void keyPressEvent(QKeyEvent *e) override;

	int mCurrentSlice_W ;
	int mCurrentSlice_H ;
    int mCurrentImageX;
    int mCurrentImageY;

	float *mCurrentSlice_data ;
	float mViewScale ;				// width of the viewing window in degrees of longitude

	bool mSliceUpdateNeeded ;
    bool mExplicitDraw;
    bool mMovingSelected;
    bool mShowImagesBorder;
    bool mShowExportGrid;

    MapRegistration::ImageDescriptor mCurrentDescriptor ;

    int  mDisplayDescriptor;
    int  mLastX ;
    int  mLastY ;
    bool mMoving ;
    QString mSelectedImage;
    QString mLastSelectedImage ;

    MapAccessor *mMA;
	std::vector<MapAccessor::ImageData> mImagesToDraw;

    MapDB::ImageSpaceCoord mCenter;
};


