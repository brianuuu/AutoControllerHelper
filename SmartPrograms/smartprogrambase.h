#ifndef SMARTPROGRAMBASE_H
#define SMARTPROGRAMBASE_H

#include "autocontrollerdefines.h"

#include <QCameraImageCapture>
#include <QCameraViewfinder>
#include <QDebug>
#include <QDomDocument>
#include <QGraphicsScene>
#include <QSettings>
#include <QTimer>
#include <QWidget>

#include <QtMath>

#include "../vlcwrapper.h"
#include "../smartprogramsetting.h"

enum SmartProgram : uint32_t
{
    SP_DelayCalibrator = 0,
    SP_BrightnessMeanFinder,
    SP_ColorCalibrator,

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

    SP_BDSP_Starter,
    SP_BDSP_MenuGlitch113,
    SP_BDSP_BoxDuplication,
    SP_BDSP_BoxOperation,
    SP_BDSP_ShinyLegendary,
    SP_BDSP_DuplicateItem1to30,

    SP_PLA_NuggetFarmer,
    SP_PLA_ResetAlphaPokemon,
    SP_PLA_BraviaryGainHeight,

    SP_COUNT
};

struct SmartProgramParameter
{
    QDomDocument* smartProgramCommands;
    VLCWrapper* vlcWrapper;
    SmartProgramSetting* settings;
    QLabel* statsLabel;
    QWidget* parent;
    QGraphicsScene* preview;
    QGraphicsScene* previewMasked;

    SmartProgramParameter(
            QDomDocument* _smartProgramCommands,
            VLCWrapper* _vlcWrapper,
            SmartProgramSetting* _settings,
            QLabel* _statsLabel,
            QWidget* _parent = nullptr,
            QGraphicsScene* _preview = nullptr,
            QGraphicsScene* _previewMasked = nullptr
            )
        : smartProgramCommands(_smartProgramCommands)
        , vlcWrapper(_vlcWrapper)
        , settings(_settings)
        , statsLabel(_statsLabel)
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
            case SP_ColorCalibrator:        return "Dev: Color Calibrator";

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

            case SP_BDSP_Starter:           return "Reset Starter";
            case SP_BDSP_MenuGlitch113:     return "Menu Glitch v1.1.3";
            case SP_BDSP_BoxDuplication:    return "Box Duplication Glitch";
            case SP_BDSP_BoxOperation:      return "Box Operation";
            case SP_BDSP_ShinyLegendary:    return "Reset Legendary";
            case SP_BDSP_DuplicateItem1to30:return "Duplicate Item 1 to 30";

            case SP_PLA_NuggetFarmer:       return "Nugget Farmer";
            case SP_PLA_ResetAlphaPokemon:  return "Reset Alpha Pokemon";
            case SP_PLA_BraviaryGainHeight: return "Braviary Gain Height";

            case SP_COUNT:                  return "Invalid";
        }
    }

    static SmartProgram getProgramEnumFromName(QString const& sp)
    {
        if (sp == "Dev: Camera Delay Checker")      return SP_DelayCalibrator;
        if (sp == "Dev: Brightness Mean Finder")    return SP_BrightnessMeanFinder;
        if (sp == "Dev: Color Calibrator")          return SP_ColorCalibrator;

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

        if (sp == "Reset Starter")                  return SP_BDSP_Starter;
        if (sp == "Menu Glitch v1.1.3")             return SP_BDSP_MenuGlitch113;
        if (sp == "Box Duplication Glitch")         return SP_BDSP_BoxDuplication;
        if (sp == "Box Operation")                  return SP_BDSP_BoxOperation;
        if (sp == "Reset Legendary")                return SP_BDSP_ShinyLegendary;
        if (sp == "Duplicate Item 1 to 30")         return SP_BDSP_DuplicateItem1to30;

        if (sp == "Nugget Farmer")                  return SP_PLA_NuggetFarmer;
        if (sp == "Reset Alpha Pokemon")            return SP_PLA_ResetAlphaPokemon;
        if (sp == "Braviary Gain Height")           return SP_PLA_BraviaryGainHeight;

        return SP_COUNT;
    }

    static QString getProgramInternalNameFromEnum(SmartProgram sp)
    {
        switch (sp)
        {
            case SP_DelayCalibrator:        return "SmartDelayCalibrator";
            case SP_BrightnessMeanFinder:   return "SmartBrightnessMeanFinder";
            case SP_ColorCalibrator:        return "SmartColorCalibrator";

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

            case SP_BDSP_Starter:           return "SmartBDSPStarter";
            case SP_BDSP_MenuGlitch113:     return "SmartBDSPMenuGlitch113";
            case SP_BDSP_BoxDuplication:    return "SmartBDSPBoxDuplication";
            case SP_BDSP_BoxOperation:      return "SmartBDSPBoxOperation";
            case SP_BDSP_ShinyLegendary:    return "SmartBDSPShinyLegendary";
            case SP_BDSP_DuplicateItem1to30:return "SmartBDSPDuplicateItem1to30";

            case SP_PLA_NuggetFarmer:       return "SmartPLANuggetFarmer";
            case SP_PLA_ResetAlphaPokemon:  return "SmartPLAResetAlphaPokemon";
            case SP_PLA_BraviaryGainHeight: return "SmartPLABraviaryGainHeight";

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
            case SP_ColorCalibrator:        return 8;

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

            case SP_BDSP_Starter:           return 7;
            case SP_BDSP_MenuGlitch113:     return 0;
            case SP_BDSP_BoxDuplication:    return 9;
            case SP_BDSP_BoxOperation:      return 10;
            case SP_BDSP_ShinyLegendary:    return 11;
            case SP_BDSP_DuplicateItem1to30:return 0;

            case SP_PLA_NuggetFarmer:       return 0;
            case SP_PLA_ResetAlphaPokemon:  return 0;
            case SP_PLA_BraviaryGainHeight: return 0;

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

    static bool getProgramExportLog(SmartProgram sp)
    {
        switch (sp)
        {
        // all simple program return false
        case SP_DelayCalibrator:
        case SP_BrightnessMeanFinder:
        case SP_ColorCalibrator:
        case SP_BattleTower:
        case SP_BDSP_MenuGlitch113:
        case SP_BDSP_BoxDuplication:
        case SP_PLA_BraviaryGainHeight:
            return false;
        default:
            return true;
        }
    }

    static QString getProgramGamePrefix(SmartProgram sp)
    {
        if (sp >= SP_PLA_NuggetFarmer)
        {
            return "Pokemon Legends: Arceus";
        }
        else if (sp >= SP_BDSP_Starter)
        {
            return "Pokemon BDSP";
        }
        else if (sp >= SP_PurpleBeamFinder)
        {
            return "Pokemon Sword/Shield";
        }
        else
        {
            return "Others";
        }
    }

    typedef QPair<int, QString> Stat;

    bool run();
    virtual void stop();
    virtual SmartProgram getProgramEnum() = 0;
    QString getProgramName() { return getProgramNameFromEnum(getProgramEnum()); }
    QString getProgramInternalName() { return getProgramInternalNameFromEnum(getProgramEnum()); }
    QString getLogFileName() { return m_logFileName; }

signals:
    void printLog(QString const log, QColor color = QColor(0,0,0));
    void completed();
    void runSequence(QString const sequence);
    void enableResetStats(bool enabled);

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

    bool setImageMatchFromResource(QString const& name, QImage& image);
    double getImageSimilarRatio(QImage const& query, QImage const& database);
    double getImageMatch(QRect rectPos, HSVRange hsvRange, QImage const& testImage);
    bool checkImageMatchTarget(QRect rectPos, HSVRange hsvRange, QImage const& testImage, double target, QPoint* offset = nullptr);

    typedef int Command;
    bool inializeCommands(int size);

    enum State
    {
        S_Error,
        S_NotStarted,
        S_Completed,
        S_CommandRunning,
        S_CommandRunningCaptureRequested,
        S_CommandFinished,
        S_TakeScreenshot,
        S_TakeScreenshotFinished,
        S_CaptureRequested,
        S_CaptureReady,
    };

    // State functions
    State getState() { return m_state; }
    void setState_completed() { m_state = S_Completed; }
    void setState_runCommand(Command commandIndex, bool requestFrameAnalyze = false);
    void setState_runCommand(QString const& customCommand, bool requestFrameAnalyze = false);
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

    void initStat(Stat& stat, QString const& key);
    void incrementStat(Stat& stat, int addCount = 1);
    void updateStats();

private:
    QString m_logFileName;
    QString m_errorMsg;
    State m_state;
    bool m_runNextState;
    QTimer m_runStateTimer;
    QTimer m_runStateDelayTimer;
};

#endif // SMARTPROGRAMBASE_H
