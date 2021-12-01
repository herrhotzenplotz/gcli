QT          +=  core network widgets

TARGET  =       ghcli-qt

SOURCES =       src/main.cc \
                src/mainwindow.cc \
                src/issuemodel.cc \
                src/issueview.cc

HEADERS =       include/ghcli-qt/mainwindow.hh \
                include/ghcli-qt/issuemodel.hh \
                include/ghcli-qt/issueview.hh

INCLUDEPATH +=  include
INCLUDEPATH +=  ../include
INCLUDEPATH +=  ../../thirdparty

LIBS += -L../../ -lghcli -L/usr/local/lib -lcurl
