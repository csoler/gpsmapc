TEMPLATE = app
CONFIG += qt uic
QT += gui xml widgets opengl

SOURCES = IGNMapper.cpp MapDB.cpp MapAccessor.cpp MapGUIWindow.cpp MapViewer.cpp
HEADERS = MapDB.h MapAccessor.h MapGUIWindow.h MapViewer.h config.h

FORMS =

LIBS *= -lQGLViewer

UI_DIR = .ui

debug {
	OBJECTS_DIR = .obj.debug
	DESTDIR = bin.debug
}
release {
	OBJECTS_DIR = .obj.release
	DESTDIR = bin.release
}
