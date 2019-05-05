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

	void dropEvent(QDropEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	GLuint getTextureId(const QString& filename,const MapAccessor::ImageData& img_data) ;
	void computeRealCoordinates(int i,int j,float& longitude,float& latitude) const;

	virtual void resizeEvent(QResizeEvent *) override;
	virtual void wheelEvent(QWheelEvent *e) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent *e) override;
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void keyPressEvent(QKeyEvent *e) override;

	int mCurrentSlice_W ;
	int mCurrentSlice_H ;

	float *mCurrentSlice_data ;
	float mViewScale ;				// width of the viewing window in degrees of longitude

	bool mSliceUpdateNeeded ;
    bool mExplicitDraw;
    bool mMovingSelected;
    bool mShowImagesBorder;

    int  mLastX ;
    int  mLastY ;
    bool mMoving ;
    QString mSelectedImage;

    MapAccessor *mMA;
	std::vector<MapAccessor::ImageData> mImagesToDraw;

    MapDB::GPSCoord mCenter;
};


