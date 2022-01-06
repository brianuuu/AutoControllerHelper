QT       += core gui
QT       += xml
QT       += network
QT       += serialport
QT       += multimedia
QT       += multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    SmartPrograms/smartbattletower.cpp \
    SmartPrograms/smartbdspdialgapalkia.cpp \
    SmartPrograms/smartbdspstarter.cpp \
    SmartPrograms/smartberryfarmer.cpp \
    SmartPrograms/smartdailyhighlight.cpp \
    SmartPrograms/smartloto.cpp \
    SmartPrograms/smartwattfarmer.cpp \
    commandsender.cpp \
    main.cpp \
    autocontrollerwindow.cpp \
    pushbuttonmapping.cpp \
    remotecontrollerwindow.cpp \
    SmartPrograms/smartbrightnessmeanfinder.cpp \
    SmartPrograms/smartdayskipper.cpp \
    SmartPrograms/smartdelaycalibrator.cpp \
    SmartPrograms/smartmaxraidbattler.cpp \
    SmartPrograms/smartprogrambase.cpp \
    SmartPrograms/smartpurplebeamfilder.cpp \
    SmartPrograms/smartsurprisetrade.cpp \
    SmartPrograms/smartycommglitch.cpp \
    smartprogramsetting.cpp \
    videoeffectsetting.cpp \
    vlcwrapper.cpp

HEADERS += \
    SmartPrograms/smartbattletower.h \
    SmartPrograms/smartbdspdialgapalkia.h \
    SmartPrograms/smartbdspstarter.h \
    SmartPrograms/smartberryfarmer.h \
    SmartPrograms/smartdailyhighlight.h \
    SmartPrograms/smartloto.h \
    SmartPrograms/smartwattfarmer.h \
    autocontrollerdefines.h \
    autocontrollerwindow.h \
    commandsender.h \
    pushbuttonmapping.h \
    remotecontrollerwindow.h \
    SmartPrograms/smartbrightnessmeanfinder.h \
    SmartPrograms/smartdayskipper.h \
    SmartPrograms/smartdelaycalibrator.h \
    SmartPrograms/smartmaxraidbattler.h \
    SmartPrograms/smartprogrambase.h \
    SmartPrograms/smartpurplebeamfilder.h \
    SmartPrograms/smartsurprisetrade.h \
    SmartPrograms/smartycommglitch.h \
    smartprogramsetting.h \
    videoeffectsetting.h \
    vlcwrapper.h

FORMS += \
    autocontrollerwindow.ui \
    remotecontrollerwindow.ui \
    smartprogramsetting.ui \
    videoeffectsetting.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RC_FILE = autocontrollerhelper.rc

RESOURCES += \
    resource.qrc

INCLUDEPATH += "D:/Brian_Data/LibVLC/include"
LIBS += -L"D:/Brian_Data/LibVLC" -lvlc -lvlccore
