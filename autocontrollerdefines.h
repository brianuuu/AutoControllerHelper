#ifndef AUTOCONTROLLERDEFINES_H
#define AUTOCONTROLLERDEFINES_H

#define VERSION "6.4.0"
#define DEBUG_ENABLED true

#if DEBUG_ENABLED
    #define ROOT_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/"
    #define HEX_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/Hex/"
    #define SOURCE_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/SourceCode/"
    #define BOT_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/SourceCode/Bots/"
    #define SCREENSHOT_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/Screenshots/"
    #define LOG_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/Logs/"
    #define SMART_COMMAND_PATH "../AutoControllerHelper/SmartCommands/"
    #define SMART_STATS_INI "C:/Users/User/Documents/GitHub/AutoController_swsh/Resources/SmartStats.ini"
    #define STREAM_COUNTER_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/StreamCounters/"
    #define RESOURCES_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/Resources/"
#else
    #define ROOT_PATH "../"
    #define HEX_PATH "../Hex/"
    #define SOURCE_PATH "../SourceCode/"
    #define BOT_PATH "../SourceCode/Bots/"
    #define SCREENSHOT_PATH "../Screenshots/"
    #define LOG_PATH "../Logs/"
    #define SMART_COMMAND_PATH "../Resources/SmartCommands/"
    #define SMART_STATS_INI "../Resources/SmartStats.ini"
    #define STREAM_COUNTER_PATH "../StreamCounters/"
    #define RESOURCES_PATH "../Resources/"
#endif

#define SET_BIT(var,pos) (var |= (1U << pos))
#define CLEAR_BIT(var,pos) (var &= ~(1U << pos))
#define CHECK_BIT(var,pos) (var & (1U << pos))
#define COMMAND_MAX 30
#define SMART_HEX_VERSION 8

#define LOG_SUCCESS QColor(0,170,0)
#define LOG_WARNING QColor(255,120,0)
#define LOG_ERROR QColor(255,0,0)
#define LOG_IMPORTANT QColor(255,0,255)

#endif // AUTOCONTROLLERDEFINES_H
