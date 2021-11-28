#ifndef SMARTPROGRAMBASE_H
#define SMARTPROGRAMBASE_H

#include "autocontrollerdefines.h"

#include <QCameraImageCapture>
#include <QCameraViewfinder>
#include <QDebug>
#include <QDomDocument>
#include <QGraphicsScene>
#include <QTimer>
#include <QWidget>

#include <QtMath>

#include "../vlcwrapper.h"
#include "../smartprogramsetting.h"

enum SmartProgram
{
    SP_DelayCalibrator = 0,
    SP_BrightnessMeanFinder,
    SP_PurpleBeamFinder,
    SP_YCommGlitch,
    SP_SurpriseTrade,
    SP_MaxRaidBattler,
    SP_DaySkipper,
    SP_BattleTower,
    SP_Loto,
    SP_DailyHighlight,
    SP_BerryFarmer,
    SP_WattFarmer,

    SP_BDSP_DialgaPalkia,

    SP_COUNT
};

struct SmartProgramParameter
{
    QDomDocument* smartProgramCommands;
    VLCWrapper* vlcWrapper;
    SmartProgramSetting* settings;
    QWidget* parent;
    QGraphicsScene* preview;
    QGraphicsScene* previewMasked;

    SmartProgramParameter(
            QDomDocument* _smartProgramCommands,
            VLCWrapper* _vlcWrapper,
            SmartProgramSetting* _settings,
            QWidget* _parent = nullptr,
            QGraphicsScene* _preview = nullptr,
            QGraphicsScene* _previewMasked = nullptr
            )
        : smartProgramCommands(_smartProgramCommands)
        , vlcWrapper(_vlcWrapper)
        , settings(_settings)
        , parent(_parent)
        , preview(_preview)
        , previewMasked(_previewMasked)
    {}
};

class SmartProgramBase : public QWidget
{
    Q_OBJECT

public:
    explicit SmartProgramBase(SmartProgramParameter parameter);

    static QString getProgramNameFromEnum(SmartProgram sp)
    {
        switch (sp)
        {
            case SP_DelayCalibrator:        return "Dev: Camera Delay Checker";
            case SP_BrightnessMeanFinder:   return "Dev: Brightness Mean Finder";
            case SP_PurpleBeamFinder:       return "Purple Beam Finder";
            case SP_YCommGlitch:            return "Y-Comm Glitch";
            case SP_SurpriseTrade:          return "Auto Surprise Trade";
            case SP_MaxRaidBattler:         return "Max Raid Battler";
            case SP_DaySkipper:             return "Auto Day Skipper";
            case SP_BattleTower:            return "Auto Battle Tower";
            case SP_Loto:                   return "Auto Loto";
            case SP_DailyHighlight:         return "Daily Highlight Farmer";
            case SP_BerryFarmer:            return "Berry Farmer";
            case SP_WattFarmer:             return "Watt Farmer";

            case SP_BDSP_DialgaPalkia:      return "Reset Dialga/Palkia";

            case SP_COUNT:                  return "Invalid";
        }
    }

    static SmartProgram getProgramEnumFromName(QString const& sp)
    {
        if (sp == "Dev: Camera Delay Checker")      return SP_DelayCalibrator;
        if (sp == "Dev: Brightness Mean Finder")    return SP_BrightnessMeanFinder;
        if (sp == "Purple Beam Finder")             return SP_PurpleBeamFinder;
        if (sp == "Y-Comm Glitch")                  return SP_YCommGlitch;
        if (sp == "Auto Surprise Trade")            return SP_SurpriseTrade;
        if (sp == "Max Raid Battler")               return SP_MaxRaidBattler;
        if (sp == "Auto Day Skipper")               return SP_DaySkipper;
        if (sp == "Auto Battle Tower")              return SP_BattleTower;
        if (sp == "Auto Loto")                      return SP_Loto;
        if (sp == "Daily Highlight Farmer")         return SP_DailyHighlight;
        if (sp == "Berry Farmer")                   return SP_BerryFarmer;
        if (sp == "Watt Farmer")                    return SP_WattFarmer;

        if (sp == "Reset Dialga/Palkia")            return SP_BDSP_DialgaPalkia;

        return SP_COUNT;
    }

    static QString getProgramInternalNameFromEnum(SmartProgram sp)
    {
        switch (sp)
        {
            case SP_DelayCalibrator:        return "SmartDelayCalibrator";
            case SP_BrightnessMeanFinder:   return "SmartBrightnessMeanFinder";
            case SP_PurpleBeamFinder:       return "SmartPurpleBeamFilder";
            case SP_YCommGlitch:            return "SmartYCommGlitch";
            case SP_SurpriseTrade:          return "SmartSurpriseTrade";
            case SP_MaxRaidBattler:         return "SmartMaxRaidBattler";
            case SP_DaySkipper:             return "SmartDaySkipper";
            case SP_BattleTower:            return "SmartBattleTower";
            case SP_Loto:                   return "SmartLoto";
            case SP_DailyHighlight:         return "SmartDailyHighlight";
            case SP_BerryFarmer:            return "SmartBerryFarmer";
            case SP_WattFarmer:             return "SmartWattFarmer";

            case SP_BDSP_DialgaPalkia:      return "SmartBDSPDialgaPalkia";

            case SP_COUNT:                  return "Invalid";
        }
    }

    static int getProgramTabID(SmartProgram sp)
    {
        // -1: Not added to list, 0: No settings
        switch (sp)
        {
            case SP_DelayCalibrator:        return 0;
            case SP_BrightnessMeanFinder:   return 1;
            case SP_PurpleBeamFinder:       return 2;
            case SP_YCommGlitch:            return 0;
            case SP_SurpriseTrade:          return 3;
            case SP_MaxRaidBattler:         return 4;
            case SP_DaySkipper:             return 5;
            case SP_BattleTower:            return 0;
            case SP_Loto:                   return 6;
            case SP_DailyHighlight:         return 6;
            case SP_BerryFarmer:            return 6;
            case SP_WattFarmer:             return 6;

            case SP_BDSP_DialgaPalkia:      return 0;

            case SP_COUNT:                  return -1;
        }
    }

    static bool getProgramEnableUI(SmartProgram sp)
    {
        switch (sp)
        {
        case SP_BrightnessMeanFinder:
        case SP_SurpriseTrade:
            return true;
        default:
            return false;
        }
    }

    static bool getProgramEnableControl(SmartProgram sp)
    {
        switch (sp)
        {
        case SP_BrightnessMeanFinder:
            return true;
        default:
            return false;
        }
    }

    static QString getProgramGamePrefix(SmartProgram sp)
    {
        switch (sp)
        {
        case SP_BDSP_DialgaPalkia:
            return "BDSP";
        case SP_DelayCalibrator:
        case SP_BrightnessMeanFinder:
            return "Others";
        default:
            return "SwSh";
        }
    }

    bool run();
    virtual void stop();
    virtual SmartProgram getProgramEnum() = 0;
    QString getProgramName() { return getProgramNameFromEnum(getProgramEnum()); }
    QString getProgramInternalName() { return getProgramInternalNameFromEnum(getProgramEnum()); }

signals:
    void printLog(QString const log, QColor color = QColor(0,0,0));
    void completed();
    void runSequence(QString const sequence);

public slots:
    void commandFinished();
    void imageReady(int id, const QImage &image);
    void imageError(int id, QCameraImageCapture::Error error, const QString &errorString);
    void runStateLoop();

protected:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void runNextStateContinue() {m_runNextState = true;}
    void runNextStateDelay(int milliseconds = 1000);

    // Capture analysis
    struct HSVRange
    {
        HSVRange(int minH, int minS, int minV, int maxH, int maxS, int maxV)
        {
            m_minHSV.setHsv(minH,minS,minV);
            m_maxHSV.setHsv(maxH,maxS,maxV);
        }
        HSVRange(QColor minHSV, QColor maxHSV)
        {
            Q_ASSERT(minHSV.spec() == QColor::Hsv);
            Q_ASSERT(maxHSV.spec() == QColor::Hsv);
            m_minHSV = minHSV;
            m_maxHSV = maxHSV;
        }

        QColor min() const {return m_minHSV;}
        QColor max() const {return m_maxHSV;}

    private:
        QColor m_minHSV;
        QColor m_maxHSV;
    };

    bool checkColorMatch(QColor testColor, QColor targetColor, int threshold = 10);
    bool checkColorMatchHSV(QColor testColor, HSVRange hsvRange);

    bool checkPixelColorMatch(QPoint pixelPos, QColor targetColor, int threshold = 10);
    QColor getAverageColor(QRect rectPos);
    bool checkAverageColorMatch(QRect rectPos, QColor targetColor, int threshold = 10);
    double getBrightnessMean(QRect rectPos, HSVRange hsvRange);
    bool checkBrightnessMeanTarget(QRect rectPos, HSVRange hsvRange, double target);

    typedef int Command;
    bool inializeCommands(int size);

    enum State
    {
        S_Error,
        S_NotStarted,
        S_Completed,
        S_CommandRunning,
        S_CommandFinished,
        S_TakeScreenshot,
        S_TakeScreenshotFinished,
        S_CaptureRequested,
        S_CaptureReady,
    };

    // State functions
    State getState() { return m_state; }
    void setState_completed() { m_state = S_Completed; }
    void setState_runCommand(Command commandIndex);
    void setState_runCommand(QString const& customCommand);
    void setState_frameAnalyzeRequest();
    void setState_error(QString _errorMsg) { m_state = S_Error; m_errorMsg = _errorMsg; }

    Command m_commandIndex;
    QString m_customCommand;
    QMap<Command, QString> m_commands;

    //QCameraImageCapture* m_cameraCapture;
    //QCameraViewfinder* m_cameraView;
    QImage m_capture;
    SmartProgramParameter m_parameters;

    QString m_screenshotName;

private:
    QString m_errorMsg;
    State m_state;
    bool m_runNextState;
    QTimer m_runStateTimer;
    QTimer m_runStateDelayTimer;
};

#endif // SMARTPROGRAMBASE_H
