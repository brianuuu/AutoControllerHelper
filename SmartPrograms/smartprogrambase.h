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
#include "../discordsettings.h"

enum SmartProgram : uint32_t
{
    SP_DelayCalibrator = 0,
    SP_BrightnessMeanFinder,
    SP_ColorCalibrator,
    SP_SoundDetection,
    SP_Test,
    SP_CodeEntry,
    SP_HomeSorter,

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
    SP_EggOperation,
    SP_BoxRelease,
    SP_TradePartnerFinder,
    SP_MaxLair,

    SP_BDSP_Starter,
    SP_BDSP_MenuGlitch113,
    SP_BDSP_BoxDuplication,
    SP_BDSP_BoxOperation,
    SP_BDSP_ShinyLegendary,
    SP_BDSP_DuplicateItem1to30,
    SP_BDSP_EggOperation,

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
    SP_SV_GimmighoulFarmer,
    SP_SV_TradePartnerFinder,
    SP_SV_ItemPrinter,

    SP_TOTK_BowFuseDuplication,
    SP_TOTK_MineruDuplication,
    SP_TOTK_ShieldSurfDuplication,
    SP_TOTK_ZonaiDeviceDuplication,

    SP_S3_TableturfSkip,
    SP_S3_TableturfCloneJelly,

    SP_PLZA_RespawnReset,
    SP_PLZA_BenchReset,
    SP_PLZA_RestaurantBattler,
    SP_PLZA_Fossil,

    SP_COUNT
};

struct SmartProgramParameter
{
    VLCWrapper* vlcWrapper;
    DiscordSettings* discordSettings;
    SmartProgramSetting* settings;
    QLabel* statsLabel;
    QWidget* parent;
    QGraphicsScene* preview;
    QGraphicsScene* previewMasked;

    SmartProgramParameter(
            VLCWrapper* _vlcWrapper,
            DiscordSettings* _discordSettings,
            SmartProgramSetting* _settings,
            QLabel* _statsLabel,
            QWidget* _parent = nullptr,
            QGraphicsScene* _preview = nullptr,
            QGraphicsScene* _previewMasked = nullptr
            )
        : vlcWrapper(_vlcWrapper)
        , discordSettings(_discordSettings)
        , settings(_settings)
        , statsLabel(_statsLabel)
        , parent(_parent)
        , preview(_preview)
        , previewMasked(_previewMasked)
    {}
};

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
            case SP_CodeEntry:              return "Code Entry";
            case SP_HomeSorter:             return "HOME Sorter";
#if DEBUG_ENABLED
            case SP_Test:                   return "Test Program";
#endif

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
            case SP_EggOperation:           return "Egg Operation";
            case SP_BoxRelease:             return "Box Release";
            case SP_TradePartnerFinder:     return "Trade Partner Finder";
            case SP_MaxLair:                return "Auto Dynamax Adventure";

            case SP_BDSP_Starter:           return "Reset Starter";
            case SP_BDSP_MenuGlitch113:     return "Menu Glitch v1.1.3";
            case SP_BDSP_BoxDuplication:    return "Box Duplication Glitch";
            case SP_BDSP_BoxOperation:      return "Box Operation";
            case SP_BDSP_ShinyLegendary:    return "Reset Legendary";
            case SP_BDSP_DuplicateItem1to30:return "Duplicate Item 1 to 30";
            case SP_BDSP_EggOperation:      return "Egg Operation (BDSP)";

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
            case SP_SV_GimmighoulFarmer:    return "Gimmighoul Farmer";
            case SP_SV_TradePartnerFinder:  return "Trade Partner Finder (SV)";
            case SP_SV_ItemPrinter:         return "Item Printer Exploit";

            case SP_TOTK_BowFuseDuplication:    return "Bow Fuse Duplication";
            case SP_TOTK_MineruDuplication:     return "Mineru Duplication";
            case SP_TOTK_ShieldSurfDuplication: return "Shield Surf Duplication";
            case SP_TOTK_ZonaiDeviceDuplication:return "Zonai Device Duplication";

            case SP_S3_TableturfSkip:       return "Tableturf Skipper";
            case SP_S3_TableturfCloneJelly: return "Tableturf Clone Jelly";

            case SP_PLZA_RespawnReset:      return "Respawn Reset";
            case SP_PLZA_BenchReset:        return "Bench Reset";
            case SP_PLZA_RestaurantBattler: return "Restaurant Battler";
            case SP_PLZA_Fossil:            return "Auto Fossil";

            default:                        return "Invalid";
        }
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
            case SP_CodeEntry:              return "SmartCodeEntry";
            case SP_HomeSorter:             return "SmartHomeSorter";

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
            case SP_EggOperation:           return "SmartEggOperation";
            case SP_BoxRelease:             return "SmartBoxReleases";
            case SP_TradePartnerFinder:     return "SmartTradePartnerFinder";
            case SP_MaxLair:                return "SmartMaxLair";

            case SP_BDSP_Starter:           return "SmartBDSPStarter";
            case SP_BDSP_MenuGlitch113:     return "SmartBDSPMenuGlitch113";
            case SP_BDSP_BoxDuplication:    return "SmartBDSPBoxDuplication";
            case SP_BDSP_BoxOperation:      return "SmartBDSPBoxOperation";
            case SP_BDSP_ShinyLegendary:    return "SmartBDSPShinyLegendary";
            case SP_BDSP_DuplicateItem1to30:return "SmartBDSPDuplicateItem1to30";
            case SP_BDSP_EggOperation:      return "SmartBDSPEggOperation";

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
            case SP_SV_GimmighoulFarmer:    return "SmartSVGimmighoulFarmer";
            case SP_SV_TradePartnerFinder:  return "SmartSVTradePartnerFinder";
            case SP_SV_ItemPrinter:         return "SmartSVItemPrinter";

            case SP_TOTK_BowFuseDuplication:    return "SmartTOTKBowFuseDuplication";
            case SP_TOTK_MineruDuplication:     return "SmartTOTKMineruDuplication";
            case SP_TOTK_ShieldSurfDuplication: return "SmartTOTKShieldSurfDuplication";
            case SP_TOTK_ZonaiDeviceDuplication:return "SmartTOTKZonaiDeviceDuplication";

            case SP_S3_TableturfSkip:       return "SmartS3TableturfSkip";
            case SP_S3_TableturfCloneJelly: return "SmartS3TableturfCloneJelly";

            case SP_PLZA_RespawnReset:      return "SmartPLZARespawnReset";
            case SP_PLZA_BenchReset:        return "SmartPLZABenchReset";
            case SP_PLZA_RestaurantBattler: return "SmartPLZARestaurantBattler";
            case SP_PLZA_Fossil:            return "SmartPLZAFossil";

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
            case SP_CodeEntry:              return 19;
            case SP_HomeSorter:             return 14;

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
            case SP_EggOperation:           return 20;
            case SP_BoxRelease:             return 4;
            case SP_TradePartnerFinder:     return 18;
            case SP_MaxLair:                return 21;

            case SP_BDSP_Starter:           return 7;
            case SP_BDSP_MenuGlitch113:     return 0;
            case SP_BDSP_BoxDuplication:    return 4;
            case SP_BDSP_BoxOperation:      return 10;
            case SP_BDSP_ShinyLegendary:    return 11;
            case SP_BDSP_DuplicateItem1to30:return 0;
            case SP_BDSP_EggOperation:      return 20;

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
            case SP_SV_GimmighoulFarmer:    return 0;
            case SP_SV_TradePartnerFinder:  return 18;
            case SP_SV_ItemPrinter:         return 22;

            case SP_TOTK_BowFuseDuplication:    return 4;
            case SP_TOTK_MineruDuplication:     return 4;
            case SP_TOTK_ShieldSurfDuplication: return 4;
            case SP_TOTK_ZonaiDeviceDuplication:return 4;

            case SP_S3_TableturfSkip:       return 0;
            case SP_S3_TableturfCloneJelly: return 0;

            case SP_PLZA_RespawnReset:      return 0;
            case SP_PLZA_BenchReset:        return 0;
            case SP_PLZA_RestaurantBattler: return 0;
            case SP_PLZA_Fossil:            return 23;

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
        case SP_CodeEntry:
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

    static bool getProgramRunWithoutSerial(SmartProgram sp)
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
        case SP_HomeSorter:

        case SP_SurpriseTrade:
        case SP_EggOperation:
        case SP_MaxLair:

        case SP_BDSP_Starter:
        case SP_BDSP_ShinyLegendary:
        case SP_BDSP_EggOperation:

        case SP_PLA_NuggetFarmer:
        case SP_PLA_ResetAlphaPokemon:
        case SP_PLA_OutbreakFinder:
        case SP_PLA_PastureSorter:

        case SP_SV_SurpriseTrade:
        case SP_SV_EggOperation:

        case SP_S3_TableturfCloneJelly:
            return true;
        default:
            return false;
        }
    }

    static QString getProgramGamePrefix(SmartProgram sp)
    {
        if (sp >= SP_PLZA_RespawnReset)
        {
            return "Pokemon Legends: Z-A";
        }

        if (sp >= SP_S3_TableturfSkip)
        {
            return "Splatoon 3";
        }

        if (sp >= SP_TOTK_BowFuseDuplication)
        {
            return "Tears of the Kingdom";
        }

        if (sp >= SP_SV_ItemDuplication)
        {
            return "Pokemon Scarlet/Violet";
        }

        if (sp >= SP_PLA_NuggetFarmer)
        {
            return "Pokemon Legends: Arceus";
        }

        if (sp >= SP_BDSP_Starter)
        {
            return "Pokemon BDSP";
        }

        if (sp >= SP_PurpleBeamFinder)
        {
            return "Pokemon Sword/Shield";
        }

        return "Others";
    }

    typedef QPair<int, QString> Stat;

    bool run();
    virtual void stop();
    virtual SmartProgram getProgramEnum() = 0;
    QString getProgramName() { return getProgramNameFromEnum(getProgramEnum()); }
    QString getProgramInternalName() { return getProgramInternalNameFromEnum(getProgramEnum()); }
    QString getLogFileName() { return m_logFileName; }

    static bool validateCommand(QString const& commands, QString& errorMsg);

    static bool checkColorMatch(QColor testColor, QColor targetColor, int threshold = 10);
    static bool checkColorMatchHSV(QColor testColor, HSVRange hsvRange);

signals:
    void printLog(QString const log, QColor color = QColor(0,0,0));
    void completed();
    void runSequence(QString const sequence);
    void enableResetStats(bool enabled);

public slots:
    void commandFinished();
    void runStateLoop();
    void printLogExternal(QString const log, QColor color);

    // Tesseract OCR (text recognition)
    void on_OCRErrorOccurred(QProcess::ProcessError error);
    void on_OCRFinished();

    // discord messages
    void sendRegularDiscordMessage();

protected:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void runNextStateContinue() { m_runNextState = true; }
    void runNextStateDelay(int milliseconds = 1000);

    // Single pixcel color
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
    void setState_ocrRequest(QImage const& image);
    void setState_error(QString _errorMsg) { m_state = S_Error; m_errorMsg = _errorMsg; }

    void initStat(Stat& stat, QString const& key);
    void incrementStat(Stat& stat, int addCount = 1);
    void updateStats();

public:
    // Discord messages
    void sendDiscordMessage(QString const& title, bool isMention, QColor color = QColor(0,0,0), QImage const* img = nullptr, QList<Discord::EmbedField> const& fields = {});
    void sendDiscordError(QString const& title, QColor color = QColor(0,0,0), QImage const* img = nullptr, QList<Discord::EmbedField> const& fields = {});

protected:
    void sendDiscordMessage(QString const& title, bool isMention, bool isError, QColor color = QColor(0,0,0), QImage const* img = nullptr, QList<Discord::EmbedField> const& fields = {});

protected:
    AudioManager*           m_audioManager;
    VideoManager*           m_videoManager;
    DiscordSettings*        m_discordSettings;
    QImage                  m_capture;
    SmartProgramSetting*    m_settings;
    QLabel*                 m_statsLabel;
    QGraphicsScene*         m_preview;
    QGraphicsScene*         m_previewMasked;
    QDateTime               m_programStartDateTime;
    bool                    m_hadDiscordMessage;

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
    QImage*     m_ocrCustomImage;
};

#endif // SMARTPROGRAMBASE_H
