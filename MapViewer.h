#include <QGLViewer/qglviewer.h>

class MapViewer: public QGLViewer
{
public:
	MapViewer(QWidget *parent) ;

	virtual void draw() override ;

	void forceUpdate();

protected:
	void computeSlice();
	void updateSlice();

	void dropEvent(QDropEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;

	virtual void resizeEvent(QResizeEvent *) override;
	virtual void wheelEvent(QWheelEvent *e) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	// virtual void keyPressEvent(QKeyEvent *e) override;

	int mCurrentSlice_W ;
	int mCurrentSlice_H ;

	float *mCurrentSlice_data ;
	float mViewScale ;

	bool mSliceUpdateNeeded ;
};


