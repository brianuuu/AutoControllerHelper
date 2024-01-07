#ifndef POKEMONSTATTABLEWIDGET_H
#define POKEMONSTATTABLEWIDGET_H

#include "../../pokemondatabase.h"
#include <QComboBox>
#include <QDebug>
#include <QHeaderView>
#include <QSpinBox>
#include <QTableWidget>
#include <QToolButton>

class PokemonStatTable
{
public:
    PokemonStatTable();
    bool Match(PokemonStatTable const& other);
    int Count(PokemonStatTable const& other, bool ignoreGender = true);
    bool Compare(PokemonStatTable const& oldTable, PokemonStatTable const& newTable, bool ignoreGender = true);

public:
    int m_target;
    IVType m_ivs[StatType::ST_COUNT];
    NatureType m_nature;
    GenderType m_gender;
    ShinyType m_shiny;
};
typedef QVector<PokemonStatTable> PokemonStatTableList;

class PokemonStatTableWidget : public QTableWidget
{
public:
    enum Mode : int
    {
        // no restrictions, can be empty
        Default,

        // auto create first slot default to be shiny
        Shiny,

        // only one slot allowed, disable shiny
        Parent,

        // auto create first and only slot for shiny
        SingleShiny,
    };

public:
    explicit PokemonStatTableWidget(QWidget *parent = nullptr);

    // public methods
    void SetMode(Mode mode);
    PokemonStatTableList GetTableList() const;


private:
    enum ColumnType : int
    {
        CT_Add = 0,
        CT_Target,
        CT_Shiny,
        CT_HP,
        CT_Attack,
        CT_Defense,
        CT_SpAtk,
        CT_SpDef,
        CT_Speed,
        CT_Nature,
        CT_Gender,
    };

private slots:
    void OnAddButtonPressed();

private:
    void AddPokemon();
    void AddDummyRow();
    void Clear();

    Mode m_mode;
};

#endif // POKEMONSTATTABLEWIDGET_H
