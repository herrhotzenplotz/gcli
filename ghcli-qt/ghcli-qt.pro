QT          +=  core network widgets concurrent

TARGET  =       ghcli-qt

SOURCES =       src/main.cc \
                src/mainwindow.cc \
                src/issuemodel.cc \
                src/pullsmodel.cc \
                src/issueview.cc  \
                src/pullsview.cc  \
                src/issuedetailview.cc

HEADERS =       include/ghcli-qt/mainwindow.hh \
                include/ghcli-qt/issuemodel.hh \
                include/ghcli-qt/pullsmodel.hh \
                include/ghcli-qt/issueview.hh  \
                include/ghcli-qt/pullsview.hh  \
                include/ghcli-qt/issuedetailview.hh

INCLUDEPATH +=  include
INCLUDEPATH +=  ../include
INCLUDEPATH +=  ../../thirdparty

LIBS += -L../../ -lghcli -L/usr/local/lib -lcurl
