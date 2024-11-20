/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "qdlgeditlimits.h"
#include "ui_qdlgeditlimits.h"

#include <QAction>
#include <QMenuBar>
#include <QComboBox>
#include <QMessageBox>

// Constructor for "limt" editing dialog
QDlgEditLimits::QDlgEditLimits(QWidget *parent, radarParamPointer& param, bool bEditingLimits, bool bEditingCurrent) :
    QDialog(parent),
    _bEditingLimits(bEditingLimits),
    _bEditingCurrent(bEditingCurrent),
    _bUpdateParam(false),
    _param(param),
    _data(QVector<QVariant>()),
    ui(new Ui::QDlgEditLimits)
{
    ui->setupUi(this);
    init_menu();
    connect(ui->cbLimitType, SIGNAL(currentIndexChanged(int)), this, SLOT(limit_type_change(int)));

    if (_bEditingLimits)
    {
        if (_param !=nullptr)
        {
            fill_table();
            if ((_param->has_min_max())&&(_param->get_type()!=RPT_ENUM))
            {
                enable_min_max();
                ui->cbLimitType->setCurrentIndex(0);
                ui->leMin->setText(param->get_min_string());
                ui->leMax->setText(param->get_max_string());
            }
            else
            {
                enable_vector_table();

                ui->cbLimitType->setCurrentIndex(1);
            }
        }

        if (_param->get_type() == RPT_ENUM)
        {
            ui->cbLimitType->setCurrentIndex(1);
            ui->cbLimitType->setEnabled(false);
            ui->leMin->setEnabled(false);
            ui->leMax->setEnabled(false);
            ui->lblMin->setEnabled(false);
            ui->lblMax->setEnabled(false);
            ui->lblTitle->setEnabled(false);
            ui->lblTitle->hide();
            ui->cbLimitType->hide();
            ui->leMax->hide();
            ui->leMin->hide();
            ui->lblMax->hide();
            ui->lblMin->hide();
            enable_vector_table();
        }
        _data = _param->availableset_to_variant();
        fill_table();
    }

    if (!_bEditingLimits)
    {
        init_menu();
        enable_vector_table();
        ui->cbLimitType->setCurrentIndex(1);
        ui->cbLimitType->setEnabled(false);
        ui->leMin->setEnabled(false);
        ui->leMax->setEnabled(false);
        ui->lblMin->setEnabled(false);
        ui->lblMax->setEnabled(false);
        ui->lblTitle->setEnabled(false);
        ui->lblTitle->hide();
        ui->cbLimitType->hide();
        ui->leMax->hide();
        ui->leMin->hide();
        ui->lblMax->hide();
        ui->lblMin->hide();
        enable_vector_table();

        if ((_param->get_type()==RPT_ENUM)||(_param->has_available_set()))
            _avail_dataset = _param->availableset_to_variant();

        _data = _param->value_to_variant();

        fill_table();
    }

    connect(ui->buttonBox, SIGNAL(accepted()),this, SLOT(save_and_close()));
    connect(ui->buttonBox, SIGNAL(rejected()),this, SLOT(discard_and_close()));
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)),this, SLOT(clicked(QAbstractButton*)));
}



//------------------------------------------------
QDlgEditLimits::~QDlgEditLimits()
{
    delete ui;
}
//------------------------------------------------
void QDlgEditLimits::limit_type_change(int index)
{
    if (!_bEditingLimits) return;
    if (ui->cbLimitType->currentIndex()==0)
        enable_min_max();
    else
        enable_vector_table();

}
//------------------------------------------------
void QDlgEditLimits::enable_min_max()
{

    ui->leMax->setEnabled(true);
    ui->leMin->setEnabled(true);
    ui->lblMin->setEnabled(true);
    ui->lblMax->setEnabled(true);

    ui->tblVector->setEnabled(false);

}
void QDlgEditLimits::enable_vector_table()
{
    ui->leMax->setEnabled(false);
    ui->leMin->setEnabled(false);
    ui->lblMin->setEnabled(false);
    ui->lblMax->setEnabled(false);

    ui->tblVector->setEnabled(true);

}

//------------------------------------------------
void QDlgEditLimits::fill_table()
{
    ui->tblVector->clear();
    if (_bEditingLimits)
    {
        if (_param->get_type()!=RPT_ENUM)
        {
            ui->tblVector->setColumnCount(1);
            ui->tblVector->setHorizontalHeaderLabels(QStringList({"Values"}));

            QVector<QVariant> data = _param->availableset_to_variant();
            ui->tblVector->setRowCount(data.count());
            for (int n=0; n < data.count(); n++)
                ui->tblVector->setItem(n,0,new QTableWidgetItem(data[n].toString()));
        }
        else
        {
            // 1. Editing limits, RPT_ENUM
            ui->tblVector->setColumnCount(2);
            ui->tblVector->setHorizontalHeaderLabels(QStringList({"Value", "Equivalent Index"}));
            QVector<QVariant> data = _param->availableset_to_variant();
            ui->tblVector->setRowCount(data.count());
            for (int n=0; n < data.count(); n++)
            {
                enumElem elem = data[n].value<enumElem>();

                ui->tblVector->setItem(n,0,new QTableWidgetItem(elem.first));
                ui->tblVector->setItem(n,1,new QTableWidgetItem(QString::number(elem.second)));
            }
        }
    }
    else
    {
        ui->tblVector->setColumnCount(1);
        // use _data
        ui->tblVector->setHorizontalHeaderLabels(QStringList({"Values"}));
        ui->tblVector->setRowCount(_data.count());
        if (_param->get_type()!=RPT_ENUM)
        for (int n=0; n<_data.count();n++)
        {
            if (_param->has_available_set())
            {
                    QComboBox *cb = new QComboBox();

                    fill_combobox_with_available_set(cb);
                    ui->tblVector->setCellWidget(n,0,cb);
                    int current_index = _param->get_index_of_value(_data[n]);
                    if (current_index == -1)
                    {
                        QMessageBox::critical(this,"Error",QString("During loading of parameter, a value has been found non consistent: \n")
                                                                 +_data.at(n).toString()+QString("\n. It will be default to first available."));
                        current_index = 0;
                    }
                    cb->setCurrentIndex(current_index);
            }
            else
            {
                ui->tblVector->setItem(n,0,new QTableWidgetItem(_data[n].toString()));
            }
        }
        else
        {
            // RPT_ENUM
            for (int n=0; n<_data.count();n++)
            {
                enumElem elem = _data[n].value<enumElem>();
                QComboBox *cb = new QComboBox();

                fill_combobox_with_available_enums(cb);
                int index = get_index_of_enum(elem.first);

                if (index<0)
                {
                    QMessageBox::critical(this,"Error",QString("During loading of parameter, a value has been found non consistent: \n")
                                                             +_data.at(n).toString()+QString("\n. It will be default to first available."));
                    index = 0;

                }
                cb->setCurrentIndex(index);
                ui->tblVector->setCellWidget(n,0,cb);
            }
        }
    }

}
//------------------------------------------------
void QDlgEditLimits::fill_combobox_with_available_enums(QComboBox *cb)
{
    if (cb==nullptr)
        return;
    if (_param==nullptr)
        return;

    cb->clear();
    for (int n=0; n<_avail_dataset.count(); n++)
    {
        enumElem elem = _avail_dataset[n].value<enumElem>();
        cb->addItem(elem.first);
    }
}
//------------------------------------------------
int  QDlgEditLimits::get_index_of_enum(QString value)
{
    for (int n=0; n < _avail_dataset.count(); n++)
    {
        enumElem elem = _avail_dataset[n].value<enumElem>();
        if (elem.first==value)
            return n;
    }

    return -1;
}
//------------------------------------------------
void QDlgEditLimits::fill_combobox_with_available_set(QComboBox *cb)
{
    if (cb==nullptr)
        return;
    if (_param==nullptr)
        return;

    cb->clear();
    for (int n=0; n<_avail_dataset.count(); n++)
        cb->addItem(_avail_dataset[n].toString());
}
//------------------------------------------------
int  QDlgEditLimits::get_index_of(QString value)
{
    for (int n=0; n < _avail_dataset.count(); n++)
    {
        enumElem elem = _avail_dataset[n].value<enumElem>();
        if (elem.first==value)
            return n;
    }

    return -1;
}
//------------------------------------------------
void QDlgEditLimits::init_menu()
{
    QMenuBar *menubar = new QMenuBar(this);

    QMenu *menu1            = menubar->addMenu("Parameters");
    QAction *actionAdd      = menu1->addAction("Add Row");
    QAction *actionRemove   = menu1->addAction("Delete Row");
    connect(actionAdd,      SIGNAL(triggered()),this,SLOT(add_new_row()));
    connect(actionRemove,   SIGNAL(triggered()),this,SLOT(remove_current_row()));
}

//------------------------------------------------
// Add a blank row
void QDlgEditLimits::add_new_row()
{
    if (_bEditingLimits)
    {
        if (_param->get_type()==RPT_ENUM)
        {
            ui->tblVector->insertRow(ui->tblVector->rowCount());
            ui->tblVector->setItem(ui->tblVector->rowCount()-1,0,new QTableWidgetItem());
            ui->tblVector->setItem(ui->tblVector->rowCount()-1,1,new QTableWidgetItem());
        }
        else
        {
            ui->tblVector->insertRow(ui->tblVector->rowCount());
            ui->tblVector->setItem(ui->tblVector->rowCount()-1,0,new QTableWidgetItem());
        }
    }
    else
    {
        if (_param->get_type()==RPT_ENUM)
        {
            ui->tblVector->insertRow(ui->tblVector->rowCount());
            QComboBox *cb = new QComboBox();

            fill_combobox_with_available_enums(cb);
            ui->tblVector->setCellWidget(ui->tblVector->rowCount()-1,0, cb);
            cb->setCurrentIndex(0);
        }
        else
        {
            if (_param->has_available_set())
            {
                ui->tblVector->insertRow(ui->tblVector->rowCount());
                QComboBox *cb = new QComboBox();

                fill_combobox_with_available_set(cb);
                ui->tblVector->setCellWidget(ui->tblVector->rowCount()-1,0, cb);
                cb->setCurrentIndex(0);
            }
            else
            {
                ui->tblVector->insertRow(ui->tblVector->rowCount());
                ui->tblVector->setItem(ui->tblVector->rowCount()-1,0,new QTableWidgetItem());
            }
        }
    }
}

//------------------------------------------------
void QDlgEditLimits::remove_current_row()
{
    int row = ui->tblVector->currentRow();
    if ((row < 0)||(row>=ui->tblVector->rowCount()))
        return;

    ui->tblVector->removeRow(row);

}

//------------------------------------------------
void QDlgEditLimits::save_and_close()
{
    if (_bEditingLimits)
    {
        // Validate min max
        if (ui->cbLimitType->currentIndex()==0)
        {
            if (!validate_string(QVariant(ui->leMin->text())))
            { QMessageBox::critical(this, "Error", "Syntax error in min value"); ui->leMin->setFocus(); return;}
            if (!validate_string(QVariant(ui->leMax->text())))
            { QMessageBox::critical(this, "Error", "Syntax error in max value"); ui->leMax->setFocus(); return;}

            _param->set_min_max(QVariant(ui->leMin->text()), QVariant(ui->leMax->text()));
            done(QMessageBox::Ok);
            return;
        }


        if (_param->get_type()==RPT_ENUM)
        {
            QVector<QVariant> temp_avail;
            for (int n= 0; n < ui->tblVector->rowCount(); n++)
            {
                bool bOk;
                int  pn     = ui->tblVector->item(n,1)->text().toUInt(&bOk);
                QString ps  = ui->tblVector->item(n,0)->text();
                if (!bOk)
                {
                    QMessageBox::critical(this, "Error", "Syntax error in enum index");
                    ui->tblVector->setFocus();
                    ui->tblVector->setCurrentCell(n,0);
                    ui->tblVector->selectRow(n);
                    return;
                }

                if (ps.isEmpty())
                {
                    QMessageBox::critical(this, "Error", "Empty string vaue");
                    ui->tblVector->setFocus();
                    ui->tblVector->setCurrentCell(n,1);
                    ui->tblVector->selectRow(n);
                    return;
                }

                enumElem test(ps,pn);
                for (int k = 0; k < n; k++)
                {
                    if ((temp_avail[k].value<enumElem>().first==ps)||(temp_avail[k].value<enumElem>().second==pn))
                    {
                        QMessageBox::critical(this, "Error", QString("Enum at ")+QString::number(n)+QString(" already defined"));
                        ui->tblVector->setFocus();
                        ui->tblVector->setCurrentCell(n,1);
                        ui->tblVector->selectRow(n);
                        return;
                    }
                }
                temp_avail.append(QVariant::fromValue<enumElem>(test));
            }
            _bUpdateParam = true;
            _avail_dataset = temp_avail;
            _param->variant_to_availabeset(temp_avail);
            done(QMessageBox::Ok);
            return;
        }
        else
        {
            QVector<QVariant> temp_avail;

            for (int n= 0; n < ui->tblVector->rowCount(); n++)
                if (!validate_string(QVariant(ui->tblVector->item(n,0)->text())))
                {
                    QMessageBox::critical(this, "Error", "Syntax error in value");
                    ui->tblVector->setFocus();
                    ui->tblVector->setCurrentCell(n,0);
                    ui->tblVector->selectRow(n);
                    return;
                }
            else
                temp_avail.append(QVariant(ui->tblVector->item(n,0)->text()));

            _avail_dataset = temp_avail;
            _bUpdateParam = true;
            _param->variant_to_availabeset(temp_avail);
            done(QMessageBox::Ok);
            return;
        }
        done(QMessageBox::Ok);
        return;
    }   // bEditing Limits
    if (!_bEditingLimits)
    {
        // Editing values
        if (_param->get_type()==RPT_ENUM)
        {
            QVector<QVariant> temp_data;
            for (int n=0; n < ui->tblVector->rowCount(); n++)
            {
                enumElem pair;
                QString testValue= ((QComboBox*)ui->tblVector->cellWidget(n,0))->currentText();
                int nf = -1;
                for (int a = 0; a < _avail_dataset.count(); a++)
                {
                    pair = _avail_dataset[a].value<enumElem>();
                    if (pair.first == testValue)
                    {
                        nf = a; break;
                    }
                }
                if (nf==-1)
                {
                    QMessageBox::critical(this, "Error", "Value not found");
                    ui->tblVector->setFocus();
                    ui->tblVector->setCurrentCell(n,0);
                    ui->tblVector->selectRow(n);
                    return;
                }
                temp_data.append(QVariant::fromValue(pair));
            }

            std::static_pointer_cast<radarParameter<enumElem>>(_param)->variant_to_value(temp_data);

            _bUpdateParam = true;
            done(QMessageBox::Ok);
            return;
        }

        if (_param->has_available_set())
        {
            // We have combo boxes, so data should be safe by construction
            QVector<QVariant> temp_data;
            for (int n=0; n < ui->tblVector->rowCount(); n++)
                temp_data.append(QVariant(((QComboBox*)ui->tblVector->cellWidget(n,0))->currentText()));

            _param->variant_to_value(temp_data);

            _bUpdateParam = true;
            done(QMessageBox::Ok);
            return;

        }

        QVector<QVariant> temp_data;
        for (int n=0; n < ui->tblVector->rowCount(); n++)
        {
            QVariant value(ui->tblVector->item(n,0)->text());
            if (_param->is_valid(QVariant(value)))
                temp_data.append(value);
            else
            {
                QMessageBox::critical(this, "Error", "Wrong syntax or Value outside its limits");
                ui->tblVector->setFocus();
                ui->tblVector->setCurrentCell(n,1);
                return;
            }
        }
        _param->variant_to_value(temp_data);

        _bUpdateParam = true;
        done(QMessageBox::Ok);
        return;
    }
}
//------------------------------------------------
bool QDlgEditLimits::validate_string(QVariant value)
{
    bool bOk;
    switch(_param->get_type())
    {
    case RPT_VOID:
    {
        QString val = value.toString();
        return val.isEmpty();
    }
    case RPT_INT8:
    {
        int val = value.toInt(&bOk);
        if (!bOk) return false;
        if ((val<INT8_MIN)||(val>INT8_MAX)) return false;
        return true;
    }
    case RPT_UINT8:
    {
        int val = value.toUInt(&bOk);
        if (!bOk) return false;
        if ((val<0)||(val>UINT8_MAX)) return false;
        return true;
    }
    case RPT_INT16:
    {
        int val = value.toInt(&bOk);
        if (!bOk) return false;
        if ((val<INT16_MIN)||(val>INT16_MAX)) return false;
        return true;
    }
    case RPT_UINT16:
    {
        int val = value.toUInt(&bOk);
        if (!bOk) return false;
        if ((val<0)||(val>UINT16_MAX)) return false;
        return true;
    }
    case RPT_INT32:
    {
        int val = value.toInt(&bOk);
        if (!bOk) return false;
        if ((val<INT32_MIN)||(val>INT32_MAX)) return false;
        return true;
    }
    case RPT_UINT32:
    {
        uint val = value.toUInt(&bOk);
        if (!bOk) return false;
        if (val>UINT32_MAX) return false;
        return true;
    }
    case RPT_UNDEFINED:
        return false;
    case RPT_FLOAT:
    {
        value.toFloat(&bOk);
        if (!bOk) return false;
        return true;
    }
    case RPT_ENUM:
    {
        if (value.toString().isEmpty()) return false;
        if (!_bEditingLimits)
        {
            return _param->is_valid(value.toString());
        }
        return true;
    }
    case RPT_STRING:
    {
        if (value.toString().isEmpty()) return false;
        if (!_bEditingLimits)
        {
            return _param->is_valid(value.toString());
        }
        return true;
    }

    }
    return false;
}
//------------------------------------------------
void QDlgEditLimits::discard_and_close()
{
    _bUpdateParam = false;
    done(QMessageBox::Cancel);
}


void QDlgEditLimits::clicked(QAbstractButton* btn)
{
    if (btn==nullptr) return;
    if ((QPushButton*)btn == ui->buttonBox->button(QDialogButtonBox::Reset))
    {
        _param->remove_available_set();
        _param->remove_min_max();
        done(QMessageBox::Ok);
    }
}

