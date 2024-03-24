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

enum StatType : uint8_t
{
    ST_HP = 0,
    ST_Attack,
    ST_Defense,
    ST_SpAtk,
    ST_SpDef,
    ST_Speed,

    ST_COUNT
};

enum IVType : uint8_t
{
    IVT_Any,
    IVT_NoGood,
    IVT_Decent,
    IVT_PrettyGood,
    IVT_VeryGood,
    IVT_Fantastic,
    IVT_Best,

    IVT_COUNT
};

enum NatureType : uint8_t
{
    NT_Any,
    NT_Neutral,
    NT_Lonely,
    NT_Adamant,
    NT_Naughty,
    NT_Brave,
    NT_Bold,
    NT_Impish,
    NT_Lax,
    NT_Relaxed,
    NT_Modest,
    NT_Mild,
    NT_Rash,
    NT_Quiet,
    NT_Calm,
    NT_Gentle,
    NT_Careful,
    NT_Sassy,
    NT_Timid,
    NT_Hasty,
    NT_Jolly,
    NT_Naive,

    NT_COUNT
};

enum GenderType : uint8_t
{
    GT_Any,
    GT_Male,
    GT_Female,

    GT_COUNT
};

enum ShinyType : uint8_t
{
    SPT_Any,
    SPT_Yes,
    SPT_Star,
    SPT_Square,
    SPT_No,

    SPT_COUNT
};

enum MoveType : uint8_t
{
    MT_Bug,
    MT_Dark,
    MT_Dragon,
    MT_Electric,
    MT_Fairy,
    MT_Fighting,
    MT_Fire,
    MT_Flying,
    MT_Ghost,
    MT_Grass,
    MT_Ground,
    MT_Ice,
    MT_Normal,
    MT_Poison,
    MT_Psychic,
    MT_Rock,
    MT_Steel,
    MT_Water,

    MT_COUNT
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
    static QString getStatTypeName(StatType type, bool fullName);
    static QString getIVTypeName(IVType type);
    static QString getNatureTypeName(NatureType type, bool fullName);
    static QString getGenderTypeName(GenderType type);
    static QString getShinyTypeName(ShinyType type);
    static MoveType getMoveTypeFromString(QString const& type);

    // PLA Enums
    static QString PLAAreaTypeToString(PLAAreaType type);
    static QString getPLACampString(PLAAreaType type, int campID, bool* valid = nullptr);
    static bool getIsPLACampSelectableAtVillage(PLAAreaType type, int campID);

public:
    typedef QMap<QString, QStringList> OCREntries;

    // Json functions
    static bool readJson(QString const& path, QJsonObject &jsonObject);

    // String manipulation
    static QString stringRemoveNonAlphaNumeric(QString const& str);
    static QString normalizeString(QString const& str);

    // Pokemon
    static NatureType getNatureFromStats(StatType inc, StatType dec);
    static OCREntries const& getEntries_PokemonIV(GameLanguage gameLanguage);
    static OCREntries const& getEntries_PokemonTypes(GameLanguage gameLanguage);
    static QStringList const& getList_PokeBalls();

    // Pokedex
    static OCREntries const& getEntries_PokedexNational(GameLanguage gameLanguage);
    static OCREntries const getEntries_PokedexSubList(GameLanguage gameLanguage, QStringList const& poekmonList);

    // Pokemon Legends: Arceus
    static QStringList const& getList_PLAMassOutbreak();
    static OCREntries const& getEntries_PLADistortion(GameLanguage gameLanguage);
    static OCREntries const getEntries_PLAMassOutbreak(GameLanguage gameLanguage);

    // Pokemon Sword & Shield
    static QStringList const& getList_SwShSprites();
    static QStringList const& getList_SwShMaxLairRental();
    static QStringList const& getList_SwShMaxLairBoss();
    static OCREntries const getEntries_SwShMaxLairRental(GameLanguage gameLanguage);
    static OCREntries const getEntries_SwShMaxLairBoss(GameLanguage gameLanguage);
    static OCREntries const& getEntries_SwShMaxLairAbilities(GameLanguage gameLanguage);
    static OCREntries const& getEntries_SwShMaxLairMoves(GameLanguage gameLanguage);
    static OCREntries const& getEntries_SwShMaxLairMaxMoves(GameLanguage gameLanguage);

private:
    typedef QMap<GameLanguage, OCREntries> Database;

    // Helper Functions
    static bool getList(QString const& name, QStringList& list);
    static bool getDatabase(QString const& name, GameLanguage gameLanguage, Database& database);

private:
    // Pokemon
    Database m_database_PokemonIV;
    Database m_database_PokemonTypes;
    QStringList m_list_Pokeballs;

    // Pokedex
    Database m_database_PokedexNational;

    // Pokemon Legends: Arceus
    QStringList m_list_PLAOutbreak;
    Database m_database_PLADistortion;

    // Pokemon Sword & Shield
    QStringList m_list_SwShSprites;
    QStringList m_list_SwShMaxLairRental;
    QStringList m_list_SwShMaxLairBoss;
    Database m_database_SwShMaxLairAbilities;
    Database m_database_SwShMaxLairMoves;
    Database m_database_SwShMaxLairMaxMoves;
};

#endif // POKEMONDATABASE_H
