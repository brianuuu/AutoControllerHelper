#include "pokemonstattablewidget.h"

PokemonStatTable::PokemonStatTable()
{
    m_target = 0;
    for (int i = 0; i < StatType::ST_COUNT; i++)
    {
        m_ivs[i] = IVType::IVT_COUNT;
    }
    m_nature = NatureType::NT_COUNT;
    m_gender = GenderType::GT_COUNT;
    m_shiny = ShinyType::SPT_COUNT;
}

bool PokemonStatTable::Match(const PokemonStatTable &other)
{
    for (int i = 0; i < StatType::ST_COUNT; i++)
    {
        if (m_ivs[i] != IVType::IVT_Any && other.m_ivs[i] != IVType::IVT_Any && m_ivs[i] != other.m_ivs[i])
        {
            return false;
        }
    }

    if (m_nature != NatureType::NT_Any && other.m_nature != NatureType::NT_Any && m_nature != other.m_nature)
    {
        return false;
    }

    if (m_gender != GenderType::GT_Any && other.m_gender != GenderType::GT_Any && m_gender != other.m_gender)
    {
        return false;
    }

    if (m_shiny != ShinyType::SPT_Any && other.m_shiny != ShinyType::SPT_Any && m_shiny != other.m_shiny)
    {
        return false;
    }

    return true;
}

PokemonStatTableWidget::PokemonStatTableWidget(QWidget *parent) : QTableWidget(parent)
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList headers = {"", "Target", "HP", "Attack", "Defense", "Sp. Atk", "Sp. Def", "Speed", "Nature", "Gender", "Shiny"};
    setColumnCount(headers.size());
    setColumnWidth(CT_Add, 20);
    setHorizontalHeaderLabels(headers);

    AddDummyRow();
}

PokemonStatTableList PokemonStatTableWidget::GetTableList() const
{
    PokemonStatTableList list;

    for (int row = 0; row < rowCount() - 1; row++)
    {
        PokemonStatTable table;

        // target
        {
            QSpinBox* spinBox = qobject_cast<QSpinBox*>(cellWidget(row, CT_Target));
            table.m_target = spinBox->value();
        }

        // IV
        {
            int stat = 0;
            for (int column = CT_HP; column <= CT_Speed; column++)
            {
                QComboBox* comboBox = qobject_cast<QComboBox*>(cellWidget(row, column));
                table.m_ivs[stat++] = IVType(comboBox->currentIndex());
            }
        }

        // nature
        {
            QComboBox* comboBox = qobject_cast<QComboBox*>(cellWidget(row, CT_Nature));
            table.m_nature = NatureType(comboBox->currentIndex());
        }

        // gender
        {
            QComboBox* comboBox = qobject_cast<QComboBox*>(cellWidget(row, CT_Gender));
            table.m_gender = GenderType(comboBox->currentIndex());
        }

        // shiny
        {
            QComboBox* comboBox = qobject_cast<QComboBox*>(cellWidget(row, CT_Shiny));
            table.m_shiny = ShinyType(comboBox->currentIndex());
        }

        list.push_back(table);
    }

    return list;
}

void PokemonStatTableWidget::OnAddButtonPressed()
{
    for (int row = 0; row < rowCount(); row++)
    {
        if (cellWidget(row, CT_Add) == sender())
        {
            if (row == rowCount() - 1)
            {
                AddPokemon();
            }
            else
            {
                removeRow(row);
            }
            return;
        }
    }
}

void PokemonStatTableWidget::AddPokemon()
{
    int row = rowCount() - 1;

    // target box
    {
        QSpinBox* spinBox = new QSpinBox();
        spinBox->setRange(1, 100);
        spinBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        setCellWidget(row, CT_Target, spinBox);
    }

    // IV
    {
        for (int i = CT_HP; i <= CT_Speed; i++)
        {
            QComboBox* comboBox = new QComboBox();
            for (int j = 0; j < IVType::IVT_COUNT; j++)
            {
                comboBox->addItem(PokemonDatabase::getIVTypeName(IVType(j)));
            }
            setCellWidget(row, i, comboBox);
        }
    }

    // nature
    {
        QComboBox* comboBox = new QComboBox();
        for (int i = 0; i < NatureType::NT_COUNT; i++)
        {
            comboBox->addItem(PokemonDatabase::getNatureTypeName(NatureType(i)));
        }
        setCellWidget(row, CT_Nature, comboBox);
    }

    // gender
    {
        QComboBox* comboBox = new QComboBox();
        for (int i = 0; i < GenderType::GT_COUNT; i++)
        {
            comboBox->addItem(PokemonDatabase::getGenderTypeName(GenderType(i)));
        }
        setCellWidget(row, CT_Gender, comboBox);
    }

    // shiny
    {
        QComboBox* comboBox = new QComboBox();
        for (int i = 0; i < ShinyType::SPT_COUNT; i++)
        {
            comboBox->addItem(PokemonDatabase::getShinyTypeName(ShinyType(i)));
        }
        setCellWidget(row, CT_Shiny, comboBox);
    }

    // change + to - button
    QToolButton* toolButton = qobject_cast<QToolButton*>(cellWidget(row, CT_Add));
    toolButton->setText("-");

    AddDummyRow();
}

void PokemonStatTableWidget::AddDummyRow()
{
    // dummy row that addes a plus button at the first column
    QToolButton* pushButton = new QToolButton();
    pushButton->setText("+");
    connect(pushButton, &QToolButton::pressed, this, &PokemonStatTableWidget::OnAddButtonPressed);

    int row = rowCount();
    insertRow(row);
    setCellWidget(row, CT_Add, pushButton);
}

void PokemonStatTableWidget::Clear()
{
    while (rowCount() > 1)
    {
        removeRow(0);
    }
}
