#ifndef AUTOCONTROLLERDEFINES_H
#define AUTOCONTROLLERDEFINES_H

#define DEBUG_ENABLED true

#if DEBUG_ENABLED
    #define ROOT_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/"
    #define HEX_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/Hex/"
    #define SOURCE_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/SourceCode/"
    #define BOT_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/SourceCode/Bots/"
    #define SCREENSHOT_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/Screenshots/"
    #define LOG_PATH "C:/Users/User/Documents/GitHub/AutoController_swsh/Logs/"
#else
    #define ROOT_PATH "../"
    #define HEX_PATH "../Hex/"
    #define SOURCE_PATH "../SourceCode/"
    #define BOT_PATH "../SourceCode/Bots/"
    #define SCREENSHOT_PATH "../Screenshots/"
    #define LOG_PATH "../Logs/"
#endif

#define COMMAND_MAX 30

#endif // AUTOCONTROLLERDEFINES_H
