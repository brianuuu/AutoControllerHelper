#ifndef SMARTPROGRAMBASE_H
#define SMARTPROGRAMBASE_H

#include "autocontrollerdefines.h"
#include "pokemondatabase.h"

#include <QCameraImageCapture>
#include <QCameraViewfinder>
#include <QDebug>
#include <QDomDocument>
#include <QGraphicsScene>
#include <QProcess>
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
    SP_SoundDetection,
    SP_Test,

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
    SP_PLA_DistortionWaiter,
    SP_PLA_OutbreakFinder,
    SP_PLA_PastureSorter,
    SP_PLA_StaticSpawn,
    SP_PLA_MMORespawn,

    SP_SV_ItemDuplication,
    SP_SV_SurpriseTrade,
    SP_SV_BoxRelease,
    SP_SV_BoxReleaseSafe,
    SP_SV_EggOperation,

    SP_COUNT
};

struct SmartProgramParameter
{
    VLCWrapper* vlcWrapper;
    SmartProgramSetting* settings;
    QLabel* statsLabel;
    QWidget* parent;
    QGraphicsScene* preview;
    QGraphicsScene* previewMasked;

    SmartProgramParameter(
            VLCWrapper* _vlcWrapper,
            SmartProgramSetting* _settings,
            QLabel* _statsLabel,
            QWidget* _parent = nullptr,
            QGraphicsScene* _preview = nullptr,
            QGraphicsScene* _previewMasked = nullptr
            )
        : vlcWrapper(_vlcWrapper)
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
            case SP_SoundDetection:         return "Dev: Sound Detection";
            case SP_Test:                   return "Test Program";

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
            case SP_PLA_ResetAlphaPokemon:  return "Reset Alpha Gallade/Crobat";
            case SP_PLA_BraviaryGainHeight: return "Braviary Gain Height";
            case SP_PLA_DistortionWaiter:   return "Distortion Waiter";
            case SP_PLA_OutbreakFinder:     return "Outbreak Finder";
            case SP_PLA_PastureSorter:      return "Pasture Sorter";
            case SP_PLA_StaticSpawn:        return "Static Spawn Reset";
            case SP_PLA_MMORespawn:         return "MMO Respawn Reset";

            case SP_SV_ItemDuplication:     return "Item Duplication";
            case SP_SV_SurpriseTrade:       return "Auto Surprise Trade (SV)";
            case SP_SV_BoxRelease:          return "Auto Box Release (SV)";
            case SP_SV_BoxReleaseSafe:      return "Auto Box Release Safe (SV)";
            case SP_SV_EggOperation:        return "Egg Operation (SV)";

            case SP_COUNT:                  return "Invalid";
        }
        return "Invalid";
    }

    static SmartProgram getProgramEnumFromName(QString const& str)
    {
        for (int i = 0; i < (int)SP_COUNT; i++)
        {
            SmartProgram sp = (SmartProgram)i;
            if (getProgramNameFromEnum(sp) == str)
            {
                return sp;
            }
        }

        return SP_COUNT;
    }

    static QString getProgramInternalNameFromEnum(SmartProgram sp)
    {
        switch (sp)
        {
            case SP_DelayCalibrator:        return "SmartDelayCalibrator";
            case SP_BrightnessMeanFinder:   return "SmartBrightnessMeanFinder";
            case SP_ColorCalibrator:        return "SmartColorCalibrator";
            case SP_SoundDetection:         return "SmartSoundDetection";
            case SP_Test:                   return "SmartTestProgram";

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
            case SP_PLA_DistortionWaiter:   return "SmartPLADistortionWaiter";
            case SP_PLA_OutbreakFinder:     return "SmartPLAOutbreakFinder";
            case SP_PLA_PastureSorter:      return "SmartPLAPastureSorter";
            case SP_PLA_StaticSpawn:        return "SmartPLAStaticSpawn";
            case SP_PLA_MMORespawn:         return "SmartPLAMMORespawn";

            case SP_SV_ItemDuplication:     return "SmartSVItemDuplication";
            case SP_SV_SurpriseTrade:       return "SmartSVSurpriseTrade";
            case SP_SV_BoxRelease:          return "SmartSVBoxRelease";
            case SP_SV_BoxReleaseSafe:      return "SmartSVBoxReleaseSafe";
            case SP_SV_EggOperation:        return "SmartSVEggOperation";

            case SP_COUNT:                  return "Invalid";
        }
        return "Invalid";
    }

    static int getProgramTabID(SmartProgram sp)
    {
        // -1: Not added to list, 0: No settings
        switch (sp)
        {
            case SP_DelayCalibrator:        return 0;
            case SP_BrightnessMeanFinder:   return 1;
            case SP_ColorCalibrator:        return 8;
            case SP_SoundDetection:         return 17;
            case SP_Test:                   return 0;

            case SP_PurpleBeamFinder:       return 2;
            case SP_YCommGlitch:            return 0;
            case SP_SurpriseTrade:          return 3;
            case SP_MaxRaidBattler:         return 4; // 4 is generic with one spinbox + label
            case SP_DaySkipper:             return 5;
            case SP_BattleTower:            return 0;
            case SP_Loto:                   return 6;
            case SP_DailyHighlight:         return 6;
            case SP_BerryFarmer:            return 6;
            case SP_WattFarmer:             return 6;

            case SP_BDSP_Starter:           return 7;
            case SP_BDSP_MenuGlitch113:     return 0;
            case SP_BDSP_BoxDuplication:    return 4;
            case SP_BDSP_BoxOperation:      return 10;
            case SP_BDSP_ShinyLegendary:    return 11;
            case SP_BDSP_DuplicateItem1to30:return 0;

            case SP_PLA_NuggetFarmer:       return 15;
            case SP_PLA_ResetAlphaPokemon:  return 12;
            case SP_PLA_BraviaryGainHeight: return 0;
            case SP_PLA_DistortionWaiter:   return 0;
            case SP_PLA_OutbreakFinder:     return 13;
            case SP_PLA_PastureSorter:      return 14;
            case SP_PLA_StaticSpawn:        return 16;
            case SP_PLA_MMORespawn:         return 0;

            case SP_SV_ItemDuplication:     return 4;
            case SP_SV_SurpriseTrade:       return 4;
            case SP_SV_BoxRelease:          return 4;
            case SP_SV_BoxReleaseSafe:      return 4;
            case SP_SV_EggOperation:        return 9;

            case SP_COUNT:                  return -1;
        }
        return 0;
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
        case SP_SoundDetection:
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
        case SP_SoundDetection:
        case SP_Test:
        case SP_BattleTower:
        case SP_BDSP_MenuGlitch113:
        case SP_BDSP_BoxDuplication:
        case SP_PLA_BraviaryGainHeight:
        case SP_SV_ItemDuplication:
        case SP_SV_BoxRelease:
        case SP_SV_BoxReleaseSafe:
            return false;
        default:
            return true;
        }
    }

    static QString getProgramGamePrefix(SmartProgram sp)
    {
        if (sp >= SP_SV_ItemDuplication)
        {
            return "Pokemon Scarlet/Violet";
        }
        else if (sp >= SP_PLA_NuggetFarmer)
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

    static bool validateCommand(QString const& commands, QString& errorMsg);

signals:
    void printLog(QString const log, QColor color = QColor(0,0,0));
    void completed();
    void runSequence(QString const sequence);
    void enableResetStats(bool enabled);

public slots:
    void commandFinished();
    void runStateLoop();

    // Tesseract OCR (text recognition)
    void on_OCRErrorOccurred(QProcess::ProcessError error);
    void on_OCRFinished();

protected:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void runNextStateContinue() { m_runNextState = true; }
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

    // Single pixcel color
    bool checkColorMatch(QColor testColor, QColor targetColor, int threshold = 10);
    bool checkColorMatchHSV(QColor testColor, HSVRange hsvRange);
    bool checkPixelColorMatch(QPoint pixelPos, QColor targetColor, int threshold = 10);

    // Average color in area
    QColor getAverageColor(QRect rectPos);
    bool checkAverageColorMatch(QRect rectPos, QColor targetColor, int threshold = 10);

    // Brightness Mean (white pixel ratio)
    double getBrightnessMean(QRect rectPos, HSVRange hsvRange);
    bool checkBrightnessMeanTarget(QRect rectPos, HSVRange hsvRange, double target);

    // Utils
    QImage getMonochromeImage(QRect rectPos, HSVRange hsvRange, bool whiteIsOne = true);

    // Image matching
    bool setImageMatchFromResource(QString const& name, QImage& image);
    double getImageSimilarRatio(QImage const& query, QImage const& database);
    double getImageMatch(QRect rectPos, HSVRange hsvRange, QImage const& testImage);
    bool checkImageMatchTarget(QRect rectPos, HSVRange hsvRange, QImage const& testImage, double target, QPoint* offset = nullptr);

    // Tesseract OCR (text recognition)
    bool startOCR(QRect rectPos, HSVRange hsvRange, bool isNumber = false);
    static int getLevenshteinDistance(QString const& a, QString const& b);
    static int getLevenshteinDistanceSubString(QString const& longStr, QString const& shortStr);
    static int matchSubStrings(QString const& query, QStringList const& subStrings, int* o_dist = nullptr);
    QString getOCRStringRaw();
    bool getOCRNumber(int& number);
    QString matchStringDatabase(PokemonDatabase::OCREntries const& entries);

    // Commands
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
        S_CaptureRequested,
        S_CaptureReady,
        S_OCRRequested,
        S_OCRReady,
    };

    // State functions
    State getState() { return m_state; }
    void setState_completed() { m_state = S_Completed; }
    void setState_runCommand(Command commandIndex, bool requestFrameAnalyze = false);
    void setState_runCommand(QString const& customCommand, bool requestFrameAnalyze = false);
    void setState_frameAnalyzeRequest();
    void setState_ocrRequest(QRect rect, HSVRange hsvRange);
    void setState_error(QString _errorMsg) { m_state = S_Error; m_errorMsg = _errorMsg; }

    void initStat(Stat& stat, QString const& key);
    void incrementStat(Stat& stat, int addCount = 1);
    void updateStats();

protected:
    AudioManager*           m_audioManager;
    VideoManager*           m_videoManager;
    QImage                  m_capture;
    SmartProgramSetting*    m_settings;
    QLabel*                 m_statsLabel;
    QGraphicsScene*         m_preview;
    QGraphicsScene*         m_previewMasked;

    // Commands
    Command m_commandIndex;
    QString m_customCommand;
    QMap<Command, QString> m_commands;

private:
    QString     m_logFileName;
    QString     m_errorMsg;
    State       m_state;
    bool        m_runNextState;
    QTimer      m_runStateTimer;
    QTimer      m_runStateDelayTimer;

    // OCR
    QProcess    m_ocrProcess;
    QRect       m_ocrRect;
    HSVRange    m_ocrHSVRange;
};

#endif // SMARTPROGRAMBASE_H
