QT += gui widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
#DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
# disables all the APIs deprecated before Qt 5.9.7
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050907

SOURCES += \
  chartmode.cpp \
  detailmode.cpp \
  globe.cpp \
  orthocam.cpp \
  outlinemode.cpp \
  outliner.cpp \
  glwidget.cpp \
  main.cpp \
  mainwindow.cpp \
  perscam.cpp \
  s57chart.cpp \
  types.cpp

LIBS += -lgdal

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
  camera.h \
  chartmode.h \
  detailmode.h \
  drawable.h \
  globe.h \
  orthocam.h \
  outlinemode.h \
  outliner.h \
  glwidget.h \
  mainwindow.h \
  earcut.hpp \
  perscam.h \
  s57chart.h \
  types.h

FORMS += \
  mainwindow.ui

RESOURCES += \
  shaders.qrc


