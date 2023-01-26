#ifndef POKEMONSTATTABLEWIDGET_H
#define POKEMONSTATTABLEWIDGET_H

#include "../../pokemondatabase.h"
#include <QComboBox>
#include <QDebug>
#include <QSpinBox>
#include <QTableWidget>
#include <QToolButton>

class PokemonStatTable
{
public:
    PokemonStatTable();
    bool Match(PokemonStatTable const& other);

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
    explicit PokemonStatTableWidget(QWidget *parent = nullptr);

    // public methods
    PokemonStatTableList GetTableList() const;

private:
    enum ColumnType : int
    {
        CT_Add = 0,
        CT_Target,
        CT_HP,
        CT_Attack,
        CT_Defense,
        CT_SpAtk,
        CT_SpDef,
        CT_Speed,
        CT_Nature,
        CT_Gender,
        CT_Shiny,
    };

private slots:
    void OnAddButtonPressed();

private:
    void AddPokemon();
    void AddDummyRow();
    void Clear();
};

#endif // POKEMONSTATTABLEWIDGET_H
