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

enum PLAAreaType : uint8_t
{
    PLAAT_ObsidianFieldlands,
    PLAAT_CrimsonMirelands,
    PLAAT_CobaltCoastlands,
    PLAAT_CoronetHighlands,
    PLAAT_AlabasterIcelands,
};

class PokemonDatabase
{
private:
    static PokemonDatabase& instance();
    PokemonDatabase();

public:
    // Game Enums
    static QString getGameLanguagePrefix(GameLanguage sp);
    static QString getGameLanguageName(GameLanguage sp);

    // PLA Enums
    static QString PLAAreaTypeToString(PLAAreaType type);
    static QString getPLACampString(PLAAreaType type, int campID, bool* valid = nullptr);
    static bool getIsPLACampSelectableAtVillage(PLAAreaType type, int campID);

public:
    typedef QMap<QString, QStringList> OCREntries;

    // String manipulation
    static QString stringRemoveNonAlphaNumeric(QString const& str);
    static QString normalizeString(QString const& str);

    // Pokedex
    static OCREntries const& getEntries_PokedexNational(GameLanguage gameLanguage);
    static OCREntries const getEntries_PokedexSubList(GameLanguage gameLanguage, QStringList const& poekmonList);

    // Pokemon Legends: Arceus
    static QStringList const& getList_PLAMassOutbreak();
    static OCREntries const& getEntries_PLADistortion(GameLanguage gameLanguage);
    static OCREntries const getEntries_PLAMassOutbreak(GameLanguage gameLanguage);

private:
    typedef QMap<GameLanguage, OCREntries> Database;

    // Json functions
    static bool readJson(QString const& path, QJsonObject &jsonObject);

    // Helper Functions
    static bool getList(QString const& name, QStringList& list);
    static bool getDatabase(QString const& name, GameLanguage gameLanguage, Database& database);

private:
    // Pokedex
    Database m_database_PokedexNational;

    // Pokemon Legends: Arceus
    QStringList m_list_PLAOutbreak;
    Database m_database_PLADistortion;
};

#endif // POKEMONDATABASE_H
