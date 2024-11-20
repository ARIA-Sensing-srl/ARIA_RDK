/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef QDLGEDITLIMITS_H
#define QDLGEDITLIMITS_H

#include <QDialog>
#include "qabstractbutton.h"
#include "radarparameter.h"
#include <QVector>
#include <QVariant>
#include <QComboBox>
namespace Ui {
class QDlgEditLimits;
}

class QDlgEditLimits : public QDialog
{
    Q_OBJECT

public:
    explicit QDlgEditLimits(QWidget *parent , radarParamPointer& param, bool bEditingLimits=false, bool bEditingCurrent=false);
    bool     update_param();
    ~QDlgEditLimits();

private:
    bool                            _bEditingLimits;
    bool                            _bEditingCurrent;
    bool                            _bUpdateParam;
    radarParamPointer               _param;
    QVector<QVariant>               _data; // Contains temporary
    QVector<QVariant>               _avail_dataset;
    Ui::QDlgEditLimits              *ui;

    void enable_min_max();
    void enable_vector_table();

    void fill_table();
    void fill_combobox_with_available_enums (QComboBox *cb);
    int  get_index_of_enum                  (QString value);
    void fill_combobox_with_available_set   (QComboBox *cb);
    int  get_index_of                       (QString value);

    void init_menu();
    bool validate_string(QVariant );



public slots:
    void limit_type_change(int index=0);
    void add_new_row();
    void remove_current_row();
    void save_and_close();
    void discard_and_close();
    void clicked(QAbstractButton* btn);
};

#endif // QDLGEDITLIMITS_H
