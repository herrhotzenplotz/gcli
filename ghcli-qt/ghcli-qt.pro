QT          +=  core network widgets

TARGET  =       ghcli-qt

SOURCES =       src/main.cc \
                src/mainwindow.cc \
                src/issuemodel.cc \
                src/pullsmodel.cc \
                src/issueview.cc  \
                src/pullsview.cc

HEADERS =       include/ghcli-qt/mainwindow.hh \
                include/ghcli-qt/issuemodel.hh \
                include/ghcli-qt/pullsmodel.hh \
                include/ghcli-qt/issueview.hh  \
                include/ghcli-qt/pullsview.hh

INCLUDEPATH +=  include
INCLUDEPATH +=  ../include
INCLUDEPATH +=  ../../thirdparty

LIBS += -L../../ -lghcli -L/usr/local/lib -lcurl
