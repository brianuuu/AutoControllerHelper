#include "pokemondatabase.h"

PokemonDatabase& PokemonDatabase::instance()
{
    static PokemonDatabase database;
    return database;
}

PokemonDatabase::PokemonDatabase()
{

}

QString PokemonDatabase::getGameLanguagePrefix(GameLanguage sp)
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

QString PokemonDatabase::getGameLanguageName(GameLanguage sp)
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

QString PokemonDatabase::PLAAreaTypeToString(PLAAreaType type)
{
    switch (type)
    {
    case PLAAT_ObsidianFieldlands: return "Obsidian Fieldlands";
    case PLAAT_CrimsonMirelands:   return "Crimson Mirelands";
    case PLAAT_CobaltCoastlands:   return "Cobalt Coastlands";
    case PLAAT_CoronetHighlands:   return "Coronet Highlands";
    case PLAAT_AlabasterIcelands:  return "Alabaster Icelands";
    }
    return "INVALID AREA";
}

QString PokemonDatabase::getPLACampString(PLAAreaType type, int campID, bool *valid)
{
    if (valid)
    {
        *valid = true;
    }

    switch (type)
    {
    case PLAAT_ObsidianFieldlands:
    {
        switch (campID)
        {
        case 1: return "Fieldlands Camp";
        case 2: return "Heights Camp";
        case 3: return "Grandtree Arena";
        }
        break;
    }
    case PLAAT_CrimsonMirelands:

    {
        switch (campID)
        {
        case 1: return "Mirelands Camp";
        case 2: return "Bogbound Camp";
        case 3: return "Diamond Settlement";
        case 4: return "Brava Arena";
        }
        break;
    }
    case PLAAT_CobaltCoastlands:
    {
        switch (campID)
        {
        case 1: return "Beachside Camp";
        case 2: return "Coastlands Camp";
        case 3: return "Molten Arena";
        }
        break;
    }
    case PLAAT_CoronetHighlands:
    {
        switch (campID)
        {
        case 1: return "Highlands Camp";
        case 2: return "Mountain Camp";
        case 3: return "Summit Camp";
        case 4: return "Moonview Arena";
        }
        break;
    }
    case PLAAT_AlabasterIcelands:
    {
        switch (campID)
        {
        case 1: return "Snowfields Camp";
        case 2: return "Icepeak Camp";
        case 3: return "Pearl Settlement";
        case 4: return "Icepeak Arena";
        }
        break;
    }
    }

    if (valid)
    {
        *valid = false;
    }
    return "INVALID AREA/CAMP";
}

bool PokemonDatabase::getIsPLACampSelectableAtVillage(PLAAreaType type, int campID)
{
    bool selectable = campID > 0;
    switch (type)
    {
    case PLAAT_ObsidianFieldlands:
    case PLAAT_CrimsonMirelands:
    case PLAAT_CobaltCoastlands:
    case PLAAT_AlabasterIcelands:
        return selectable && campID < 3;
    case PLAAT_CoronetHighlands:
        return selectable && campID < 4;
    }
    return false;
}

// -----------------------------------------------
// String manipulation
// -----------------------------------------------
QString PokemonDatabase::stringRemoveNonAlphaNumeric(const QString &str)
{
    QString temp;
    for (QChar c : str)
    {
        if (c.isLetterOrNumber()
         || c == QChar(0x3099)  // Japanese dakuten
         || c == QChar(0x309A)) // Japanese handakuten
        {
            temp += c;
            continue;
        }
    }

    return temp;
}

QString PokemonDatabase::normalizeString(const QString &str)
{
    QString temp = str.normalized(QString::NormalizationForm_KD);
    temp = stringRemoveNonAlphaNumeric(temp);
    return temp.toLower();
}

// -----------------------------------------------
// Pokedex
// -----------------------------------------------
const PokemonDatabase::OCREntries &PokemonDatabase::getEntries_PokedexNational(GameLanguage gameLanguage)
{
    instance().getDatabase("PokemonCommon/PokemonName", gameLanguage, instance().m_database_PokedexNational);
    return instance().m_database_PokedexNational[gameLanguage];
}

const PokemonDatabase::OCREntries PokemonDatabase::getEntries_PokedexSubList(GameLanguage gameLanguage, const QStringList &poekmonList)
{
    OCREntries const& allPokemonEntries = getEntries_PokedexNational(gameLanguage);

    // Get only the entries for requested pokemon
    OCREntries pokemonSubList;
    for (QString const& pokemon : poekmonList)
    {
        auto iter = allPokemonEntries.find(pokemon);
        if (iter != allPokemonEntries.end())
        {
            pokemonSubList[pokemon] = iter.value();
        }
    }
    return pokemonSubList;
}

// -----------------------------------------------
// Pokemon Legends: Arceus
// -----------------------------------------------
const QStringList &PokemonDatabase::getList_PLAMassOutbreak()
{
    instance().getList("PokemonLA/Pokemon-MassOutbreak", instance().m_list_PLAOutbreak);
    return instance().m_list_PLAOutbreak;
}

const PokemonDatabase::OCREntries &PokemonDatabase::getEntries_PLADistortion(GameLanguage gameLanguage)
{
    instance().getDatabase("PokemonLA/DistortionNotification", gameLanguage, instance().m_database_PLADistortion);
    return instance().m_database_PLADistortion[gameLanguage];
}

const PokemonDatabase::OCREntries PokemonDatabase::getEntries_PLAMassOutbreak(GameLanguage gameLanguage)
{
    return getEntries_PokedexSubList(gameLanguage, getList_PLAMassOutbreak());
}

// -----------------------------------------------
// Json functions
// -----------------------------------------------
bool PokemonDatabase::readJson(const QString &path, QJsonObject& jsonObject)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
    {
        return false;
    }

    QJsonParseError error;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        return false;
    }

    jsonObject = jsonDocument.object();
    return true;
}

// -----------------------------------------------
// Helper functions
// -----------------------------------------------
bool PokemonDatabase::getList(const QString &name, QStringList &list)
{
    QFile file(RESOURCES_PATH + name + ".txt");
    if (file.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString const line = in.readLine();
            if (!list.contains(line))
            {
                list.push_back(line);
            }
        }

        file.close();
        return true;
    }

    return false;
}

bool PokemonDatabase::getDatabase(const QString &name, GameLanguage gameLanguage, Database &database)
{
    // Use this function to lazily initialize database if it's not created yet
    if (!database[gameLanguage].isEmpty()) return true;

    // Path is expected to be in RESOURCES_PATH already
    QJsonObject jsonObject;
    bool success = readJson(RESOURCES_PATH + name + "-" + getGameLanguagePrefix(gameLanguage) + ".json", jsonObject);
    if (!success || jsonObject.isEmpty())
    {
        return false;
    }

    // Read all entries from json file
    OCREntries entries;
    for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
    {
        //qDebug() << it.key();
        QStringList valueList;
        for (QJsonValueRef value : it.value().toArray())
        {
            //qDebug() << value.toString();
            valueList.push_back(normalizeString(value.toString()));
        }

        entries.insert(it.key(), valueList);
    }
    database.insert(gameLanguage, entries);
    return true;
}
