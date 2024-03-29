#include "pokemondatabase.h"

PokemonDatabase& PokemonDatabase::instance()
{
    static PokemonDatabase database;
    return database;
}

PokemonDatabase::PokemonDatabase()
{
    for (int atk = 0; atk < MT_COUNT; atk++)
    {
        for (int def = 0; def < MT_COUNT; def++)
        {
            m_typeMatchupTable[atk][def] = 1.0;
        }
    }

    m_typeMatchupTable[MT_Normal][MT_Rock] = 0.5;
    m_typeMatchupTable[MT_Normal][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Normal][MT_Ghost] = 0;

    m_typeMatchupTable[MT_Fire][MT_Fire] = 0.5;
    m_typeMatchupTable[MT_Fire][MT_Water] = 0.5;
    m_typeMatchupTable[MT_Fire][MT_Rock] = 0.5;
    m_typeMatchupTable[MT_Fire][MT_Dragon] = 0.5;
    m_typeMatchupTable[MT_Fire][MT_Grass] = 2.0;
    m_typeMatchupTable[MT_Fire][MT_Ice] = 2.0;
    m_typeMatchupTable[MT_Fire][MT_Bug] = 2.0;
    m_typeMatchupTable[MT_Fire][MT_Steel] = 2.0;

    m_typeMatchupTable[MT_Water][MT_Water] = 0.5;
    m_typeMatchupTable[MT_Water][MT_Grass] = 0.5;
    m_typeMatchupTable[MT_Water][MT_Dragon] = 0.5;
    m_typeMatchupTable[MT_Water][MT_Fire] = 2.0;
    m_typeMatchupTable[MT_Water][MT_Ground] = 2.0;
    m_typeMatchupTable[MT_Water][MT_Rock] = 2.0;

    m_typeMatchupTable[MT_Grass][MT_Fire] = 0.5;
    m_typeMatchupTable[MT_Grass][MT_Grass] = 0.5;
    m_typeMatchupTable[MT_Grass][MT_Poison] = 0.5;
    m_typeMatchupTable[MT_Grass][MT_Flying] = 0.5;
    m_typeMatchupTable[MT_Grass][MT_Bug] = 0.5;
    m_typeMatchupTable[MT_Grass][MT_Dragon] = 0.5;
    m_typeMatchupTable[MT_Grass][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Grass][MT_Water] = 2.0;
    m_typeMatchupTable[MT_Grass][MT_Ground] = 2.0;
    m_typeMatchupTable[MT_Grass][MT_Rock] = 2.0;

    m_typeMatchupTable[MT_Electric][MT_Ground] = 0.0;
    m_typeMatchupTable[MT_Electric][MT_Grass] = 0.5;
    m_typeMatchupTable[MT_Electric][MT_Electric] = 0.5;
    m_typeMatchupTable[MT_Electric][MT_Dragon] = 0.5;
    m_typeMatchupTable[MT_Electric][MT_Water] = 2.0;
    m_typeMatchupTable[MT_Electric][MT_Flying] = 2.0;

    m_typeMatchupTable[MT_Ice][MT_Fire] = 0.5;
    m_typeMatchupTable[MT_Ice][MT_Water] = 0.5;
    m_typeMatchupTable[MT_Ice][MT_Ice] = 0.5;
    m_typeMatchupTable[MT_Ice][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Ice][MT_Grass] = 2.0;
    m_typeMatchupTable[MT_Ice][MT_Ground] = 2.0;
    m_typeMatchupTable[MT_Ice][MT_Flying] = 2.0;
    m_typeMatchupTable[MT_Ice][MT_Dragon] = 2.0;

    m_typeMatchupTable[MT_Fighting][MT_Ghost] = 0.0;
    m_typeMatchupTable[MT_Fighting][MT_Poison] = 0.5;
    m_typeMatchupTable[MT_Fighting][MT_Flying] = 0.5;
    m_typeMatchupTable[MT_Fighting][MT_Psychic] = 0.5;
    m_typeMatchupTable[MT_Fighting][MT_Bug] = 0.5;
    m_typeMatchupTable[MT_Fighting][MT_Fairy] = 0.5;
    m_typeMatchupTable[MT_Fighting][MT_Normal] = 2.0;
    m_typeMatchupTable[MT_Fighting][MT_Ice] = 2.0;
    m_typeMatchupTable[MT_Fighting][MT_Rock] = 2.0;
    m_typeMatchupTable[MT_Fighting][MT_Dark] = 2.0;
    m_typeMatchupTable[MT_Fighting][MT_Steel] = 2.0;

    m_typeMatchupTable[MT_Poison][MT_Steel] = 0.0;
    m_typeMatchupTable[MT_Poison][MT_Poison] = 0.5;
    m_typeMatchupTable[MT_Poison][MT_Ground] = 0.5;
    m_typeMatchupTable[MT_Poison][MT_Rock] = 0.5;
    m_typeMatchupTable[MT_Poison][MT_Ghost] = 0.5;
    m_typeMatchupTable[MT_Poison][MT_Grass] = 2.0;
    m_typeMatchupTable[MT_Poison][MT_Fairy] = 2.0;

    m_typeMatchupTable[MT_Ground][MT_Flying] = 0.0;
    m_typeMatchupTable[MT_Ground][MT_Grass] = 0.5;
    m_typeMatchupTable[MT_Ground][MT_Bug] = 0.5;
    m_typeMatchupTable[MT_Ground][MT_Fire] = 2.0;
    m_typeMatchupTable[MT_Ground][MT_Electric] = 2.0;
    m_typeMatchupTable[MT_Ground][MT_Poison] = 2.0;
    m_typeMatchupTable[MT_Ground][MT_Rock] = 2.0;
    m_typeMatchupTable[MT_Ground][MT_Steel] = 2.0;

    m_typeMatchupTable[MT_Flying][MT_Electric] = 0.5;
    m_typeMatchupTable[MT_Flying][MT_Rock] = 0.5;
    m_typeMatchupTable[MT_Flying][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Flying][MT_Grass] = 2.0;
    m_typeMatchupTable[MT_Flying][MT_Fighting] = 2.0;
    m_typeMatchupTable[MT_Flying][MT_Bug] = 2.0;

    m_typeMatchupTable[MT_Psychic][MT_Dark] = 0.0;
    m_typeMatchupTable[MT_Psychic][MT_Psychic] = 0.5;
    m_typeMatchupTable[MT_Psychic][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Psychic][MT_Fighting] = 2.0;
    m_typeMatchupTable[MT_Psychic][MT_Poison] = 2.0;

    m_typeMatchupTable[MT_Bug][MT_Fire] = 0.5;
    m_typeMatchupTable[MT_Bug][MT_Fire] = 0.5;
    m_typeMatchupTable[MT_Bug][MT_Poison] = 0.5;
    m_typeMatchupTable[MT_Bug][MT_Flying] = 0.5;
    m_typeMatchupTable[MT_Bug][MT_Ghost] = 0.5;
    m_typeMatchupTable[MT_Bug][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Bug][MT_Fairy] = 0.5;
    m_typeMatchupTable[MT_Bug][MT_Grass] = 2.0;
    m_typeMatchupTable[MT_Bug][MT_Psychic] = 2.0;
    m_typeMatchupTable[MT_Bug][MT_Dark] = 2.0;

    m_typeMatchupTable[MT_Rock][MT_Fighting] = 0.5;
    m_typeMatchupTable[MT_Rock][MT_Ground] = 0.5;
    m_typeMatchupTable[MT_Rock][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Rock][MT_Fire] = 2.0;
    m_typeMatchupTable[MT_Rock][MT_Ice] = 2.0;
    m_typeMatchupTable[MT_Rock][MT_Flying] = 2.0;
    m_typeMatchupTable[MT_Rock][MT_Bug] = 2.0;

    m_typeMatchupTable[MT_Ghost][MT_Normal] = 0.0;
    m_typeMatchupTable[MT_Ghost][MT_Dark] = 0.5;
    m_typeMatchupTable[MT_Ghost][MT_Psychic] = 2.0;
    m_typeMatchupTable[MT_Ghost][MT_Ghost] = 2.0;

    m_typeMatchupTable[MT_Dragon][MT_Fairy] = 0.0;
    m_typeMatchupTable[MT_Dragon][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Dragon][MT_Dragon] = 2.0;

    m_typeMatchupTable[MT_Dark][MT_Fighting] = 0.5;
    m_typeMatchupTable[MT_Dark][MT_Dark] = 0.5;
    m_typeMatchupTable[MT_Dark][MT_Fairy] = 0.5;
    m_typeMatchupTable[MT_Dark][MT_Psychic] = 2.0;
    m_typeMatchupTable[MT_Dark][MT_Ghost] = 2.0;

    m_typeMatchupTable[MT_Steel][MT_Fire] = 0.5;
    m_typeMatchupTable[MT_Steel][MT_Water] = 0.5;
    m_typeMatchupTable[MT_Steel][MT_Electric] = 0.5;
    m_typeMatchupTable[MT_Steel][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Steel][MT_Ice] = 2.0;
    m_typeMatchupTable[MT_Steel][MT_Rock] = 2.0;
    m_typeMatchupTable[MT_Steel][MT_Fairy] = 2.0;

    m_typeMatchupTable[MT_Fairy][MT_Fire] = 0.5;
    m_typeMatchupTable[MT_Fairy][MT_Poison] = 0.5;
    m_typeMatchupTable[MT_Fairy][MT_Steel] = 0.5;
    m_typeMatchupTable[MT_Fairy][MT_Fighting] = 2.0;
    m_typeMatchupTable[MT_Fairy][MT_Dragon] = 2.0;
    m_typeMatchupTable[MT_Fairy][MT_Dark] = 2.0;
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
    default:                    return "invalid";
    }
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
    default:                    return "Unknown";
    }
}

QString PokemonDatabase::getStatTypeName(StatType type, bool fullName)
{
    switch (type)
    {
    case ST_HP:         return "HP";
    case ST_Attack:     return fullName ? "Attack" : "Atk";
    case ST_Defense:    return fullName ? "Defense" : "Def";
    case ST_SpAtk:      return fullName ? "Special Attack" : "SpA";
    case ST_SpDef:      return fullName ? "Special Defense" : "SpD";
    case ST_Speed:      return fullName ? "Speed" : "Spe";
    default:            return "INVALID";
    }
}

QString PokemonDatabase::getIVTypeName(IVType type)
{
    switch (type)
    {
    case IVT_Any:           return "Any"; // for UI only, not used for OCR
    case IVT_NoGood:        return "No Good";
    case IVT_Decent:        return "Decent";
    case IVT_PrettyGood:    return "Pretty Good";
    case IVT_VeryGood:      return "Very Good";
    case IVT_Fantastic:     return "Fantastic";
    case IVT_Best:          return "Best";
    default:                return "INVALID";
    }
}

QString PokemonDatabase::getNatureTypeName(NatureType type, bool fullName)
{
    switch (type)
    {
    case NT_Any:        return "Any";
    case NT_Neutral:    return "Neutral";
    case NT_Lonely:     return !fullName ? "Lonely" : "Lonely (+Atk,-Def)";
    case NT_Adamant:    return !fullName ? "Adamant" : "Adamant (+Atk,-SpA)";
    case NT_Naughty:    return !fullName ? "Naughty" : "Naughty (+Atk,-SpD)";
    case NT_Brave:      return !fullName ? "Brave" : "Brave (+Atk,-Spe)";
    case NT_Bold:       return !fullName ? "Bold" : "Bold (+Def,-Atk)";
    case NT_Impish:     return !fullName ? "Impish" : "Impish (+Def,-SpA)";
    case NT_Lax:        return !fullName ? "Lax" : "Lax (+Def,-SpD)";
    case NT_Relaxed:    return !fullName ? "Relaxed" : "Relaxed (+Def,-Spe)";
    case NT_Modest:     return !fullName ? "Modest" : "Modest (+SpA,-Atk)";
    case NT_Mild:       return !fullName ? "Mild" : "Mild (+SpA,-Def)";
    case NT_Rash:       return !fullName ? "Rash" : "Rash (+SpA,-SpD)";
    case NT_Quiet:      return !fullName ? "Quiet" : "Quiet (+SpA,-Spe)";
    case NT_Calm:       return !fullName ? "Calm" : "Calm (+SpD,-Atk)";
    case NT_Gentle:     return !fullName ? "Gentle" : "Gentle (+SpD,-Def)";
    case NT_Careful:    return !fullName ? "Careful" : "Careful (+SpD,-SpA)";
    case NT_Sassy:      return !fullName ? "Sassy" : "Sassy (+SpD,-Spe)";
    case NT_Timid:      return !fullName ? "Timid" : "Timid (+Spe,-Atk)";
    case NT_Hasty:      return !fullName ? "Hasty" : "Hasty (+Spe,-Def)";
    case NT_Jolly:      return !fullName ? "Jolly" : "Jolly (+Spe,-SpA)";
    case NT_Naive:      return !fullName ? "Naive" : "Naive (+Spe,-SpD)";
    default:            return "INVALID";
    }
}

QString PokemonDatabase::getGenderTypeName(GenderType type)
{
    switch (type)
    {
    case GT_Any:    return "Any";
    case GT_Male:   return "Male";
    case GT_Female: return "Female";
    default:        return "INVALID";
    }
}

QString PokemonDatabase::getShinyTypeName(ShinyType type)
{
    switch (type)
    {
    case SPT_Any:       return "Any";
    case SPT_Yes:       return "Yes";
    case SPT_Star:      return "Star";
    case SPT_Square:    return "Square";
    case SPT_No:        return "No";
    default:            return "INVALID";
    }
}

MoveType PokemonDatabase::getMoveTypeFromString(const QString &type)
{
    if (type == "bug") return MT_Bug;
    if (type == "dark") return MT_Dark;
    if (type == "dragon") return MT_Dragon;
    if (type == "electric") return MT_Electric;
    if (type == "fairy") return MT_Fairy;
    if (type == "fighting") return MT_Fighting;
    if (type == "fire") return MT_Fire;
    if (type == "flying") return MT_Flying;
    if (type == "ghost") return MT_Ghost;
    if (type == "grass") return MT_Grass;
    if (type == "ground") return MT_Ground;
    if (type == "ice") return MT_Ice;
    if (type == "normal") return MT_Normal;
    if (type == "poison") return MT_Poison;
    if (type == "psychic") return MT_Psychic;
    if (type == "rock") return MT_Rock;
    if (type == "steel") return MT_Steel;
    if (type == "water") return MT_Water;

    return MT_COUNT;
}

BallType PokemonDatabase::getBallTypeFromString(const QString &type)
{
    if (type == "beast-ball") return BT_Beast;
    if (type == "cherish-ball") return BT_Cherish;
    if (type == "dive-ball") return BT_Dive;
    if (type == "dream-ball") return BT_Dream;
    if (type == "dusk-ball") return BT_Dusk;
    if (type == "fast-ball") return BT_Fast;
    if (type == "friend-ball") return BT_Friend;
    if (type == "great-ball") return BT_Great;
    if (type == "heal-ball") return BT_Heal;
    if (type == "heavy-ball") return BT_Heavy;
    if (type == "level-ball") return BT_Level;
    if (type == "love-ball") return BT_Love;
    if (type == "lure-ball") return BT_Lure;
    if (type == "luxury-ball") return BT_Luxury;
    if (type == "master-ball") return BT_Master;
    if (type == "moon-ball") return BT_Moon;
    if (type == "nest-ball") return BT_Nest;
    if (type == "net-ball") return BT_Net;
    if (type == "park-ball") return BT_Park;
    if (type == "poke-ball") return BT_Poke;
    if (type == "premier-ball") return BT_Premier;
    if (type == "quick-ball") return BT_Quick;
    if (type == "repeat-ball") return BT_Repeat;
    if (type == "safari-ball") return BT_Safari;
    if (type == "sport-ball") return BT_Sport;
    if (type == "timer-ball") return BT_Timer;
    if (type == "ultra-ball") return BT_Ultra;

    return BT_COUNT;
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
// Pokemon
// -----------------------------------------------
NatureType PokemonDatabase::getNatureFromStats(StatType inc, StatType dec)
{
    if (inc == StatType::ST_COUNT && dec == StatType::ST_COUNT)
    {
        // inc and dec unknown, assume neutral
        return NatureType::NT_Neutral;
    }

    switch (inc)
    {
    case ST_Attack:
        switch (dec)
        {
        case ST_Defense:    return NatureType::NT_Lonely;
        case ST_SpAtk:      return NatureType::NT_Adamant;
        case ST_SpDef:      return NatureType::NT_Naughty;
        case ST_Speed:      return NatureType::NT_Brave;
        default: break;
        }
        break;
    case ST_Defense:
        switch (dec)
        {
        case ST_Attack:     return NatureType::NT_Bold;
        case ST_SpAtk:      return NatureType::NT_Impish;
        case ST_SpDef:      return NatureType::NT_Lax;
        case ST_Speed:      return NatureType::NT_Relaxed;
        default: break;
        }
        break;
    case ST_SpAtk:
        switch (dec)
        {
        case ST_Attack:     return NatureType::NT_Modest;
        case ST_Defense:    return NatureType::NT_Mild;
        case ST_SpDef:      return NatureType::NT_Rash;
        case ST_Speed:      return NatureType::NT_Quiet;
        default: break;
        }
        break;
    case ST_SpDef:
        switch (dec)
        {
        case ST_Attack:     return NatureType::NT_Calm;
        case ST_Defense:    return NatureType::NT_Gentle;
        case ST_SpAtk:      return NatureType::NT_Careful;
        case ST_Speed:      return NatureType::NT_Sassy;
        default: break;
        }
        break;
    case ST_Speed:
        switch (dec)
        {
        case ST_Attack:     return NatureType::NT_Timid;
        case ST_Defense:    return NatureType::NT_Hasty;
        case ST_SpAtk:      return NatureType::NT_Jolly;
        case ST_SpDef:      return NatureType::NT_Naive;
        default: break;
        }
        break;
    default: break;
    }

    // error
    return NatureType::NT_COUNT;
}

const PokemonDatabase::OCREntries &PokemonDatabase::getEntries_PokemonIV(GameLanguage gameLanguage)
{
    instance().getDatabase("PokemonCommon/PokemonIV", gameLanguage, instance().m_database_PokemonIV);
    return instance().m_database_PokemonIV[gameLanguage];
}

const PokemonDatabase::OCREntries &PokemonDatabase::getEntries_PokemonTypes(GameLanguage gameLanguage)
{
    instance().getDatabase("PokemonCommon/PokemonTypes", gameLanguage, instance().m_database_PokemonTypes);
    return instance().m_database_PokemonTypes[gameLanguage];
}

const PokemonDatabase::OCREntries &PokemonDatabase::getEntries_Pokeballs(GameLanguage gameLanguage)
{
    instance().getDatabase("PokemonCommon/Pokeballs", gameLanguage, instance().m_database_Pokeballs);
    return instance().m_database_Pokeballs[gameLanguage];
}

const QStringList &PokemonDatabase::getList_Pokeballs()
{
    instance().getList("PokemonCommon/Pokemon-Balls", instance().m_list_Pokeballs);
    return instance().m_list_Pokeballs;
}

void PokemonDatabase::populatePokeballs(QComboBox *cb)
{
    for (QString const& ball : getList_Pokeballs())
    {
        cb->addItem(QIcon(RESOURCES_PATH + QString("PokemonCommon/Balls/") + ball + ".png"), ball + " Ball");
        cb->setIconSize(QSize(24,24));
    }
}

double PokemonDatabase::typeMatchupMultiplier(MoveType atk, MoveType def1, MoveType def2)
{
    return instance().m_typeMatchupTable[atk][def1] * (def2 == MT_COUNT ? 1.0 : instance().m_typeMatchupTable[atk][def2]);
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
// Pokemon Sword & Shield
// -----------------------------------------------
const QStringList &PokemonDatabase::getList_SwShSprites()
{
    instance().getList("PokemonSwSh/Pokemon-Sprites", instance().m_list_SwShSprites);
    return instance().m_list_SwShSprites;
}

const QStringList &PokemonDatabase::getList_SwShMaxLairRental()
{
    instance().getList("PokemonSwSh/MaxLair/Pokemon-Rental", instance().m_list_SwShMaxLairRental);
    return instance().m_list_SwShMaxLairRental;
}

const QStringList &PokemonDatabase::getList_SwShMaxLairBoss()
{
    instance().getList("PokemonSwSh/MaxLair/Pokemon-Boss", instance().m_list_SwShMaxLairBoss);
    return instance().m_list_SwShMaxLairBoss;
}

const PokemonDatabase::OCREntries PokemonDatabase::getEntries_SwShMaxLairRental(GameLanguage gameLanguage)
{
    return getEntries_PokedexSubList(gameLanguage, getList_SwShMaxLairRental());
}

const PokemonDatabase::OCREntries PokemonDatabase::getEntries_SwShMaxLairBoss(GameLanguage gameLanguage)
{
    return getEntries_PokedexSubList(gameLanguage, getList_SwShMaxLairBoss());
}

const PokemonDatabase::OCREntries &PokemonDatabase::getEntries_SwShMaxLairAbilities(GameLanguage gameLanguage)
{
    instance().getDatabase("PokemonSwSh/MaxLair/Abilities", gameLanguage, instance().m_database_SwShMaxLairAbilities);
    return instance().m_database_SwShMaxLairAbilities[gameLanguage];
}

const PokemonDatabase::OCREntries &PokemonDatabase::getEntries_SwShMaxLairMoves(GameLanguage gameLanguage)
{
    instance().getDatabase("PokemonSwSh/MaxLair/Moves", gameLanguage, instance().m_database_SwShMaxLairMoves);
    return instance().m_database_SwShMaxLairMoves[gameLanguage];
}

const PokemonDatabase::OCREntries &PokemonDatabase::getEntries_SwShMaxLairMaxMoves(GameLanguage gameLanguage)
{
    instance().getDatabase("PokemonSwSh/MaxLair/MaxMoves", gameLanguage, instance().m_database_SwShMaxLairMaxMoves);
    return instance().m_database_SwShMaxLairMaxMoves[gameLanguage];
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
    // Use this function to lazily initialize list if it's not created yet
    if (!list.isEmpty()) return true;

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
