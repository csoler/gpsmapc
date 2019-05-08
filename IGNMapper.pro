TEMPLATE = app
CONFIG += qt uic
QT = gui xml widgets opengl

QMAKE_CXXFLAGS *= -fopenmp
LIBS *= -lgomp

SOURCES = IGNMapper.cpp MapDB.cpp MapAccessor.cpp MapGUIWindow.cpp MapViewer.cpp \
    MapExporter.cpp \
    MapRegistration.cpp
HEADERS = MapDB.h MapAccessor.h MapGUIWindow.h MapViewer.h config.h \
    MapExporter.h \
    MapRegistration.h

FORMS =

LIBS *= -lQGLViewer

UI_DIR = .ui

LIBS *= -lglut -lGLU

debug {
	OBJECTS_DIR = .obj.debug
	DESTDIR = bin.debug
}
release {
	OBJECTS_DIR = .obj.release
	DESTDIR = bin.release
}
