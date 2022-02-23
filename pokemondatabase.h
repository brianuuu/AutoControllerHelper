#ifndef POKEMONDATABASE_H
#define POKEMONDATABASE_H

#include "autocontrollerdefines.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMessageBox>
#include <QWidget>

enum GameLanguage : uint8_t
{
    GL_English = 0,
    GL_ChineseSimplified,
    GL_ChineseTraditional,
    GL_French,
    GL_German,
    GL_Italian,
    GL_Japanese,
    GL_Korean,
    GL_Spanish,

    GL_COUNT
};

class PokemonDatabase
{
public:
    static QString getGameLanguagePrefix(GameLanguage sp)
    {
        switch (sp)
        {
        case GL_English:            return "eng";
        case GL_ChineseSimplified:  return "chi_sim";
        case GL_ChineseTraditional: return "chi_tra";
        case GL_French:             return "fra";
        case GL_German:             return "deu";
        case GL_Italian:            return "ita";
        case GL_Japanese:           return "jpn";
        case GL_Korean:             return "kor";
        case GL_Spanish:            return "spa";
        case GL_COUNT:              return "invalid";
        }
        return "invalid";
    }

    static QString getGameLanguageName(GameLanguage sp)
    {
        switch (sp)
        {
        case GL_English:            return "English";
        case GL_ChineseSimplified:  return "Chinese (Simplified)";
        case GL_ChineseTraditional: return "Chinese (Traditional)";
        case GL_French:             return "French";
        case GL_German:             return "German";
        case GL_Italian:            return "Italian";
        case GL_Japanese:           return "Japanese";
        case GL_Korean:             return "Korean";
        case GL_Spanish:            return "Spanish";
        case GL_COUNT:              return "Unknown";
        }
        return "Unknown";
    }

public:
    static PokemonDatabase& instance();

private:
    PokemonDatabase();

public:
    typedef QPair<QString, QStringList> OCREntry;
    typedef QVector<OCREntry> OCREntries;
    typedef QMap<GameLanguage, OCREntries> Database;

    // String manipulation
    static QString stringRemoveNonAlphaNumeric(QString const& str);
    static QString normalizeString(QString const& str);

    // Pokemon Legends: Arceus
    OCREntries const& getEntries_PLADistortion(GameLanguage gameLanguage);

private:
    // Json functions
    static bool readJson(QString const& path, QJsonObject &jsonObject);

    // Helper Functions
    static bool getDatabase(QString const& name, GameLanguage gameLanguage, Database& database);

protected:
    // Pokemon Legends: Arceus
    Database m_database_PLADistortion;
};

#endif // POKEMONDATABASE_H
