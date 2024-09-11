TEMPLATE = app
CONFIG += qt uic link_pkgconfig
QT = gui xml widgets opengl

PKGCONFIG += opencv4
QMAKE_CXXFLAGS *= -fopenmp
LIBS *= -lgomp

SOURCES = IGNMapper.cpp \
        ScreenshotCollectionMapDB.cpp \
        MapDB.cpp \
        MapAccessor.cpp \
        MapGUIWindow.cpp \
        MapViewer.cpp \
        QctFile.cpp \
        MapExporter.cpp \
        MapRegistration.cpp \
        QctMapDB.cpp

HEADERS = MapDB.h \
        MapAccessor.h \
        MapGUIWindow.h \
        ScreenshotCollectionMapDB.h \
        MapViewer.h \
        config.h \
        QctFile.h \
        MapExporter.h \
        MapRegistration.h \
        QctMapDB.h

INCLUDEPATH += /usr/include/opencv4

SOURCES += opencv_nonfree/surf.cpp
HEADERS += opencv_nonfree/surf.hpp opencv_nonfree/cuda.hpp opencv_nonfree/nonfree.hpp opencv_nonfree/precomp.hpp opencv_nonfree/xfeatures2d.hpp

DEFINES += OPENCV_ENABLE_NONFREE

FORMS =

LIBS *= -lQGLViewer-qt5

UI_DIR = .ui

LIBS *= -lglut -lGLU

# openCV
LIBS *= -lopencv_core -lopencv_imgproc  -lopencv_features2d  -lopencv_flann -lopencv_imgcodecs

#-lopencv_highgui -lopencv_ml -lopencv_video -lopencv_objdetect
#-lopencv_contrib -lopencv_legacy -lopencv_calib3d

debug {
	OBJECTS_DIR = .obj.debug
	DESTDIR = bin.debug
}
release {
	OBJECTS_DIR = .obj.release
	DESTDIR = bin.release
}
