QT          +=  core network widgets

TARGET  =       ghcli-qt

SOURCES =       src/main.cc \
                src/mainwindow.cc
HEADERS =       include/ghcli-qt/mainwindow.hh

INCLUDEPATH +=  include ../include

LIBS += -L../../ -lghcli
