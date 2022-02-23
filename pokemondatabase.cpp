#include "pokemondatabase.h"

PokemonDatabase& PokemonDatabase::instance()
{
    static PokemonDatabase database;
    return database;
}

PokemonDatabase::PokemonDatabase()
{

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
// Pokemon Legends: Arceus
// -----------------------------------------------
const PokemonDatabase::OCREntries &PokemonDatabase::getEntries_PLADistortion(GameLanguage gameLanguage)
{
    getDatabase("PokemonLA/DistortionNotification", gameLanguage, m_database_PLADistortion);
    return m_database_PLADistortion[gameLanguage];
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

        entries.push_back(OCREntry(it.key(), valueList));
    }
    database.insert(gameLanguage, entries);
    return true;
}
