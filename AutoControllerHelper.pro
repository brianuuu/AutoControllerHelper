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
    SmartPrograms/Dev/smartbrightnessmeanfinder.cpp \
    SmartPrograms/Dev/smartcolorcalibrator.cpp \
    SmartPrograms/Dev/smartdelaycalibrator.cpp \
    SmartPrograms/Dev/smarttestprogram.cpp \
    SmartPrograms/PokemonBDSP/smartbdspboxoperation.cpp \
    SmartPrograms/PokemonBDSP/smartbdspduplicateitem1to30.cpp \
    SmartPrograms/PokemonBDSP/smartbdspshinylegendary.cpp \
    SmartPrograms/PokemonBDSP/smartbdspstarter.cpp \
    SmartPrograms/PokemonPLA/smartpladistortionwaiter.cpp \
    SmartPrograms/PokemonPLA/smartplanuggetfarmer.cpp \
    SmartPrograms/PokemonPLA/smartplaoutbreakfinder.cpp \
    SmartPrograms/PokemonPLA/smartplapasturesorter.cpp \
    SmartPrograms/PokemonPLA/smartplaresetalphapokemon.cpp \
    SmartPrograms/PokemonSwSh/smartberryfarmer.cpp \
    SmartPrograms/PokemonSwSh/smartdailyhighlight.cpp \
    SmartPrograms/PokemonSwSh/smartdayskipper.cpp \
    SmartPrograms/PokemonSwSh/smartloto.cpp \
    SmartPrograms/PokemonSwSh/smartmaxraidbattler.cpp \
    SmartPrograms/PokemonSwSh/smartpurplebeamfilder.cpp \
    SmartPrograms/PokemonSwSh/smartsurprisetrade.cpp \
    SmartPrograms/PokemonSwSh/smartwattfarmer.cpp \
    SmartPrograms/PokemonSwSh/smartycommglitch.cpp \
    SmartPrograms/Widgets/pokemonautofilllineedit.cpp \
    SmartPrograms/smartprogrambase.cpp \
    SmartPrograms/smartsimpleprogram.cpp \
    commandsender.cpp \
    main.cpp \
    autocontrollerwindow.cpp \
    pokemondatabase.cpp \
    pushbuttonmapping.cpp \
    remotecontrollerwindow.cpp \
    smartprogramsetting.cpp \
    videoeffectsetting.cpp \
    vlcwrapper.cpp

HEADERS += \
    SmartPrograms/Dev/smartbrightnessmeanfinder.h \
    SmartPrograms/Dev/smartcolorcalibrator.h \
    SmartPrograms/Dev/smartdelaycalibrator.h \
    SmartPrograms/Dev/smarttestprogram.h \
    SmartPrograms/PokemonBDSP/smartbdspboxoperation.h \
    SmartPrograms/PokemonBDSP/smartbdspduplicateitem1to30.h \
    SmartPrograms/PokemonBDSP/smartbdspshinylegendary.h \
    SmartPrograms/PokemonBDSP/smartbdspstarter.h \
    SmartPrograms/PokemonPLA/smartpladistortionwaiter.h \
    SmartPrograms/PokemonPLA/smartplanuggetfarmer.h \
    SmartPrograms/PokemonPLA/smartplaoutbreakfinder.h \
    SmartPrograms/PokemonPLA/smartplapasturesorter.h \
    SmartPrograms/PokemonPLA/smartplaresetalphapokemon.h \
    SmartPrograms/PokemonSwSh/smartberryfarmer.h \
    SmartPrograms/PokemonSwSh/smartdailyhighlight.h \
    SmartPrograms/PokemonSwSh/smartdayskipper.h \
    SmartPrograms/PokemonSwSh/smartloto.h \
    SmartPrograms/PokemonSwSh/smartmaxraidbattler.h \
    SmartPrograms/PokemonSwSh/smartpurplebeamfilder.h \
    SmartPrograms/PokemonSwSh/smartsurprisetrade.h \
    SmartPrograms/PokemonSwSh/smartwattfarmer.h \
    SmartPrograms/PokemonSwSh/smartycommglitch.h \
    SmartPrograms/Widgets/pokemonautofilllineedit.h \
    SmartPrograms/smartprogrambase.h \
    SmartPrograms/smartsimpleprogram.h \
    autocontrollerdefines.h \
    autocontrollerwindow.h \
    commandsender.h \
    pokemondatabase.h \
    pushbuttonmapping.h \
    remotecontrollerwindow.h \
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
