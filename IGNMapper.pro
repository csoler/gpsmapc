TEMPLATE = app
CONFIG += qt
QT += gui xml widgets

SOURCES = IGNMapper.cpp MapDB.cpp MapAccessor.cpp MapGUI.cpp
HEADERS = MapDB.h MapAccessor.h MapGUI.h MapGUIWindow.h

FORMS = MapGUIWindow.ui

