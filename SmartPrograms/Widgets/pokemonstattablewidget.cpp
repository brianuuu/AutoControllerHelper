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
    // check if other table completely match this

    // IV
    for (int i = 0; i < StatType::ST_COUNT; i++)
    {
        if (m_ivs[i] != IVType::IVT_Any && other.m_ivs[i] != IVType::IVT_Any && m_ivs[i] != other.m_ivs[i])
        {
            return false;
        }
    }

    // Nature
    if (m_nature != NatureType::NT_Any && other.m_nature != NatureType::NT_Any && m_nature != other.m_nature)
    {
        return false;
    }

    // Gender
    if (m_gender != GenderType::GT_Any && other.m_gender != GenderType::GT_Any && m_gender != other.m_gender)
    {
        return false;
    }

    // Shiny
    switch (m_shiny)
    {
    case ShinyType::SPT_Any:
        break;
    case ShinyType::SPT_Yes:
        switch (other.m_shiny)
        {
        case ShinyType::SPT_No:
            return false;
        default: break;
        }
        break;
    case ShinyType::SPT_Star:
        switch (other.m_shiny)
        {
        case ShinyType::SPT_Square:
        case ShinyType::SPT_No:
            return false;
        default: break;
        }
        break;
    case ShinyType::SPT_Square:
        switch (other.m_shiny)
        {
        case ShinyType::SPT_Star:
        case ShinyType::SPT_No:
            return false;
        default: break;
        }
        break;
    case ShinyType::SPT_No:
        switch (other.m_shiny)
        {
        case ShinyType::SPT_Yes:
        case ShinyType::SPT_Star:
        case ShinyType::SPT_Square:
            return false;
        default: break;
        }
        break;
    default: return false;
    }

    return true;
}

int PokemonStatTable::Count(const PokemonStatTable &other, bool ignoreGender)
{
    // check how many stats other table matches this
    int matchCount = 0;

    // IV
    for (int i = 0; i < StatType::ST_COUNT; i++)
    {
        if (m_ivs[i] == IVType::IVT_Any || other.m_ivs[i] == IVType::IVT_Any || m_ivs[i] == other.m_ivs[i])
        {
            matchCount++;
        }
    }

    // Nature
    if (m_nature == NatureType::NT_Any || other.m_nature == NatureType::NT_Any || m_nature == other.m_nature)
    {
        matchCount++;
    }

    // Gender
    if (!ignoreGender)
    {
        if (m_gender == GenderType::GT_Any || other.m_gender == GenderType::GT_Any || m_gender == other.m_gender)
        {
            matchCount++;
        }
    }

    // Shiny
    switch (m_shiny)
    {
    case ShinyType::SPT_Any:
        matchCount++;
        break;
    case ShinyType::SPT_Yes:
        switch (other.m_shiny)
        {
        case ShinyType::SPT_No:
            break;
        default:
            matchCount++;
            break;
        }
        break;
    case ShinyType::SPT_Star:
        switch (other.m_shiny)
        {
        case ShinyType::SPT_Square:
        case ShinyType::SPT_No:
            break;
        default:
            matchCount++;
            break;
        }
        break;
    case ShinyType::SPT_Square:
        switch (other.m_shiny)
        {
        case ShinyType::SPT_Star:
        case ShinyType::SPT_No:
            break;
        default:
            matchCount++;
            break;
        }
        break;
    case ShinyType::SPT_No:
        switch (other.m_shiny)
        {
        case ShinyType::SPT_Yes:
        case ShinyType::SPT_Star:
        case ShinyType::SPT_Square:
            break;
        default:
            matchCount++;
            break;
        }
        break;
    default: break;
    }

    return matchCount;
}

bool PokemonStatTable::Compare(const PokemonStatTable &oldTable, const PokemonStatTable &newTable, bool ignoreGender)
{
    return Count(oldTable, ignoreGender) < Count(newTable, ignoreGender);
}

PokemonStatTableWidget::PokemonStatTableWidget(QWidget *parent) : QTableWidget(parent)
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList headers = {"", "Target", "Shiny", "HP", "Attack", "Defense", "Sp. Atk", "Sp. Def", "Speed", "Nature", "Gender"};
    setColumnCount(headers.size());
    //setColumnWidth(CT_Add, 20);
    //setColumnWidth(CT_Target, 40);
    //setColumnWidth(CT_Shiny, 60);
    setHorizontalHeaderLabels(headers);

    // initial setup
    AddDummyRow();
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // set initial mode
    SetMode(Mode::Default);
}

void PokemonStatTableWidget::SetMode(PokemonStatTableWidget::Mode mode)
{
    m_mode = mode;
    Clear();

    // Add at least one pokemon for shiny/parent mode
    if (mode != Mode::Default)
    {
        AddPokemon();
    }

    // for parent mode, disable the + button
    {
        QToolButton* toolButton = qobject_cast<QToolButton*>(cellWidget(rowCount() - 1, CT_Add));
        toolButton->setEnabled(mode != Mode::Parent && mode != Mode::SingleShiny);
    }
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
        spinBox->setRange(1, m_mode == Mode::SingleShiny ? 25 : 100);
        spinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        spinBox->setFixedWidth(40);
        setCellWidget(row, CT_Target, spinBox);

        if (m_mode == Mode::Parent)
        {
            spinBox->setEnabled(false);
        }
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

            if (m_mode == Mode::SingleShiny)
            {
                comboBox->setEnabled(false);
            }
        }
    }

    // nature
    {
        QComboBox* comboBox = new QComboBox();
        for (int i = 0; i < NatureType::NT_COUNT; i++)
        {
            comboBox->addItem(PokemonDatabase::getNatureTypeName(NatureType(i), true));
        }
        setCellWidget(row, CT_Nature, comboBox);

        if (m_mode == Mode::SingleShiny)
        {
            comboBox->setEnabled(false);
        }
    }

    // gender
    {
        QComboBox* comboBox = new QComboBox();
        for (int i = 0; i < GenderType::GT_COUNT; i++)
        {
            comboBox->addItem(PokemonDatabase::getGenderTypeName(GenderType(i)));
        }
        setCellWidget(row, CT_Gender, comboBox);

        if (m_mode == Mode::SingleShiny)
        {
            comboBox->setEnabled(false);
        }
    }

    // shiny
    {
        QComboBox* comboBox = new QComboBox();
        for (int i = 0; i < ShinyType::SPT_COUNT; i++)
        {
            comboBox->addItem(PokemonDatabase::getShinyTypeName(ShinyType(i)));
        }
        if (row == 0 && (m_mode == Mode::Shiny || m_mode == Mode::SingleShiny))
        {
            comboBox->setCurrentIndex(ShinyType::SPT_Yes);
        }
        if (m_mode == Mode::Parent || m_mode == Mode::SingleShiny)
        {
            comboBox->setEnabled(false);
        }
        setCellWidget(row, CT_Shiny, comboBox);
    }

    // change + to - button
    QToolButton* toolButton = qobject_cast<QToolButton*>(cellWidget(row, CT_Add));
    toolButton->setText("-");
    if (row == 0 && (m_mode == Mode::Parent || m_mode == Mode::SingleShiny))
    {
        toolButton->setEnabled(false);
    }

    AddDummyRow();
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
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
