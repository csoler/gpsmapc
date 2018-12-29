TEMPLATE = app
CONFIG = debug

SOURCES = qct2png.cpp

DEFINES *= USE_PNG

LIBS *= -lpng
