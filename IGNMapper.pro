TEMPLATE = app
CONFIG += qt
QT += gui xml widgets opengl

SOURCES = IGNMapper.cpp MapDB.cpp MapAccessor.cpp MapGUI.cpp MapViewer.cpp
HEADERS = MapDB.h MapAccessor.h MapGUI.h MapGUIWindow.h MapViewer.h config.h

FORMS = MapGUIWindow.ui

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
