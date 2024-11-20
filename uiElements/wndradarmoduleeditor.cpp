/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "wndradarmoduleeditor.h"
#include "qmenu.h"
#include "ui_wndradarmoduleeditor.h"


#include <QPushButton>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>

#include <stdint.h>

#include "ariautils.h"
#include "radarmodule.h"
#include "radarparameter.h"
#include "radarinstance.h"

#include "serialport_utils.h"
#include "qdlgeditlimits.h"
#include "wnddirectionfocusing.h"
#include "wndscanmodules.h"
#include "wndradarinstanceeditor.h"

#define COL_NAME 0
#define COL_TYPE 1
#define COL_SIZE 2
#define COL_VALUE 3
#define COL_LIMITS 4
#define COL_IO 5
#define COL_GROUP 6
#define COL_COMMAND 7
#define COL_INQUIRY 8
//#define COL_LINKED_TO_WORKSPACE 9
//#define COL_MULTIRADAR 11
//#define COL_ALIAS 12

#define COL_ANTMODEL 0
#define COL_ANTNAME  1
#define COL_ANT_POSX 2
#define COL_ANT_POSY 3
#define COL_ANT_POSZ 4
#define COL_ANT_LOSS 5
#define COL_ANT_DELAY 6
#define COL_ANT_ROT_THETA 7
#define COL_ANT_ROT_PHI   8

extern QDir             ariasdk_modules_path;
extern QDir             ariasdk_projects_path;
extern QDir             ariasdk_scripts_path;
extern QDir             ariasdk_antennas_path;
extern QDir             ariasdk_antennaff_path;

extern QString cstr_radar_devices;
extern QString cstr_scripts;
extern QString cstr_handler;
extern QString cstr_radar_modules;
extern QString cstr_antenna_models;
extern QString cstr_comm_protocol;


//---------------------------------------------------------
// Param table policy:
// If a user move to a new row,
//      1- if the row is a new param, don't validate (need "add button" to be clicked)
//      2- Otherwise validate
// Flow is as follows:
//      When moving from a row to a new one:
//          1. if "old" row is "present param" validate. If ok,
//              move internal data to parameter
//          2. move new row parameter data to internal

//---------------------------------------------------------
wndRadarModuleEditor::wndRadarModuleEditor(radarModule* module, radarProject *project,  QWidget *parent) :
    QDialog(parent),
    _project(project),
    ui(new Ui::wndRadarModuleEditor),
    _radar_module(new radarModule()),
    _source(module),
    _ptable(),
    _antenna_table(),
    _available_antennas(),
    _uploading_antenna_cb(false)
{
    if (_project == nullptr)
    {
        QMessageBox::critical(this, "Error", "A project must be defined");

        this->done(-1);
        this->parentWidget()->close();
        return;
    }
    ui->setupUi(this);

    if (module !=nullptr)
    {
        _bNewModule = false;
        *_radar_module = *module;
        this->setWindowTitle(_radar_module->get_name());
        copy_radar_array_to_temp();
    }
    else
    {
        _radar_module->set_temporary_project(project);
        _bNewModule = true;
    }

    copy_radar_params_to_temp();
    init_param_table();

    init_txrx_pairs_table();

    init_tables();

    init_serial_port_combos();

    connect(ui->btnAddParam, SIGNAL(clicked()), this, SLOT(add_parameter()));
    connect(ui->btnDeleteParam, SIGNAL(clicked()), this, SLOT(remove_parameter()));

    ui->btnSaveParams->setVisible(false);
    ui->btnRevertParams->setVisible(false);

//    connect(ui->btnSaveParams, SIGNAL(clicked()), this, SLOT(save_parameters()));
//    connect(ui->btnRevertParams,SIGNAL(clicked()), this, SLOT(undo_parameters()));
    connect(ui->btnCopyParam,SIGNAL(clicked()), this, SLOT(copy_parameter()));
    connect(ui->tblParamsEditor,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(current_cell_change(int,int,int,int)));

    // Antenna array designer
    copy_radar_array_to_temp();
    init_antenna_table();
    connect(ui->btnAddAntenna, SIGNAL(clicked()), this, SLOT(add_antenna()));
    connect(ui->btnDirectionFocusing,SIGNAL(clicked()),this,SLOT(start_direction_focusing()));
    connect(ui->btnPointFocusing,SIGNAL(clicked()), this, SLOT(start_point_focusing()));
    connect(ui->tblArrayEditor,SIGNAL(cellChanged(int,int)), this, SLOT(update_antenna_data(int,int)));

    ui->btnConfirmArray->setVisible(false);
    ui->btnRevertArray->setVisible(false);
//    connect(ui->btnConfirmArray, SIGNAL(clicked()), this, SLOT(save_antennas()));
//    connect(ui->btnRevertArray, SIGNAL(clicked()), this, SLOT(revert_antennas()));
    connect(ui->btnRemoveAntenna, SIGNAL(clicked()),this, SLOT(remove_antenna()));

    // Radar module save / load
    connect(ui->btnSaveRadarModule, SIGNAL(clicked()), this, SLOT(save_radar_module()));
    connect(ui->btnSaveAsRadarModule, SIGNAL(clicked()), this, SLOT(save_radar_module_as()));
    connect(ui->btnLoadRadarModule, SIGNAL (clicked()), this, SLOT(load_radar_module()));
    // array configuration
    connect(ui->btnAddTxRxPair, SIGNAL(clicked()), this, SLOT(add_txrx_pair()));
    connect(ui->btnRemoveTxRxPair, SIGNAL(clicked()), this, SLOT(remove_txrx_pair()));


    connect(ui->tblParamsInit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(initParams_RightClick(QPoint)));
    connect(ui->tblParamsAcquisition, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(acqParams_RightClick(QPoint)));
    connect(ui->tblParamsPostAcquisition, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(postacqParams_RightClick(QPoint)));
    connect(ui->tblScriptsInit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(initScripts_RightClick(QPoint)));
    connect(ui->tblScriptsAcquisition, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(acqScripts_RightClick(QPoint)));
    connect(ui->tblScriptsPostAcq, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(postacqScripts_RightClick(QPoint)));

    connect(ui->btnAddInitParam, SIGNAL(clicked()), this, SLOT(add_initParam()));
    connect(ui->btnAddAcquisitionParam, SIGNAL(clicked()), this, SLOT(add_acquisitionParam()));
    connect(ui->btnAddPostAcqParam, SIGNAL(clicked()), this, SLOT(add_postAcquisitionParam()));
    connect(ui->btnRemoveInitParam, SIGNAL(clicked()), this, SLOT(remove_initParam()));
    connect(ui->btnRemoveAcquisitionParam, SIGNAL(clicked()), this, SLOT(remove_acquisitionParam()));
    connect(ui->btnRemovePostAcqParam, SIGNAL(clicked()), this, SLOT(remove_postAcquisitionParam()));

    connect(ui->btnAddInitScript, SIGNAL(clicked()), this, SLOT(add_initScript()));
    connect(ui->btnAddAcquisitionScript, SIGNAL(clicked()), this, SLOT(add_acquisitionScript()));
    connect(ui->btnAddPostAcquisitionScript, SIGNAL(clicked()), this, SLOT(add_postAcquisitionScript()));
    connect(ui->btnRemoveInitScript, SIGNAL(clicked()), this, SLOT(remove_initScript()));
    connect(ui->btnRemoveAcquisitionScript, SIGNAL(clicked()), this, SLOT(remove_acquisitionScript()));
    connect(ui->btnRemovePostAcquisitionScript, SIGNAL(clicked()), this, SLOT(remove_postAcquisitionScript()));

    connect(ui->btnScanRadarDevices,SIGNAL(clicked()), this, SLOT(scanRadarDevices()));
    connect(ui->btnCreateDevice,SIGNAL(clicked()), this, SLOT(createDevice()));
}
//---------------------------------------------------------
wndRadarModuleEditor::~wndRadarModuleEditor()
{
    for (int n=0; n < _ptable.count(); n++)
        _ptable[n].reset();
    _ptable.clear();
    for (int n=0; n < _antenna_table.count(); n++)
        if (_antenna_table[n]!=nullptr)
            delete _antenna_table[n];
    _antenna_table.clear();
    delete ui;
}

//---------------------------------------------------------
void wndRadarModuleEditor::init_param_table()
{
    //ui->->clear();
    ui->tblParamsEditor->clear();
    ui->tblParamsEditor->setRowCount(0);
    ui->tblParamsEditor->setColumnCount(9);

    QStringList headers;
    headers << "Param name" << "Base Type" << "Dimension" << "Default Value" << "Valid values" << "I/O" << "Group" << "Command String (hex)" << "Inquiry Value (hex)";
            //<< "Linked to Workspace" << "Compound name" << "Alias";

    ui->tblParamsEditor->setHorizontalHeaderLabels(headers);

    for (int np = 0; np < _ptable.count(); np++)
        set_param_row(_ptable.at(np),np);

    update_cb_inquiry();
}
//---------------------------------------------------------
void wndRadarModuleEditor::set_param_row(radarParamPointer param, int rownumber)
{
    if (rownumber>=ui->tblParamsEditor->rowCount())
    {
        ui->tblParamsEditor->insertRow(ui->tblParamsEditor->rowCount());
        rownumber = ui->tblParamsEditor->rowCount()-1;
    }

    QTableWidgetItem *item = new QTableWidgetItem();
    item->setText(param->get_name());

    item->setFlags(item->flags() | Qt::ItemIsEditable);    
    ui->tblParamsEditor->setItem(rownumber, COL_NAME, item);

    QComboBox *cbType = new QComboBox();
    cbType->setObjectName("cbType"+QString::number(rownumber));
    QStringList items;
    for (int rpt=0; rpt < 11; rpt++)
        if (rpt!=RPT_UNDEFINED)
            items << typeIdToString((RADARPARAMTYPE)rpt);

    cbType->addItems(items);

    cbType->setCurrentIndex(param->get_type()-1);
    connect(cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(cbType_index_changed(int)));

    ui->tblParamsEditor->setCellWidget(rownumber,COL_TYPE ,cbType);

    QComboBox *cbSize = new QComboBox();
    cbSize->setObjectName("cbSize"+QString::number(rownumber));
    cbSize->addItems(QStringList({"scalar","nd-array"}));
    cbSize->setCurrentIndex(param->get_size()==RPT_SCALAR? 0 : 1);
    connect(cbSize, SIGNAL(currentIndexChanged(int)), this, SLOT(cbSize_index_changed(int)));
    ui->tblParamsEditor->setCellWidget(rownumber,COL_SIZE,cbSize);

    update_default_widget(param, rownumber);
    cbSize->setEnabled(param->get_type()!=RPT_VOID);

    QPushButton* btn_edit_limits = new QPushButton();
    btn_edit_limits->setText("Edit");
    btn_edit_limits->setIcon(QIcon(":/icons/calendar-plus-line-icon.png"));
    connect(btn_edit_limits,SIGNAL(clicked()),this,SLOT(btn_edit_limits_clicked()));
    ui->tblParamsEditor->setCellWidget(rownumber, COL_LIMITS, btn_edit_limits);
    btn_edit_limits->setEnabled(param->get_type()!=RPT_VOID);

    QComboBox *cbIO = new QComboBox();
    cbIO->setObjectName("cbIO"+QString::number(rownumber));
    cbIO->addItems(QStringList({"Input","Output","I/O"}));
    cbIO->setCurrentIndex((int)(param->get_io_type()));
    connect(cbIO, SIGNAL(currentIndexChanged(int)), this, SLOT(cbIO_index_changed(int)));
    ui->tblParamsEditor->setCellWidget(rownumber,COL_IO,cbIO);

    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(param->get_group());
    ui->tblParamsEditor->setItem(rownumber, COL_GROUP, item);

    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(param->get_command_string());
    ui->tblParamsEditor->setItem(rownumber, COL_COMMAND, item);

    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    QString inq_text = "";
    if (param->has_inquiry_value())
    {
        QVariant var = param->inquiry_value_to_variant();
        if (param->get_type()==RPT_ENUM)
            inq_text = var.value<enumElem>().first;
        else
            inq_text = var.toString();
    }
    item->setText(inq_text);
    ui->tblParamsEditor->setItem(rownumber, COL_INQUIRY, item);

    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);


//....> to here
    set_param_row_void(param, rownumber);
    update_cb_inquiry();
}

void wndRadarModuleEditor::set_param_row_void(radarParamPointer param,int row)
{
    if ((row < 0)||(row>=ui->tblParamsEditor->rowCount()))
        return;

    // COL_SIZE
    QWidget* widget = ui->tblParamsEditor->cellWidget(row, COL_SIZE);
    if (widget!=nullptr)
        widget->setVisible(param->get_type()!=RPT_VOID);

    // COL_VALUE
    widget = ui->tblParamsEditor->cellWidget(row, COL_VALUE);
    if (widget!=nullptr)
        widget->setVisible(param->get_type()!=RPT_VOID);

    if (get_size(row)==RPT_SCALAR)
    {
        QTableWidgetItem *item = ui->tblParamsEditor->item(row,COL_VALUE);
        if (item!=nullptr)
        {
            bool disabled = param->get_type() == RPT_VOID || (get_rpt_io(row)==RPT_IO_OUTPUT);

            item->setFlags(disabled ? item->flags() & ~Qt::ItemIsEditable : item->flags() | Qt::ItemIsEditable);
            if (param->get_type() == RPT_VOID)
                item->setText("");
        }
    }
    // COL_VALUE
    widget = ui->tblParamsEditor->cellWidget(row, COL_LIMITS);
    if (widget!=nullptr)
        widget->setVisible(param->get_type()!=RPT_VOID);

    QTableWidgetItem* item = ui->tblParamsEditor->item(row,COL_INQUIRY);
    if (item!=nullptr)
    {
        item->setFlags(param->get_type() == RPT_VOID ? item->flags() & ~Qt::ItemIsEditable : item->flags() | Qt::ItemIsEditable);
        if (param->get_type() == RPT_VOID)
            item->setText("");
    }

}

//---------------------------------------------------------
void wndRadarModuleEditor::update_default_widget(radarParamPointer param, int row,bool scalar,bool force_scalar)
{
    // Delete previous stuff, if any
    QWidget* currItem = ui->tblParamsEditor->cellWidget(row,COL_VALUE);
    if (currItem!=nullptr)
    {
        ui->tblParamsEditor->removeCellWidget(row,COL_VALUE);
        delete currItem;
    }

    bool scalar_val = (scalar & force_scalar) | ((param->get_size()==RPT_SCALAR) & (!force_scalar));
    if (scalar_val)
    {
        if (param->get_type()!=RPT_ENUM)
        {
            if (param->has_available_set())
            {
                QComboBox *cbAvailable = new QComboBox();
                cbAvailable->setObjectName("cbAvailable"+QString::number(row));
                fill_cb_with_available(cbAvailable, param);
                ui->tblParamsEditor->setCellWidget(row,COL_VALUE,cbAvailable);
            }
            else
            {
                if ((param->get_type()==RPT_UINT8)||((param->get_type()==RPT_INT8)))
                {
                    QTableWidgetItem *item = new QTableWidgetItem();
                    item->setFlags(item->flags() | Qt::ItemIsEditable);

                    if (param->get_type()==RPT_UINT8)
                    {
                        uint8NDArray data = param->get_value().uint8_array_value();
                        if (data.numel()>0)
                        {
                            item->setText(QString::number((double)(data(0))));
                        }
                        else
                            item->setText("");
                    }
                    else
                    {
                        int8NDArray data = param->get_value().int8_array_value();
                        if (data.numel()>0)
                        {
                            item->setText(QString::number((double)(data(0))));
                        }
                        else
                            item->setText("");
                    }
                    ui->tblParamsEditor->setItem(row, COL_VALUE, item);
                }
                else
                {
                    QTableWidgetItem *item = new QTableWidgetItem();
                    item->setFlags(item->flags() | Qt::ItemIsEditable);
                    QVector<QVariant> dataDefault = param->value_to_variant();
                    if (dataDefault.count()>0)
                        item->setText(dataDefault[0].toString());
                    else
                        item->setText("");
                    ui->tblParamsEditor->setItem(row, COL_VALUE, item);
                }

                bool enabled = param->get_io_type() != RPT_IO_OUTPUT;
                QTableWidgetItem* item = ui->tblParamsEditor->item(row, COL_VALUE);

                if (item!=nullptr)
                {
                    if (enabled)
                        item->setFlags(item->flags() | Qt::ItemIsEditable);
                    else
                        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                }
            }
        }
        else
        {
            QComboBox *cbEnum = new QComboBox();
            cbEnum->setObjectName("cbEnum"+QString::number(row));
            fill_cb_with_enums(cbEnum, param);
            ui->tblParamsEditor->setCellWidget(row,COL_VALUE,cbEnum);
        }

    }
    else
    {
        QPushButton* btn_edit_default = new QPushButton();
        btn_edit_default->setText("Edit");
        btn_edit_default->setIcon(QIcon(":/icons/calendar-plus-line-icon.png"));
        connect(btn_edit_default,SIGNAL(clicked()),this,SLOT(btn_edit_default_clicked()));
        ui->tblParamsEditor->setCellWidget(row, COL_VALUE, btn_edit_default);

    }
    set_param_row_void(param,row);
}
//---------------------------------------------------------
void wndRadarModuleEditor::fill_cb_with_enums(QComboBox* cb,radarParamPointer rp)
{
    if (cb==nullptr)
        return;

    if (rp->get_type()!=RPT_ENUM)
        return;


    std::shared_ptr<radarParameter<enumElem>> rpenum = std::dynamic_pointer_cast<radarParameter<enumElem>>(rp);
    if (rpenum==nullptr) return;

    int currIndex = -1;

    QVector<QVariant> def = rp->value_to_variant();
    enumElem rpdef;
    if (def.count() == 1)
        rpdef = def[0].value<enumElem>();


    radarEnum available = rpenum->get_list_available_values();

    for (int n=0; n < available.count(); n++)
    {
        enumElem pair = available[n];
        cb->addItem(pair.first);

        if ((pair==rpdef)&&(def.count()==1))
            currIndex=n;
    }

    if (currIndex>=0)
        cb->setCurrentIndex(currIndex);
}
//---------------------------------------------------------
void wndRadarModuleEditor::fill_cb_with_available(QComboBox* cb,radarParamPointer rp)
{
    if (cb==nullptr)
        return;

    if (!rp->has_available_set())
        return;
    int currIndex = -1;
    QVector<QVariant> def = rp->value_to_variant();
    QString rpdef = "";
    if (def.count() == 1)
       rpdef = def[0].toString();

    QVector<QVariant> avail = rp->availableset_to_variant();

    for (int n=0; n < avail.count(); n++)
    {
       QString avstr = avail[n].toString();
       cb->addItem(avstr);

       if ((def.count()==1)&&(avstr == rpdef))
            currIndex = n;
    }
    if (currIndex >=0)
        cb->setCurrentIndex(currIndex);
}
//---------------------------------------------------------
void wndRadarModuleEditor::btn_edit_default_clicked()
{
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParamsEditor->indexAt(w->pos()).row();

    if (row<0) return;

    radarParamPointer pbase =  _ptable.at(row);

    if (pbase==nullptr)
    {
        QMessageBox::critical(this,"Error", "void parameter!");
        return;
    }

    edit_parameter_default(pbase);
}
//---------------------------------------------------------
void wndRadarModuleEditor::btn_edit_limits_clicked()
{
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParamsEditor->indexAt(w->pos()).row();

    if (row<0) return;

    radarParamPointer pbase =  _ptable.at(row);

    if (pbase==nullptr)
    {
        QMessageBox::critical(this,"Error", "void parameter!");
        return;
    }

    if (edit_parameter_limits(pbase))
    {
        QComboBox *cb = (QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_SIZE));
        bool scalar   = cb->currentIndex()==0;

        update_default_widget(pbase,row,scalar,true);
    }
}
//---------------------------------------------------------
void wndRadarModuleEditor::cbType_index_changed(int index)
{
    // Recast current parameter
    // Warning : changing the parameter class will result in loss of previous data
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParamsEditor->indexAt(w->pos()).row();

    if (row<0) return;

    RADARPARAMTYPE rt = get_rpt(row);
    QString     name  = get_name(row);

    // Get intended size from QCb
    radarParamPointer xp = recreate_param(_ptable.at(row), name, rt);
    xp->set_size(RPT_SCALAR);

    if (xp==nullptr)
    {
        QMessageBox::critical(this, "Error", "Error while setting ENUM type");
        return;
    }
    _ptable[row] = xp;
    QComboBox* cb = (QComboBox*)ui->tblParamsEditor->cellWidget(row, COL_SIZE);
    //bool scalar = cb->currentIndex()==1;
    cb->setCurrentIndex(0);

    update_default_widget(xp,row, true, true);
    set_param_row_void(xp,row);

}
//---------------------------------------------------------
void wndRadarModuleEditor::cbIO_index_changed(int index)
{
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParamsEditor->indexAt(w->pos()).row();
    if (row<0) return;

    _ptable[row]->set_io_type((RADARPARAMIOTYPE)((QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_IO)))->currentIndex());

    QComboBox *cb = (QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_SIZE));
    bool scalar   = cb->currentIndex()==0;


    update_default_widget(_ptable[row],row,scalar,true);


}
//---------------------------------------------------------
void wndRadarModuleEditor::cbSize_index_changed(int index)
{
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParamsEditor->indexAt(w->pos()).row();

    if (row<0) return;

    update_default_widget(_ptable[row],row,index==0,true);
}

//---------------------------------------------------------
int wndRadarModuleEditor::validate_param(radarParamPointer &dest, int row)
{
    if (dest==nullptr)
    {
        QMessageBox::critical(this,"Error",QString(" Parameter void! "));
        return COL_NAME;
    }

    // If we are adding a new parameter, the name must be non-existent
    QString _name = ui->tblParamsEditor->item(row,COL_NAME)->text();
    if (_name.isEmpty())
    {
        QMessageBox::critical(this,"Error",QString(" Param name is empty."));
        return COL_NAME;
    }

    // Search for previously defined parameters
    int param_id = -1;
    for (int np=0; np < _ptable.count(); np++)
        if (_ptable.at(np)!=nullptr)
            if (_ptable.at(np)->get_name() == _name)
            { param_id = np; break;}

    if ((param_id!=-1)&&(param_id!=row))
    {
        QMessageBox::critical(this,"Error",_name + QString(" is already defined."));
        return COL_NAME;
    }

    // Check if we have to update the parameters tables
    if (_name != _ptable[row]->get_name())
    {

    }

    RADARPARAMTYPE rpt          = get_rpt(row);
    RADARPARAMIOTYPE rpt_io     = get_rpt_io(row);
    RADARPARAMSIZE   rpt_size   = get_size(row);
    //dest = recreate_param(dest, _name, rpt);
    dest->set_name(_name);
    dest->set_io_type(rpt_io);
    dest->set_size(rpt_size);

    dest->set_group( ui->tblParamsEditor->item(row,COL_GROUP)->text());

    if (rpt!=RPT_VOID)
    {

        QComboBox* cb = (QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_SIZE));
        bool is_scalar = cb->currentIndex()==0;
        dest->set_size(is_scalar?RPT_SCALAR:RPT_NDARRAY);
        bool bOk = false;

        QString inquiry_string = ui->tblParamsEditor->item(row,COL_INQUIRY)->text();
        if (inquiry_string.isEmpty())
            dest->set_inquiry_value();
        else
        {

            if (dest->is_valid(QVariant(inquiry_string)))
                dest->variant_to_inquiry_value(QVariant(inquiry_string));
            else
                dest->set_inquiry_value();
        }


        if (is_scalar)
        {
            // Validate the value as a column string

            if (dest->has_available_set())
            {
                if (rpt==RPT_ENUM)
                {
                    QComboBox *cb = (QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_VALUE));
                    int index = cb->currentIndex();

                    enumElem val = dest->availableset_to_variant()[index].value<enumElem>();
                    (std::dynamic_pointer_cast<radarParameter<enumElem>>(dest))->value(val);
                    bOk = true;
                }
                else
                {
                    QComboBox *cb = (QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_VALUE));
                    int index = cb->currentIndex();

                    QVector<QVariant> val({dest->availableset_to_variant()[index]});
                    dest->variant_to_value(val);
                    bOk = true;
                }
            }
            else
            {
                QVariant value = ui->tblParamsEditor->item(row,COL_VALUE)->text();
                bool bEmptyVal = false;
                if (value.toString().isEmpty())
                {
                    bOk = true;
                    bEmptyVal = true;
                }
                else
                {
                    if (rpt == RPT_VOID)
                    {
                        bEmptyVal = true;
                        bOk = true;
                    }
                    if (rpt == RPT_INT8)
                    {
                        int val = value.toInt(&bOk);
                        if (bOk)
                        {
                            if ((val<INT8_MIN)||(val>INT8_MAX))
                                bOk = false;
                        }
                    }
                    if (rpt == RPT_UINT8)
                    {
                        int val = value.toUInt(&bOk);
                        if (bOk)
                        {
                            if ((val<0)||(val>UINT8_MAX))
                                bOk = false;
                        }
                    }
                    if (rpt == RPT_INT16)
                    {
                        int val = value.toInt(&bOk);
                        if (bOk)
                        {
                            if ((val<INT16_MIN)||(val>INT16_MAX))
                                bOk = false;
                        }
                    }
                    if (rpt == RPT_UINT16)
                    {
                        int val = value.toUInt(&bOk);
                        if (bOk)
                        {
                            if ((val<0)||(val>UINT16_MAX))
                                bOk = false;
                        }
                    }
                    if (rpt == RPT_INT32)
                    {
                        int val = value.toInt(&bOk);
                        if (bOk)
                        {
                            if ((val<INT32_MIN)||(val>INT32_MAX))
                                bOk = false;
                        }
                    }
                    if (rpt == RPT_UINT32)
                    {
                        uint val = value.toUInt(&bOk);
                        if (bOk)
                        {
                            if ((val<0)||(val>UINT32_MAX))
                                bOk = false;
                        }
                    }
                    if (rpt == RPT_FLOAT)
                    {
                        value.toFloat(&bOk);
                    }
                    if (rpt == RPT_STRING)
                    {
                        bOk = true;
                    }
                }
                if (!bOk)
                {
                    QMessageBox::critical(this,"Error",QString("Error in default value parameter: ")+dest->get_name());
                    return COL_VALUE;
                }
                else
                    dest->variant_to_value(bEmptyVal ? QVector<QVariant>() : QVector<QVariant>({value}));
           }
        }
        else
           bOk = true;

        if (bOk) bOk =  (dest->is_last_assignment_valid());
        if (!bOk)
        {
            QMessageBox::critical(this,"Error","Default value parameter not valid for given limits/available set");
            return COL_VALUE;
        }
    }
    else // rpt != RPT_VOID
        dest->set_io_type(RPT_IO_INPUT);


    QString command_string = ui->tblParamsEditor->item(row,COL_COMMAND)->text();

    if (command_string.isEmpty())
    {
        QMessageBox::critical(this,"Error","Empty parameter string");
        return COL_COMMAND;
    }

    param_id = -1;
    for (int np=0; np < _ptable.count(); np++)
        if (_ptable.at(np)!=nullptr)
            if (_ptable.at(np)->get_command_string() == command_string)
            { param_id = np; break;}

    if ((param_id!=-1)&&(param_id!=row))
    {
        QMessageBox::critical(this,"Error","Command string already defined");
        return COL_COMMAND;
    }

    dest->set_command_string(command_string);


    return -1;
}
//---------------------------------------------------------
void wndRadarModuleEditor::add_param_clicked()
{
    int currentRow = ui->tblParamsEditor->currentRow();
    radarParamPointer param = std::make_shared<radarParameter<float>>();
    int errorCol = validate_param(param,currentRow);
    if (errorCol>=0)
    {
        ui->tblParamsEditor->setCurrentCell(currentRow, errorCol);
        param.reset();
        return;
    }
    // Update prev row
    set_param_row(param, currentRow);
    //_radar_module->append_param(temp_param_table.at(currentRow));

    _ptable.last()=param;

    set_param_row(_ptable.last(),_ptable.count());
}

//---------------------------------------------------------
void wndRadarModuleEditor::param_selection_changed()
{
}

//---------------------------------------------------------
bool wndRadarModuleEditor::edit_parameter_limits(radarParamPointer& param)
{
    QDlgEditLimits editLimits(this, param,true,false);
    int result = editLimits.exec();
    if (result==QMessageBox::Ok)
    {
        if (param->availableset_to_variant().count()==0)
            param->remove_available_set();
        return true;
    }

    if (result == QMessageBox::Reset)
    {
        param->remove_available_set();
        param->remove_min_max();
    }
    return false;
}

//---------------------------------------------------------
void wndRadarModuleEditor::edit_parameter_default(radarParamPointer& param)
{
    if (param==nullptr) return;

    QDlgEditLimits editDefault(this,param,false,false);
    if (editDefault.exec()==QMessageBox::Ok)
    {

    }
}

//---------------------------------------------------------
RADARPARAMTYPE wndRadarModuleEditor::get_rpt(int row)
{
    QComboBox* cb = (QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_TYPE));
    if (cb==nullptr) return RPT_UNDEFINED;
    int index = cb->currentIndex();
    return (RADARPARAMTYPE)(index+1);
}

RADARPARAMSIZE wndRadarModuleEditor::get_size(int row)
{
    QComboBox* cb = (QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_SIZE));
    if (cb==nullptr) return RPT_SCALAR;
    int index = cb->currentIndex();
    return index == 0? RPT_SCALAR : RPT_NDARRAY;

}

//---------------------------------------------------------
RADARPARAMIOTYPE wndRadarModuleEditor::get_rpt_io(int row)
{
    QComboBox* cb = (QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_IO));
    if (cb==nullptr) return RPT_IO_INPUT;
    int index = cb->currentIndex();
    return (RADARPARAMIOTYPE)(index);
}

//---------------------------------------------------------

QString        wndRadarModuleEditor::get_name(int row)
{
    return (ui->tblParamsEditor->item(row,COL_NAME)->text());
}



//---------------------------------------------------------
void wndRadarModuleEditor::current_cell_change(int row, int col, int oldrow, int oldcol)
{
    if (row==oldrow)
        return;
    // We are changing the row so we need to validate the previous row
    // Get current parameter
    if ((oldrow>=0)&&(oldrow<_ptable.count()))
    {
        if (validate_param(_ptable[oldrow],oldrow)>=0)
           return;
    }
}

//---------------------------------------
radarParamPointer wndRadarModuleEditor::recreate_param(radarParamPointer pbase, QString name, RADARPARAMTYPE rt)
{
    radarParamPointer out;
    QVector<QVariant> val(pbase->value_to_variant());
    QVector<QVariant> avl(pbase->availableset_to_variant());

    if (rt==RPT_UNDEFINED) return nullptr;
    if (rt==RPT_VOID)
    {
        if (pbase!=nullptr) pbase.reset();
        out = std::make_shared<radarParameter<void>>(name);
    }

    if (rt==RPT_INT8)
    {
        if (pbase!=nullptr) pbase.reset();
        out = std::make_shared<radarParameter<int8_t>>(name);
    }

    if (rt==RPT_UINT8)
    {
        if (pbase!=nullptr) pbase.reset();
        out = std::make_shared<radarParameter<uint8_t>>(name);
    }

    if (rt==RPT_INT16)
    {
        if (pbase!=nullptr) pbase.reset();
        out = std::make_shared<radarParameter<int16_t>>(name);
    }

    if (rt==RPT_UINT16)
    {
        if (pbase!=nullptr) pbase.reset();
        out =  std::make_shared<radarParameter<uint16_t>>(name);
    }

    if (rt==RPT_INT32)
    {
        if (pbase!=nullptr) pbase.reset();
        out =  std::make_shared<radarParameter<int32_t>>(name);
    }

    if (rt==RPT_UINT32)
    {
        if (pbase!=nullptr) pbase.reset();
        out =  std::make_shared<radarParameter<uint32_t>>(name);
    }

    if (rt==RPT_FLOAT)
    {
        if (pbase!=nullptr) pbase.reset();
        out = std::make_shared<radarParameter<float>>(name);
    }

    if (rt==RPT_STRING)
    {
        if (pbase!=nullptr) pbase.reset();
        out =  std::make_shared<radarParameter<QString>>(name);
    }

    if (rt==RPT_ENUM)
    {
        if (pbase!=nullptr) pbase.reset();
        out = std::make_shared<radarParameter<enumElem>>(name);
    }

    if (out==nullptr) return out;

    out->variant_to_availabeset(avl);
    out->variant_to_value(val);
    return out;
}

//---------------------------------------
void wndRadarModuleEditor::update_row_param(int row)
{
    if ((row < 0)||(row>_ptable.count())) return;
    // Transfer data & re-create if needed
    radarParamPointer stored_param = _ptable.at(row);
    RADARPARAMTYPE rpt = get_rpt(row);
    if (get_rpt(row)!=stored_param->get_type())
        stored_param = recreate_param(stored_param,
                                      ui->tblParamsEditor->item(row,COL_NAME)->text(),
                                      rpt);
        else
        stored_param->set_name(ui->tblParamsEditor->item(row,COL_NAME)->text());

    int size_selector = ((QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_SIZE)))->currentIndex();
    if (size_selector == 0) // Scalar
    {
        if (rpt == RPT_ENUM)
        {
           QString val_selector = ((QComboBox*)(ui->tblParamsEditor->cellWidget(row,COL_VALUE)))->currentText();
           std::dynamic_pointer_cast<radarParameter<enumElem>>(stored_param)->value(val_selector);
        }
        else
        {
           QVariant variant_default(ui->tblParamsEditor->item(row,COL_VALUE)->text());
           stored_param->variant_to_value(QVector<QVariant>({variant_default}));
        }
    }

    _ptable[row] = stored_param;
}

//---------------------------------------
void wndRadarModuleEditor::add_parameter()
{
    radarParamPointer newParam = std::make_shared<radarParameter<float>>("");
    _ptable.append(newParam);
    ui->tblParamsEditor->insertRow(ui->tblParamsEditor->rowCount());
    set_param_row(_ptable.at(_ptable.count()-1),_ptable.count()-1);
}

//---------------------------------------
void wndRadarModuleEditor::remove_parameter()
{
    int row = ui->tblParamsEditor->currentRow();
    if ((row < 0)||(row>=_ptable.count())) return;

    radarParamPointer p = _ptable.at(row);
    QString question = QString("Confirm deleting parameter ")+p->get_name();

    if (QMessageBox::question(this, "Confirm action",question)==QMessageBox::No)
        return;

    if (p!=nullptr) p.reset();
    _ptable.removeAt(row);
    ui->tblParamsEditor->removeRow(row);
}

//---------------------------------------
void wndRadarModuleEditor::save_parameters()
{
    if (_radar_module==nullptr) return;
    // Clear

    // Validate each row and update radar_module
    for (int n=0; n < ui->tblParamsEditor->rowCount(); n++)
    {
        update_row_param(n);
        if (validate_param(_ptable[n],n)>=0) return;
    }

    copy_temp_params_to_radar();

    _radar_module->set_inquiry_moduleid_command_as_string(ui->cbModuleInquiry->currentText());
    _radar_module->set_inquiry_moduleid_expectedvalue_as_string(ui->leInquiryCommand->text());
    _radar_module->set_inquiry_instanceid_command_as_string(ui->cbRadarId->currentText());


    QString inquiry_string = ui->cbModuleInquiry->currentText();
    QString instance_inquiry = ui->cbRadarId->currentText();

    const QVector<radarParamPointer>& ptable = _ptable;

    bool bfound_moduleid = false;
    bool bfound_instanceid = false;

    for (const auto& param : ptable)
    {
        QString pdata = QString(param->get_command_group().toHex(0));
        if (pdata==inquiry_string)
        {
           _radar_module->set_inquiry_moduleid_command_as_string(inquiry_string);
           bfound_moduleid = true;
        }

        if (pdata==instance_inquiry)
        {
           _radar_module->set_inquiry_instanceid_command_as_string(instance_inquiry);
           bfound_instanceid = true;
        }

        if ((bfound_instanceid)&&(bfound_moduleid))
           break;
    }

    if (!bfound_moduleid)
        _radar_module->set_inquiry_moduleid_command_as_string("");
    if (!bfound_instanceid)
        _radar_module->set_inquiry_instanceid_command_as_string("");

    update_cb_inquiry();

    // Save Serial Port data

    QString serial_number = ui->leSerialNumber->text();
    QString serial_manufacturer = ui->leSerialManufacturer->text();
    QString serial_productid = ui->leProductId->text();
    QString serial_vendorid = ui->leVendorId->text();
    _radar_module->set_expected_serial_port_data(ui->leSerialDescription->text(),
                                                 serial_manufacturer,
                                                 serial_number,
                                                 serial_vendorid,
                                                 serial_productid
                                                 );

}
//---------------------------------------
void wndRadarModuleEditor::undo_parameters()
{
    if (_radar_module==nullptr) return;

    clear_ptable();

    copy_radar_params_to_temp();

    init_param_table();
}
//---------------------------------------
void wndRadarModuleEditor::copy_parameter()
{
    int rowStart = ui->tblParamsEditor->currentRow();
    if (rowStart < 0) return;

    update_row_param(rowStart);
    radarParamPointer px = _ptable[rowStart]->clone();
    if (!validate_param(px, rowStart)) return;

    QString baseName = px->get_name();
    if (baseName.isEmpty())
        baseName = "newparam";
    int nx = 0;
    while (1)
    {
        QString newName=baseName+QString::number(nx);
        bool found = false;
        for (int n=0; n < _ptable.count(); n++)
           if (_ptable[n]->get_name() == newName)
           { found = true; break;}

        if (!found)
        {
           baseName = newName;
           break;
        }
        nx++;
    }

    px->set_name(baseName);
    _ptable.append(px);

    set_param_row(_ptable.at(_ptable.count()-1),_ptable.count());
}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Array editor
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void wndRadarModuleEditor::init_antenna_table()
{
    _bAntennaNeedValid = false;
    ui->tblArrayEditor->clear();
    ui->tblArrayEditor->setRowCount(0);
    // copy every antenna into table
    ui->tblArrayEditor->setColumnCount(9);
    ui->tblArrayEditor->setHorizontalHeaderLabels(QStringList({"Model","Name","Pos_x","Pos_y","Pos_z","conn loss (dB)","conn delay (s)","Rot Phi (°)","Rot Theta (°)"}));

    for (int n=0; n < _antenna_table.count(); n++)
    {
        ui->tblArrayEditor->insertRow(ui->tblArrayEditor->rowCount());
        set_table_antennarow(_antenna_table[n],n);
    }
}
//---------------------------------------
void wndRadarModuleEditor::antenna_name_changed(QWidget *item)
{
    int row = ui->tblArrayEditor->currentRow();
//    int column = ui->tblArrayEditor->currentColumn();

    if (!validate_antenna_row(row))
    {
        QMessageBox::critical(this, "Error", QString("Error detected at antenna : ")+QString::number(row+1));
        return;
    }

    antenna_table_row_to_antenna(_antenna_table[row],row);

}

//---------------------------------------
void wndRadarModuleEditor::set_table_antennarow(antenna_pointer ant, int row)
{
    //1. Antenna Model
    QComboBox *cb_antenna_model = new QComboBox();
    cb_antenna_model->setObjectName("cbAntModel"+QString::number(row));
    fill_antenna_combobox(cb_antenna_model);
    set_antennacombobox_index(cb_antenna_model, ant->get_antenna_model());

    connect(cb_antenna_model, SIGNAL(currentIndexChanged(int)), this, SLOT(cb_antenna_model_changed(int)));
    ui->tblArrayEditor->setCellWidget(row,COL_ANTMODEL,cb_antenna_model);

    //2. Position (x,y,z)
    QTableWidgetItem *item;
    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(_antenna_table[row]->get_antenna_name());
//qDebug() <<_antenna_table[row]->get_antenna_name();
    ui->tblArrayEditor->setItem(row, COL_ANTNAME, item);
    connect(ui->tblArrayEditor->itemDelegate(),SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)),SLOT(antenna_name_changed(QWidget*)));

    //2. Position (x,y,z)

    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(QString::number(_antenna_table[row]->get_position(COORD_X)));
    ui->tblArrayEditor->setItem(row, COL_ANT_POSX, item);

    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(QString::number(_antenna_table[row]->get_position(COORD_Y)));
    ui->tblArrayEditor->setItem(row, COL_ANT_POSY, item);

    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(QString::number(_antenna_table[row]->get_position(COORD_Z)));
    ui->tblArrayEditor->setItem(row, COL_ANT_POSZ, item);

    // 3. Loss
    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(QString::number(_antenna_table[row]->get_loss()));
    ui->tblArrayEditor->setItem(row, COL_ANT_LOSS, item);

    // 4. Delay
    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(QString::number(_antenna_table[row]->get_delay()));
    ui->tblArrayEditor->setItem(row, COL_ANT_DELAY, item);

    // 5. Rot_Phi
    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(QString::number(_antenna_table[row]->get_rotation(true)));
    ui->tblArrayEditor->setItem(row, COL_ANT_ROT_PHI, item);

    // 6. Rot_Theta
    item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(QString::number(_antenna_table[row]->get_rotation(false)));
    ui->tblArrayEditor->setItem(row, COL_ANT_ROT_THETA, item);
}
//---------------------------------------
void wndRadarModuleEditor::add_antenna()
{
    // Create empty antenna
    antenna_pointer ant = new antennaInstance();

    QString ant_name = ant->get_name();
    int     index = 0;
    QString temp_name;
    while (1)
    {
        temp_name = ant_name + QString::number(index);
        bool bFound = false;
        for (int n=0; n < _antenna_table.count(); n++)
        {
           if (_antenna_table[n]->get_antenna_name() == temp_name)
           {bFound = true; break;}
        }
        if (!bFound) break;
        index++;
    }

    ant->set_antenna_name(temp_name);

    _antenna_table.append(ant);
    ui->tblArrayEditor->insertRow(ui->tblArrayEditor->rowCount());
    set_table_antennarow(ant,ui->tblArrayEditor->rowCount()-1);

}

void wndRadarModuleEditor::remove_wrong_txrx_pairs()
{
    // Remove lines from array combos that would become meaningless
    int n=0;

    while (n < ui->tblTxRxPairTable->rowCount())
    {
        QComboBox* cb1 = (QComboBox*)(ui->tblTxRxPairTable->cellWidget(n,0));
        QComboBox* cb2 = (QComboBox*)(ui->tblTxRxPairTable->cellWidget(n,1));

        bool found1 = false, found2 = false;
        const QVector<antenna_pointer>& _at = _antenna_table;
        for (const antenna_pointer& ant: _at)
        {
           if (cb1->currentText() == ant->get_antenna_name()) found1 = true;
           if (cb2->currentText() == ant->get_antenna_name()) found2 = true;
           if (found1&&found2) break;
        }

        if ((!found1)||(!found2))
        {
            if (_antenna_table[n]!=nullptr)
                delete _antenna_table[n];
            _antenna_table.remove(n);
            ui->tblArrayEditor->removeRow(n);
        }
        else
            n++;
    }
}
//---------------------------------------
void wndRadarModuleEditor::remove_antenna()
{
    int ant_to_remove = ui->tblArrayEditor->currentRow();
    if (ant_to_remove >= _antenna_table.count())
        return;

    if (QMessageBox::question(this, "Confirm",QString("Do you really want to delete antenna\n")+_antenna_table[ant_to_remove]->get_name() + QString("?"))==QMessageBox::No)
        return;

    if (_antenna_table[ant_to_remove]!=nullptr)
        delete _antenna_table[ant_to_remove];
    _antenna_table.removeAt(ant_to_remove);
    ui->tblArrayEditor->removeRow(ant_to_remove);

}
//---------------------------------------
void wndRadarModuleEditor::save_antennas()
{
    // Copy all rows into _radar_module

    copy_temp_array_to_module();

}
//---------------------------------------
void wndRadarModuleEditor::revert_antennas()
{
    clear_antenna_table();

    _available_antennas = _project->get_available_antennas();
    copy_radar_array_to_temp();
}
//---------------------------------------
void wndRadarModuleEditor::start_direction_focusing()
{
    if (_radar_module==nullptr) return;
    copy_temp_array_to_module();

    const auto& atable = _antenna_table;
    for (const auto& ant: atable)
        if (ant->get_antenna_model()==nullptr)
        {
            QMessageBox::critical(this,"Error","At least one antenna model is not defined");
            return;
        }

    if (_radar_module->get_antenna_count()<1)
    {
        QMessageBox::critical(this,"Error","Please define at least one antenna");
        return;
    }

    if (_radar_module->get_txrx_pairs_count()<1)
    {
        QMessageBox::critical(this,"Error","Please define at least one tx/rx combination");
        return;
    }

    wndDirectionFocusing dlgDirectionFocusing(this,_radar_module);
    dlgDirectionFocusing.exec();

}
//---------------------------------------
void wndRadarModuleEditor::start_point_focusing()
{
}
//---------------------------------------
void wndRadarModuleEditor::copy_radar_array_to_temp()
{
    for (int n=0; n < _antenna_table.count(); n++)
        if (_antenna_table[n]!=nullptr)
        delete _antenna_table[n];

    _antenna_table.clear();
    _available_antennas = _project->get_available_antennas();

    if (_radar_module==nullptr) return;

    // Sweep thru all antennas
    for (int n = 0; n < _radar_module->get_antenna_count(); n++)
    {
        antennaInstance* array_antenna = _radar_module->get_antenna(n);
        if (array_antenna!=nullptr)
        {
            /* Antenna model is already checked at module load
            // Look for a proper model
            antenna* module_antenna_model =array_antenna->get_antenna_model();
            if (module_antenna_model==nullptr)
                continue;

            bool bmodelfound = false;
            for (const auto& model : _available_antennas)
            {
                    if (model->get_name()==module_antenna_model->get_name())
                    {
                        bmodelfound = true;
                        break;
                    }
            }
            if (!bmodelfound)
            {
                QMessageBox::critical(this, "Warning", QString("The model for antenna ")+array_antenna->get_antenna_name()+"is mssing, setting to void model");
                array_antenna->set_antenna_model(nullptr);
            }
            */
            _antenna_table.append(new antennaInstance(*(array_antenna)));
        }
    }
}
//---------------------------------------
void wndRadarModuleEditor::clear_ptable()
{
    for (int n=0; n < _ptable.count(); n++)
        _ptable[n].reset();

    _ptable.clear();

}
//---------------------------------------
void wndRadarModuleEditor::copy_radar_params_to_temp()
{

    clear_ptable();
    // init the temporary param table
    QVector<radarParamPointer> radarmod_ptable = _radar_module->get_param_table();
    _ptable.resize(radarmod_ptable.count());
    for (int n=0; n < radarmod_ptable.count(); n++)
            _ptable[n]= radarmod_ptable[n]->clone();

}
//---------------------------------------
void wndRadarModuleEditor::copy_temp_params_to_radar()
{
    if (_radar_module==nullptr) return;

    update_commands(_radar_module->get_param_table(), _ptable);
    init_tables();
    command_tables_to_radar();
    _radar_module->clear_param_table();
    for (int n=0; n < _ptable.count(); n++)
        _radar_module->append_copy_of_param(_ptable[n]);


}
//---------------------------------------
void wndRadarModuleEditor::update_antenna_data(int row, int column)
{
    _bAntennaNeedValid = true;
}
//---------------------------------------
void wndRadarModuleEditor::copy_temp_array_to_module()
{
    _radar_module->clear_antenna_table();

    if (validate_antennas()>=0) return;

    for (int n=0; n < ui->tblArrayEditor->rowCount(); n++)
    {
        if (!antenna_table_row_to_antenna(_antenna_table[n],n)) continue;
        antennaInstance* new_antenna = new antennaInstance();
        new_antenna->copy_from(*(_antenna_table[n]));
        _radar_module->append_antenna(new_antenna);
    }

    remove_wrong_txrx_pairs();
}

int    wndRadarModuleEditor::validate_antennas()
{
    if (!_bAntennaNeedValid) return -1;

    for (int n=0; n < ui->tblArrayEditor->rowCount(); n++)
    {
        if (!validate_antenna_row(n))
        {
           QMessageBox::critical(this, "Error", QString("Error detected at antenna : ")+QString::number(n+1));
           return n;
        }
    }
    return -1;
}

//---------------------------------------
void wndRadarModuleEditor::fill_antenna_combobox(QComboBox* cb)
{
    cb->clear();

    cb->addItem("[No module]");

    for (int n=0; n< _available_antennas.count(); n++)
        cb->addItem(_available_antennas[n]->get_name());

    cb->addItem("[..Load New Antenna Model...]");

}

//---------------------------------------
void wndRadarModuleEditor::set_antennacombobox_index(QComboBox* cb, antenna_model_pointer ant)
{
    if (ant==nullptr)
    { cb->setCurrentIndex(0); return;}


    for (int n=0; n < cb->count(); n++)
        if (cb->itemText(n) == ant->get_name())
        {
           cb->setCurrentIndex(n);
           return;
        }
}

//---------------------------------------
void wndRadarModuleEditor::cb_antenna_model_changed(int index)
{
        if (_uploading_antenna_cb)
            return;

        _bAntennaNeedValid = true;
        _uploading_antenna_cb = true;
        QComboBox *w = qobject_cast<QComboBox *>(sender());
        QString antennaFilename;
        if (index == w->count()-1)
        {
            // Open an antenna file and add to the list
             antennaFilename = QFileDialog::getOpenFileName(this,"Open antenna model file",
                                                                ariasdk_antennas_path.absolutePath() + QDir::separator(),
                                                               tr("Antenna Model (*.antm);;All files (*.*)"),
                                                               nullptr,
                                                               QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));

            if (antennaFilename.isEmpty()) {w->setCurrentIndex(0); _uploading_antenna_cb = false; return;}
        }
        else
        {
            index--;
            if (index < 0) {w->setCurrentIndex(0);}
            _uploading_antenna_cb = false; return;
        }
        // Check if antenna is already in the db
        for (int a =0; a < _available_antennas.count(); a++)
           if (_available_antennas[a]->get_full_filepath() == antennaFilename)
           {
                w->setCurrentIndex(a);
                _uploading_antenna_cb = false;
                return;
           }

        if (QMessageBox::question(this, "Confirm","Do you want to create a project entry with the new model?")==QMessageBox::No)
        {
            w->setCurrentIndex(0); _uploading_antenna_cb = false; return;
        }

        antenna test;
        ariasdk_antennas_path.setPath(QFileInfo(antennaFilename).absolutePath());
        if (!test.load(antennaFilename))
        {
           QMessageBox::critical(this, "Error", "Error while loading antenna file");
           w->setCurrentIndex(0);
           _uploading_antenna_cb = false;
           return;
        }
        antenna* new_antenna = _project->add_antenna(antennaFilename);
        _available_antennas = _project->get_available_antennas();
        int nfound = -1;
        for (int nant = 0; nant < _available_antennas.count(); nant++)
        {
            if (_available_antennas[nant]==new_antenna)
            {
                nfound = nant;
                break;;
            }
        }
        fill_antenna_combobox(w);
        if (nfound == -1)
        {
            QMessageBox::critical(this, "Error", "Can't find new model");
            w->setCurrentIndex(0);
        }
        else
        {
            w->setCurrentIndex(nfound+1);
            _project->save_project_file();
        }

    _uploading_antenna_cb = false;
}

//---------------------------------------
void wndRadarModuleEditor::clear_antenna_table()
{
    ui->tblArrayEditor->clear();
    for (int n=0; n < _antenna_table.count(); n++)
        if (_antenna_table[n]!=nullptr)
                delete _antenna_table[n];

    _antenna_table.clear();
}


//---------------------------------------
void wndRadarModuleEditor::save_radar_module()
{
    QString radarModuleName= _radar_module->get_filename();
    if (radarModuleName.isEmpty())
    {
        bool ok;
          radarModuleName = QInputDialog::getText(this, tr("Save Module"),
                                               tr("Radar Module Name:"), QLineEdit::Normal,
                                               "newRadarModule", &ok);
          if (!ok || radarModuleName.isEmpty())
              return;
    }
    if (QFileInfo(radarModuleName).suffix()!="arm")
        radarModuleName+=".arm";

    // Check if current radar module overwrites a previous one
    QVector<radarModule*> available_modules = _project->get_available_modules();

    radarModule* prev_module = nullptr;

    for (auto& module : available_modules)
        if (module!=nullptr)
        {
            if (module->get_name() == radarModuleName)
            {
                if (_source != module)
                {
                    if (QMessageBox::question(this, "Confirm", "Do you want to overwrite?",QMessageBox::Yes|QMessageBox::No)==QMessageBox::No)
                        return;
                }
                prev_module = module;
            }
        }

    copy_temp_array_to_module();
    command_tables_to_radar();
    save_parameters();

    QFileInfo fi(QDir(_project->get_folder(cstr_radar_modules)->get_full_path()),radarModuleName);

    ariasdk_modules_path.setPath(fi.absolutePath());

    if (!_radar_module->save_file(fi.absoluteFilePath()))
        QMessageBox::critical(this, "Error", "Error while saving");
    else
        setWindowTitle(QString("Radar Module Editor : ")+_radar_module->get_name());

    // 1. We are saving a new module into the project
    if (_bNewModule && (prev_module==nullptr))
    {
        _project->add_radar_module(_radar_module);
        _source = _radar_module;
        _bNewModule = false;
        _radar_module = new radarModule(*_source);
        _project->save_project_file();
    }
    // 2. We are overwriting an old module starting from a blank one
    if (_bNewModule && (prev_module!=nullptr))
    {
        (*prev_module) = (*_radar_module);
        _bNewModule = false;
          emit radar_saved(prev_module);
        _source = prev_module;
    }
    // 3. We are saving old module
    if (!_bNewModule && (prev_module!=nullptr)&&(_source == prev_module))
    {
        (*_source)=(*_radar_module);
        _project->update_radar_module(_source);
        emit radar_saved(_source);
    }
    // 4. We are overwriting a new
    if (!_bNewModule && (prev_module!=nullptr) && (_source != prev_module))
    {
        (*prev_module) = (*_radar_module);
        _project->update_radar_module(prev_module);
        emit radar_saved(prev_module);
        _source = prev_module;
    }
    // 5. Saving an existing module as a new one
    if (!_bNewModule && (prev_module==nullptr))
    {
        _project->add_radar_module(_radar_module);
        _source = _radar_module;
        _project->update_radar_module(_source);
        _radar_module = new radarModule(*_source);
        _project->save_project_file();
    }
}


//---------------------------------------
void wndRadarModuleEditor::save_radar_module_as()
{
    bool ok;
    QString radarModuleName = QInputDialog::getText(this, tr("Save Module"),
                                           tr("Radar Module Name:"), QLineEdit::Normal,
                                           "newRadarModule", &ok);
      if (!ok || radarModuleName.isEmpty())
          return;

      if (QFileInfo(radarModuleName).suffix()!="arm")
          radarModuleName+=".arm";
    // Check if current radar module overwrites a previous one
    QVector<radarModule*> available_modules = _project->get_available_modules();
    radarModule* prev_module = nullptr;

    for (auto& module : available_modules)
        if (module!=nullptr)
        {
            if (module->get_name() == radarModuleName)
            {
                prev_module = module;
                if (_source != module)
                {
                    if (QMessageBox::question(this, "Confirm", "Do you want to overwrite?",QMessageBox::Yes|QMessageBox::No)==QMessageBox::No)
                        return;
                }
            }
        }

    if (!validate_antennas())
    {
        QMessageBox::critical(this, "Error", "Error in antennas");
        return;
    }

    copy_temp_array_to_module();
    command_tables_to_radar();
    save_parameters();

    QFileInfo fi(QDir(_project->get_folder(cstr_radar_modules)->get_full_path()),radarModuleName);

    ariasdk_modules_path.setPath(fi.absolutePath());

    if (!_radar_module->save_file(fi.absoluteFilePath()))
        QMessageBox::critical(this, "Error", "Error while saving");
    else
        setWindowTitle(QString("Radar Module Editor : ")+_radar_module->get_name());


    // 1. We are saving a new module into the project
    if (_bNewModule && (prev_module==nullptr))
    {
        _project->add_radar_module(_radar_module);
        _source = _radar_module;
        _bNewModule = false;
        _radar_module = new radarModule(*_source);
        _project->save_project_file();
    }
    // 2. We are overwriting an old module starting from a blank one
    if (_bNewModule && (prev_module!=nullptr))
    {
        (*prev_module) = (*_radar_module);
        _bNewModule = false;
          emit radar_saved(prev_module);
        _source = prev_module;
    }
    // 3. We are saving old module
    if (!_bNewModule && (prev_module==nullptr)&&(_source == prev_module))
    {
        (*_source)=(*_radar_module);
        emit radar_saved(_source);
    }
    // 4. We are overwriting a new
    if (!_bNewModule && (prev_module!=nullptr) && (_source != prev_module))
    {
        (*prev_module) = (*_radar_module);
        emit radar_saved(prev_module);
        _source = prev_module;
    }
    // 5. Saving an existing module as a new one
    if (!_bNewModule && (prev_module==nullptr))
    {
        _project->add_radar_module(_radar_module);
        _source = _radar_module;
        _radar_module = new radarModule(*_source);
        _project->save_project_file();
    }

}
//---------------------------------------
void wndRadarModuleEditor::load_radar_module()
{
    QString projectFile = QFileDialog::getOpenFileName(this,"New ARIA Radar Module",
                                                       ariasdk_modules_path.absolutePath()+QDir::separator()+QString("New Radar Module.arm"),
                                                       tr("ARIA Radar Module(*.arm);;All files (*.*)"),
                                                       nullptr,
                                                       QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));
    if (projectFile.isEmpty())
        return;

    QVector<radarModule*> available_modules = _project->get_available_modules();
    bool bPrevFound = false;
    for (auto & previous : available_modules)
        if (previous!=nullptr)
        {
            if (previous->get_full_filepath() == projectFile)
            {
                // We have a matching previous module
                _source = previous;
                _bNewModule = false;
                bPrevFound = true;
                break;
            }
        }

    if (!bPrevFound)
    {
        _bNewModule = true;
        _source = nullptr;
    }


    if (!_radar_module->load_file(projectFile))
    {
        QMessageBox::critical(this, "Error", "Error while loading");
        return;
    }

    ariasdk_modules_path.setPath(QFileInfo(projectFile).absolutePath());

    setWindowTitle(QString("Radar Module Editor : ")+_radar_module->get_name());

    copy_radar_params_to_temp();
    init_param_table();

    copy_radar_array_to_temp();
    init_antenna_table();

    init_txrx_pairs_table();
    init_tables();

    init_serial_port_combos();
}
//---------------------------------------
bool wndRadarModuleEditor::validate_antenna_row(int row)
{
    QComboBox* cb = (QComboBox*)(ui->tblArrayEditor->cellWidget(row,COL_ANTMODEL));
    if (cb==nullptr) return false;
    int ant_index = cb->currentIndex();

    /*
    if (cb->itemText(ant_index)==QString("[No Module]"))
    {
        return false;
    }*/
    ant_index--;
    if ((ant_index <-1)||(ant_index>=_available_antennas.count()))
        return false;

    QString name = ui->tblArrayEditor->item(row, COL_ANTNAME)->text();
    if (name.isEmpty()) return false;
    // Duplicate antenna name
    for (int n=0; n<_antenna_table.count(); n++)
        if ((_antenna_table[n]->get_antenna_name()==name)&&(n!=row)) return false;

    bool bOk;
    ui->tblArrayEditor->item(row,COL_ANT_POSX)->text().toDouble(&bOk);
    if (!bOk) return false;
    ui->tblArrayEditor->item(row,COL_ANT_POSY)->text().toDouble(&bOk);
    if (!bOk) return false;
    ui->tblArrayEditor->item(row,COL_ANT_POSZ)->text().toDouble(&bOk);
    if (!bOk) return false;
    ui->tblArrayEditor->item(row,COL_ANT_ROT_PHI)->text().toDouble(&bOk);
    if (!bOk) return false;
    ui->tblArrayEditor->item(row,COL_ANT_ROT_THETA)->text().toDouble(&bOk);
    if (!bOk) return false;
    ui->tblArrayEditor->item(row,COL_ANT_LOSS)->text().toDouble(&bOk);
    if (!bOk) return false;
    ui->tblArrayEditor->item(row,COL_ANT_DELAY)->text().toDouble(&bOk);
    if (!bOk) return false;

    return true;
}
//---------------------------------------
void wndRadarModuleEditor::update_all_txrx_combos(QString oldname, QString newname)
{
    for (int r=0; r < ui->tblTxRxPairTable->rowCount(); r++)
    {
        for (int c=0; c < 2; c++)
        {
            QComboBox* cb = (QComboBox*)(ui->tblTxRxPairTable->cellWidget(r,c));
            if (cb==nullptr) return;

            for (int n=0; n < cb->count(); n++)
                if (cb->itemText(n)==oldname)
                    cb->setItemText(n,newname);
        }
    }
}


//---------------------------------------
bool wndRadarModuleEditor::antenna_table_row_to_antenna(antenna_pointer ant, int row)
{

    QComboBox* cb = (QComboBox*)(ui->tblArrayEditor->cellWidget(row,COL_ANTMODEL));
    if (cb==nullptr) return false;
    int ant_index = cb->currentIndex()-1;

    if (!validate_antenna_row(row)) return false;
    QString new_name = ui->tblArrayEditor->item(row,COL_ANTNAME)->text();
    QString old_name = ant->get_antenna_name();

    if (new_name != old_name)
        update_all_txrx_combos(old_name, new_name);

    ant->set_antenna_name(new_name);
    ant->set_antenna_model(ant_index == -1 ?
                nullptr : _available_antennas.at(ant_index));

    ant->set_position(0, ui->tblArrayEditor->item(row,COL_ANT_POSX)->text().toDouble());
    ant->set_position(1, ui->tblArrayEditor->item(row,COL_ANT_POSY)->text().toDouble());
    ant->set_position(2, ui->tblArrayEditor->item(row,COL_ANT_POSZ)->text().toDouble());

    ant->set_rotation(true,  ui->tblArrayEditor->item(row,COL_ANT_ROT_PHI)->text().toDouble());
    ant->set_rotation(false, ui->tblArrayEditor->item(row,COL_ANT_ROT_THETA)->text().toDouble());

    ant->set_delay(ui->tblArrayEditor->item(row,COL_ANT_DELAY)->text().toDouble());
    ant->set_loss(ui->tblArrayEditor->item(row,COL_ANT_LOSS)->text().toDouble());

    return true;

}
//---------------------------------------
void wndRadarModuleEditor::init_txrx_pairs_table()
{

    ui->tblTxRxPairTable->clear();
    ui->tblTxRxPairTable->setRowCount(0);
    ui->tblTxRxPairTable->setColumnCount(2);
    ui->tblTxRxPairTable->setHorizontalHeaderLabels(QStringList({"Antenna Tx","Antenna Rx"}));
    if (_antenna_table.count()==0) return;
    for (int n=0; n < _radar_module->get_txrx_pairs_count(); n++)
    {
        add_txrx_pair(false);
        QPair<int,int> pair = _radar_module->get_txrx_pair(n);
        ((QComboBox*)(ui->tblTxRxPairTable->cellWidget(n,0)))->setCurrentIndex(pair.first);
        ((QComboBox*)(ui->tblTxRxPairTable->cellWidget(n,1)))->setCurrentIndex(pair.second);
    }
}

void wndRadarModuleEditor::init_txrx_antenna_combobox(QComboBox* cb)
{
    cb->clear();
    for (int n=0; n < _antenna_table.count(); n++)
        cb->addItem(_antenna_table[n]->get_antenna_name());
}
//---------------------------------------
void wndRadarModuleEditor::current_txrx_pair_changed(int current)
{
    if (_radar_module==nullptr) return;

    int nRow = ui->tblTxRxPairTable->currentRow();
    int nCol = ui->tblTxRxPairTable->currentColumn();
    QPair<int,int> txrx = _radar_module->get_txrx_pair(nRow);
    int antenna_id = ((QComboBox*)(sender()))->currentIndex();
    (nCol==0 ?  txrx.first : txrx.second) = antenna_id;
    _radar_module->set_txrx_pair(nRow, txrx.first, txrx.second);

}
//---------------------------------------
void wndRadarModuleEditor::add_txrx_pair()
{
    if (_antenna_table.count()==0) return;
    add_txrx_pair(true);
}
//---------------------------------------
void wndRadarModuleEditor::add_txrx_pair(bool create_new)
{

    if (validate_antennas()>=0) return;

    ui->tblTxRxPairTable->insertRow(ui->tblTxRxPairTable->rowCount());
    int row = ui->tblTxRxPairTable->rowCount()-1;
    QComboBox   *item = new QComboBox();
    init_txrx_antenna_combobox(item);
    ui->tblTxRxPairTable->setCellWidget(row,0,item);

    connect(item,SIGNAL(currentIndexChanged(int)), this, SLOT(current_txrx_pair_changed(int)));

    item = new QComboBox();
    init_txrx_antenna_combobox(item);
    ui->tblTxRxPairTable->setCellWidget(row,1,item);

    connect(item,SIGNAL(currentIndexChanged(int)), this, SLOT(current_txrx_pair_changed(int)));

    if (create_new)
    {
        ((QComboBox*)(ui->tblTxRxPairTable->cellWidget(row,0)))->setCurrentIndex(0);
        ((QComboBox*)(ui->tblTxRxPairTable->cellWidget(row,1)))->setCurrentIndex(0);
        _radar_module->append_txrx_pair(0,0);
    }

}

//---------------------------------------
void wndRadarModuleEditor::remove_txrx_pair()
{
    int row = ui->tblTxRxPairTable->currentRow();
    //_radar_module.
    if ((row<0)||(row>=ui->tblTxRxPairTable->rowCount()))
        return;

    ui->tblTxRxPairTable->removeRow(row);
    if (_radar_module!=nullptr)
        _radar_module->remove_txrx_pair(row);
}

//---------------------------------------------------------
// Context menus
void wndRadarModuleEditor::initParams_RightClick(QPoint pos)
{
    QList<QTableWidgetItem *> selected = ui->tblParamsInit->selectedItems();

    QMenu *menu = new QMenu(this);
    QAction *paramNew = new QAction("Add new init params");
    QAction *paramRemove = new QAction("Remove init param(s)");

    paramRemove->setEnabled(selected.count()>0);
    connect(paramNew, SIGNAL(triggered()), this, SLOT(add_initParam()));
    connect(paramRemove, SIGNAL(triggered()), this,SLOT(remove_initParam()));
    menu->addAction(paramNew);
    menu->addAction(paramRemove);
    menu->popup(ui->tblParamsInit->viewport()->mapToGlobal(pos));
}
//---------------------------------------------------------

void wndRadarModuleEditor::acqParams_RightClick(QPoint pos)
{
    QList<QTableWidgetItem *> selected = ui->tblParamsAcquisition->selectedItems();

    QMenu *menu = new QMenu(this);
    QAction *paramNew = new QAction("Add new acquisition params");
    QAction *paramRemove = new QAction("Remove acquisition param(s)");

    paramRemove->setEnabled(selected.count()>0);
    connect(paramNew, SIGNAL(triggered()), this, SLOT(add_acquisitionParam()));
    connect(paramRemove, SIGNAL(triggered()), this,SLOT(remove_acquisitionParam()));
    menu->addAction(paramNew);
    menu->addAction(paramRemove);
    menu->popup(ui->tblParamsAcquisition->viewport()->mapToGlobal(pos));
}
//---------------------------------------------------------

void wndRadarModuleEditor::postacqParams_RightClick(QPoint pos)
{
    QList<QTableWidgetItem *> selected = ui->tblParamsPostAcquisition->selectedItems();

    QMenu *menu = new QMenu(this);
    QAction *paramNew = new QAction("Add new post-acquisition params");
    QAction *paramRemove = new QAction("Remove post-acquisition param(s)");

    paramRemove->setEnabled(selected.count()>0);
    connect(paramNew, SIGNAL(triggered()), this, SLOT(add_postAcquisitionParam()));
    connect(paramRemove, SIGNAL(triggered()), this,SLOT(remove_postAcquisitionParam()));
    menu->addAction(paramNew);
    menu->addAction(paramRemove);
    menu->popup(ui->tblParamsPostAcquisition->viewport()->mapToGlobal(pos));
}
//---------------------------------------------------------


void wndRadarModuleEditor::initScripts_RightClick(QPoint pos)
{
    QList<QTableWidgetItem *> selected = ui->tblScriptsInit->selectedItems();

    QMenu *menu = new QMenu(this);
    QAction *paramNew = new QAction("Add new init script");
    QAction *paramRemove = new QAction("Remove init script(s)");

    paramRemove->setEnabled(selected.count()>0);
    connect(paramNew, SIGNAL(triggered()), this, SLOT(add_initScript()));
    connect(paramRemove, SIGNAL(triggered()), this,SLOT(remove_initScript()));
    menu->addAction(paramNew);
    menu->addAction(paramRemove);
    menu->popup(ui->tblScriptsInit->viewport()->mapToGlobal(pos));
}
//---------------------------------------------------------

void wndRadarModuleEditor::acqScripts_RightClick(QPoint pos)
{
    QList<QTableWidgetItem *> selected = ui->tblScriptsAcquisition->selectedItems();

    QMenu *menu = new QMenu(this);
    QAction *paramNew = new QAction("Add new acquisition params");
    QAction *paramRemove = new QAction("Remove acquisition param(s)");

    paramRemove->setEnabled(selected.count()>0);
    connect(paramNew, SIGNAL(triggered()), this, SLOT(add_acquisitionScript()));
    connect(paramRemove, SIGNAL(triggered()), this,SLOT(remove_acquisitionScript()));
    menu->addAction(paramNew);
    menu->addAction(paramRemove);
    menu->popup(ui->tblScriptsAcquisition->viewport()->mapToGlobal(pos));
}
//---------------------------------------------------------

void wndRadarModuleEditor::postacqScripts_RightClick(QPoint pos)
{
    QList<QTableWidgetItem *> selected = ui->tblScriptsPostAcq->selectedItems();

    QMenu *menu = new QMenu(this);
    QAction *paramNew = new QAction("Add new post-acquisition script");
    QAction *paramRemove = new QAction("Remove post-acquisition script(s)");

    paramRemove->setEnabled(selected.count()>0);
    connect(paramNew, SIGNAL(triggered()), this, SLOT(add_postAcquisitionScript()));
    connect(paramRemove, SIGNAL(triggered()), this,SLOT(remove_postAcquisitionScript()));
    menu->addAction(paramNew);
    menu->addAction(paramRemove);
    menu->popup(ui->tblScriptsPostAcq->viewport()->mapToGlobal(pos));
}


//---------------------------------------
void wndRadarModuleEditor::add_initParam( )
{
    if (_radar_module==nullptr) return;

    int n = ui->tblParamsInit->rowCount();

    ui->tblParamsInit->insertRow(n);
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setText(QString::number(n+1));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tblParamsInit->setItem(n, 0, item);

    QComboBox *cb = new QComboBox();
    cb->setObjectName("initParam"+QString::number(n+1));

    fill_param_combobox(cb, 0);
    ui->tblParamsInit->setCellWidget(n,1 , cb);

    QVector<int> param_init = _radar_module->get_init_commands();
    param_init.append(0);
    _radar_module->set_init_commands(param_init);
}
//---------------------------------------
void wndRadarModuleEditor::remove_initParam( )
{
    if (_radar_module==nullptr) return;
    command_tables_to_radar();
    QList<QTableWidgetItem *> selected = ui->tblParamsInit->selectedItems();
    QVector<int> commands = _radar_module->get_init_commands();

    for (int n=selected.count()-1; n >=0  ; n--)
    {
        int row = selected[n]->row();
        ui->tblParamsInit->removeRow(row);

        commands.removeAt(row);
    }
    _radar_module->set_init_commands(commands);

    renumber_table(ui->tblParamsInit);
}
//---------------------------------------
void wndRadarModuleEditor::add_acquisitionParam( )
{
    if (_radar_module==nullptr) return;

    int n = ui->tblParamsAcquisition->rowCount();

    ui->tblParamsAcquisition->insertRow(n);
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setText(QString::number(n+1));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tblParamsAcquisition->setItem(n, 0, item);

    QComboBox *cb = new QComboBox();
    cb->setObjectName("acquisitionParams"+QString::number(n+1));

    fill_param_combobox(cb, 0);
    ui->tblParamsAcquisition->setCellWidget(n,1 , cb);

    QVector<int> params = _radar_module->get_acquisition_commands();
    params.append(0);
    _radar_module->set_acquisition_commands(params);

}
//---------------------------------------
void wndRadarModuleEditor::remove_acquisitionParam( )
{
    if (_radar_module==nullptr) return;
    command_tables_to_radar();
    QList<QTableWidgetItem *> selected = ui->tblParamsAcquisition->selectedItems();
    QVector<int> commands = _radar_module->get_acquisition_commands();

    for (int n=selected.count()-1; n >=0  ; n--)
    {
        int row = selected[n]->row();
        ui->tblParamsAcquisition->removeRow(row);

        commands.removeAt(row);
    }
    _radar_module->set_acquisition_commands(commands);

    renumber_table(ui->tblParamsAcquisition);

}
//---------------------------------------
void wndRadarModuleEditor::add_postAcquisitionParam( )
{
    if (_radar_module==nullptr) return;

    int n = ui->tblParamsPostAcquisition->rowCount();

    ui->tblParamsPostAcquisition->insertRow(n);
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setText(QString::number(n+1));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tblParamsPostAcquisition->setItem(n, 0, item);

    QComboBox *cb = new QComboBox();
    cb->setObjectName("postAcquisitionParams"+QString::number(n+1));

    fill_param_combobox(cb, 0);
    ui->tblParamsPostAcquisition->setCellWidget(n,1 , cb);

    QVector<int> params = _radar_module->get_postacquisition_commands();
    params.append(0);
    _radar_module->set_postacquisition_commands(params);

}
//---------------------------------------
void wndRadarModuleEditor::remove_postAcquisitionParam( )
{
    if (_radar_module==nullptr) return;
    command_tables_to_radar();
    QList<QTableWidgetItem *> selected = ui->tblParamsPostAcquisition->selectedItems();
    QVector<int> commands = _radar_module->get_postacquisition_commands();

    for (int n=selected.count()-1; n >=0  ; n--)
    {
        int row = selected[n]->row();
        ui->tblParamsPostAcquisition->removeRow(row);

        commands.removeAt(row);

    }
    _radar_module->set_postacquisition_commands(commands);
    renumber_table(ui->tblParamsPostAcquisition);
}
//---------------------------------------
void wndRadarModuleEditor::add_initScript( )
{
    if (_radar_module==nullptr) return;

    octaveScript* script = add_new_script();
    if (script==nullptr) return;

    int n = ui->tblScriptsInit->rowCount();

    ui->tblScriptsInit->insertRow(n);
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setText(QString::number(n+1));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tblScriptsInit->setItem(n, 0, item);

    item = new QTableWidgetItem();
    item->setText(script->get_name());
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tblScriptsInit->setItem(n, 1, item);

    QVector<octaveScript_ptr> scripts = _radar_module->get_init_scripts();
    scripts.append(script);
    _radar_module->set_init_scripts(scripts);

}
//---------------------------------------
void wndRadarModuleEditor::remove_initScript( )
{
    if (_radar_module==nullptr) return;

    QList<QTableWidgetItem *> selected = ui->tblScriptsInit->selectedItems();
    QVector<octaveScript_ptr> scripts = _radar_module->get_init_scripts();

    for (int n=selected.count()-1; n >=0  ; n--)
    {
        int row = selected[n]->row();
        ui->tblScriptsInit->removeRow(row);
        delete scripts[row];
        scripts.removeAt(row);
    }
    _radar_module->set_init_scripts(scripts);

    renumber_table(ui->tblScriptsInit);


}
//---------------------------------------
void wndRadarModuleEditor::add_acquisitionScript( )
{
    octaveScript* script = add_new_script();
    if (script==nullptr) return;

    int n = ui->tblScriptsAcquisition->rowCount();

    ui->tblScriptsAcquisition->insertRow(n);
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setText(QString::number(n+1));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tblScriptsAcquisition->setItem(n, 0, item);

    item = new QTableWidgetItem();
    item->setText(script->get_name());
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tblScriptsAcquisition->setItem(n, 1, item);

    QVector<octaveScript_ptr> scripts = _radar_module->get_acquisition_scripts();
    scripts.append(script);
    _radar_module->set_acquisition_scripts(scripts);

}
//---------------------------------------
void wndRadarModuleEditor::remove_acquisitionScript( )
{
    if (_radar_module==nullptr) return;

    QList<QTableWidgetItem *> selected = ui->tblScriptsAcquisition->selectedItems();
    QVector<octaveScript_ptr> scripts = _radar_module->get_acquisition_scripts();

    for (int n=selected.count()-1; n >=0  ; n--)
    {
        int row = selected[n]->row();
        ui->tblScriptsAcquisition->removeRow(row);
        delete scripts[row];
        scripts.removeAt(row);
    }
    _radar_module->set_acquisition_scripts(scripts);

    renumber_table(ui->tblScriptsAcquisition);
}
//---------------------------------------
void wndRadarModuleEditor::add_postAcquisitionScript( )
{
    octaveScript* script = add_new_script();
    if (script==nullptr) return;

    int n = ui->tblScriptsPostAcq->rowCount();

    ui->tblScriptsPostAcq->insertRow(n);
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setText(QString::number(n+1));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tblScriptsPostAcq->setItem(n, 0, item);

    item = new QTableWidgetItem();
    item->setText(script->get_name());
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tblScriptsPostAcq->setItem(n, 1, item);

    QVector<octaveScript_ptr> scripts = _radar_module->get_postacquisition_scripts();
    scripts.append(script);
    _radar_module->set_postacquisition_scripts(scripts);


}
//---------------------------------------
void wndRadarModuleEditor::remove_postAcquisitionScript( )
{
    if (_radar_module==nullptr) return;

    QList<QTableWidgetItem *> selected = ui->tblScriptsPostAcq->selectedItems();
    QVector<octaveScript_ptr> scripts = _radar_module->get_postacquisition_scripts();

    for (int n=selected.count()-1; n >=0  ; n--)
    {
        int row = selected[n]->row();
        ui->tblScriptsPostAcq->removeRow(row);
        delete scripts[row];
        scripts.removeAt(row);
    }
    _radar_module->set_postacquisition_scripts(scripts);

    renumber_table(ui->tblScriptsPostAcq);
}
//---------------------------------------
void wndRadarModuleEditor::init_tables()
{
    ui->tblParamsInit->clear();
    ui->tblParamsAcquisition->clear();
    ui->tblParamsPostAcquisition->clear();

    ui->tblParamsInit->setColumnCount(2);
    ui->tblParamsInit->setRowCount(0);
    ui->tblParamsInit->setHorizontalHeaderLabels(QStringList({"Sequence step","Parameter"}));

    ui->tblParamsAcquisition->setColumnCount(2);
    ui->tblParamsAcquisition->setRowCount(0);
    ui->tblParamsAcquisition->setHorizontalHeaderLabels(QStringList({"Sequence step","Parameter"}));

    ui->tblParamsPostAcquisition->setColumnCount(2);
    ui->tblParamsPostAcquisition->setRowCount(0);
    ui->tblParamsPostAcquisition->setHorizontalHeaderLabels(QStringList({"Sequence step","Parameter"}));

    ui->tblScriptsInit->setColumnCount(2);
    ui->tblScriptsInit->setRowCount(0);
    ui->tblScriptsInit->setHorizontalHeaderLabels(QStringList({"Sequence step","Script"}));

    ui->tblScriptsAcquisition->setColumnCount(2);
    ui->tblScriptsAcquisition->setRowCount(0);
    ui->tblScriptsAcquisition->setHorizontalHeaderLabels(QStringList({"Sequence step","Script"}));

    ui->tblScriptsPostAcq->setColumnCount(2);
    ui->tblScriptsPostAcq->setRowCount(0);
    ui->tblScriptsPostAcq->setHorizontalHeaderLabels(QStringList({"Sequence step","Script"}));

    QVector<int> init_params        = _radar_module==nullptr? QVector<int>():_radar_module->get_init_commands();
    QVector<int> acq_params         = _radar_module==nullptr? QVector<int>():_radar_module->get_acquisition_commands();
    QVector<int> postacq_params     = _radar_module==nullptr? QVector<int>():_radar_module->get_postacquisition_commands();

    ui->tblParamsInit->setRowCount(init_params.count());
    ui->tblParamsAcquisition->setRowCount(acq_params.count());
    ui->tblParamsPostAcquisition->setRowCount(postacq_params.count());

    _init_scripts       = _radar_module == nullptr ? QVector<octaveScript_ptr>() : _radar_module->get_init_scripts();
    _acq_scripts        = _radar_module == nullptr ? QVector<octaveScript_ptr>() : _radar_module->get_acquisition_scripts();
    _postacq_scripts    = _radar_module == nullptr ? QVector<octaveScript_ptr>() : _radar_module->get_postacquisition_scripts();

    ui->tblScriptsInit->setRowCount(_init_scripts.count());
    ui->tblScriptsAcquisition->setRowCount(_acq_scripts.count());
    ui->tblScriptsPostAcq->setRowCount(_postacq_scripts.count());

    for (int n=0; n < init_params.count(); n++)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(QString::number(n+1));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tblParamsInit->setItem(n, 0, item);

        QComboBox *cb = new QComboBox();
        cb->setObjectName("initParam"+QString::number(n+1));

        fill_param_combobox(cb, init_params[n]);
        ui->tblParamsInit->setCellWidget(n,1 , cb);
    }

    for (int n=0; n < acq_params.count(); n++)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(QString::number(n+1));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tblParamsAcquisition->setItem(n, 0, item);

        QComboBox *cb = new QComboBox();
        cb->setObjectName("acqParam"+QString::number(n+1));

        fill_param_combobox(cb, acq_params[n]);
        ui->tblParamsAcquisition->setCellWidget(n,1 , cb);
    }

    for (int n=0; n < postacq_params.count(); n++)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(QString::number(n+1));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tblParamsPostAcquisition->setItem(n, 0, item);

        QComboBox *cb = new QComboBox();
        cb->setObjectName("postParam"+QString::number(n+1));

        fill_param_combobox(cb, postacq_params[n]);
        ui->tblParamsPostAcquisition->setCellWidget(n,1 , cb);
    }

    for (int n=0; n < _init_scripts.count(); n++)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(QString::number(n+1));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tblScriptsInit->setItem(n, 0, item);

        item = new QTableWidgetItem();
        item->setText(_init_scripts[n]->get_name());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tblScriptsInit->setItem(n, 1, item);
    }

    for (int n=0; n < _acq_scripts.count(); n++)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(QString::number(n+1));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tblScriptsAcquisition->setItem(n, 0, item);

        item = new QTableWidgetItem();
        item->setText(_acq_scripts[n]->get_name());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tblScriptsAcquisition->setItem(n, 1, item);
    }

    for (int n=0; n < _postacq_scripts.count(); n++)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(QString::number(n+1));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tblScriptsPostAcq->setItem(n, 0, item);

        item = new QTableWidgetItem();
        item->setText(_postacq_scripts[n]->get_name());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tblScriptsPostAcq->setItem(n, 1, item);
    }
}
//---------------------------------------
void wndRadarModuleEditor::fill_param_combobox(QComboBox* cb, int current)
{
    if (cb==nullptr)
        return;
    if (_radar_module==nullptr)
        return;

    cb->clear();
    for (int n=0; n < _radar_module->get_param_count(); n++)
        cb->addItem(_radar_module->get_param(n)->get_name());

    if ((current >=0)&&(current<_radar_module->get_param_count()))
        cb->setCurrentIndex(current);
}

//---------------------------------------
void wndRadarModuleEditor::update_commands(QVector<radarParamPointer> previous_table, QVector<radarParamPointer> new_table)
{
    // For each param in (init, acquisition, post-acquisition)
    if (_radar_module==nullptr) return;
    _radar_module->set_init_commands(update_commands(_radar_module->get_init_commands(), previous_table, new_table));
    _radar_module->set_acquisition_commands(update_commands(_radar_module->get_acquisition_commands(), previous_table, new_table));
    _radar_module->set_postacquisition_commands(update_commands(_radar_module->get_postacquisition_commands(), previous_table, new_table));
}

void wndRadarModuleEditor::update_commands(QString old_param_name, QString new_param_name)
{
    QVector<QTableWidget*> tables({ui->tblParamsInit, ui->tblParamsAcquisition, ui->tblParamsPostAcquisition});
    const QVector<QTableWidget*> _table_const = tables;
    for (auto table : _table_const)
    {
        for (int row = 0; row < table->rowCount(); row++)
        {
            QComboBox* cb = (QComboBox*)(table->cellWidget(row,1));
            if (cb==nullptr) continue;
        }
    }

}
//---------------------------------------
QVector<int> wndRadarModuleEditor::update_commands(QVector<int> paramlist, QVector<radarParamPointer> previous_table, QVector<radarParamPointer> new_table)
{
    for (int n=paramlist.count()-1 ; n >=0 ; n--)
    {
        int nParam = paramlist[n];
        if ((nParam < 0)||(nParam>=previous_table.count()))
            paramlist.removeAt(n);

        if ((nParam < 0)||(nParam>=new_table.count()))
            paramlist.removeAt(n);

        QString param_name = previous_table[nParam]->get_name();

        if (param_name==new_table[nParam]->get_name())
            continue;

        // Search for the new parameter
        int found = -1;
        for (int m=0; m<new_table.count();m++)
            if (new_table[m]->get_name()==param_name)
            {
                found = m; break;
            }

        if (found==-1)
            paramlist.remove(n);
        else
            paramlist[n] = found;
    }

    return paramlist;
}


//---------------------------------------
void  wndRadarModuleEditor::command_tables_to_radar()
{
    if (_radar_module==nullptr) return;

    QVector<int> vecParams;
    vecParams.resize(ui->tblParamsInit->rowCount());

    for (int n=0; n<ui->tblParamsInit->rowCount(); n++)
    {
        QComboBox* cb = (QComboBox*)(ui->tblParamsInit->cellWidget(n,1));
        int index = cb->currentIndex();
        if ((index  < 0)||(index>=_radar_module->get_param_count()))
            continue;
        vecParams[n]=index;
    }
    _radar_module->set_init_commands(vecParams);

    vecParams.resize(ui->tblParamsAcquisition->rowCount());

    for (int n=0; n<ui->tblParamsAcquisition->rowCount(); n++)
    {
        QComboBox* cb = (QComboBox*)(ui->tblParamsAcquisition->cellWidget(n,1));
        int index = cb->currentIndex();
        if ((index  < 0)||(index>=_radar_module->get_param_count()))
            continue;
        vecParams[n]=index;
    }
    _radar_module->set_acquisition_commands(vecParams);

    vecParams.resize(ui->tblParamsPostAcquisition->rowCount());

    for (int n=0; n<ui->tblParamsPostAcquisition->rowCount(); n++)
    {
        QComboBox* cb = (QComboBox*)(ui->tblParamsPostAcquisition->cellWidget(n,1));
        int index = cb->currentIndex();
        if ((index  < 0)||(index>=_radar_module->get_param_count()))
            continue;
        vecParams[n]=index;
    }
    _radar_module->set_postacquisition_commands(vecParams);
}

//---------------------------------------
void  wndRadarModuleEditor::script_tables_to_radar()
{
    if (_radar_module==nullptr) return;
    _radar_module->set_init_scripts(_init_scripts);
    _radar_module->set_acquisition_scripts(_acq_scripts);
    _radar_module->set_postacquisition_scripts(_postacq_scripts);
}

//---------------------------------------
void    wndRadarModuleEditor::renumber_table(QTableWidget* table)
{
    for (int n=0; n < table->rowCount(); n++)
    {
        QTableWidgetItem*   item = (QTableWidgetItem*)(table->item(n,0));
        if (item!=nullptr)
        item->setText(QString::number(n+1));
    }
}


//---------------------------------------
octaveScript* wndRadarModuleEditor::add_new_script()
{
    // Open an antenna file and add to the list
    QString scriptFileName = QFileDialog::getOpenFileName(this,"Script file",
                                                          ariasdk_scripts_path.absolutePath()+QDir::separator(),
                                                           tr("Script file (*.m);;All files (*.*)"),
                                                           nullptr,
                                                           QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));

    if (scriptFileName.isEmpty()) return nullptr;
    ariasdk_scripts_path.setPath(QFileInfo(scriptFileName).absolutePath());
   octaveScript* script =  _project->add_script(scriptFileName);
   if (script!=nullptr)
     _project->save_project_file();

    return script;
}

//---------------------------------------
bool    wndRadarModuleEditor::validate_param_groups()
{
    return true;
}

//---------------------------------------
void        wndRadarModuleEditor::init_serial_port_combos()
{
    ui->cbParity->clear();

    SerialSettings settings;
    if (_radar_module !=nullptr)
        settings = _radar_module->get_serial_port_configuration();
    else
        settings = {
            115200,
            QSerialPort::Data8,
            QSerialPort::NoParity,
            QSerialPort::OneStop,
            QSerialPort::NoFlowControl
        };

    ui->cbBaudRate->setCurrentText(QString::number(settings.baudRate));

    for (auto par_descr = serial_port_parity_descriptor.begin(), end = serial_port_parity_descriptor.end();
         par_descr != end;
         par_descr++)
        ui->cbParity->addItem(par_descr.value());

    ui->cbParity->setCurrentText(serial_port_parity_descriptor.value(settings.parity));


    ui->cbFlowControl->clear();
    for (auto par_flow = serial_port_flowcontrol_descriptor.begin(), end = serial_port_flowcontrol_descriptor.end();
         par_flow != end;
         par_flow++)
        ui->cbFlowControl->addItem(par_flow.value());

    ui->cbFlowControl->setCurrentText(serial_port_flowcontrol_descriptor.value(settings.flowControl));

    ui->cbStopBits->clear();
    for (auto sb_flow = serial_port_stopbits_descriptor.begin(), end = serial_port_stopbits_descriptor.end();
         sb_flow != end;
         sb_flow++)
        ui->cbStopBits->addItem(sb_flow.value());

    ui->cbStopBits->setCurrentText(serial_port_stopbits_descriptor.value(settings.stopBits));

    ui->sbDataBits->setValue(settings.dataBits);

    ui->leSerialDescription->setText(_radar_module->get_serial_description());
    ui->leSerialManufacturer->setText(_radar_module->get_serial_manufacturer());
    ui->leSerialNumber->setText(_radar_module->get_serial_number());
    ui->leProductId->setText(_radar_module->get_serial_productid());
    ui->leVendorId->setText(_radar_module->get_serial_vendorid());
}

//---------------------------------------
void wndRadarModuleEditor::update_cb_inquiry()
{

    const QVector<radarParamPointer>& px = _ptable;
    ui->cbModuleInquiry->clear();
    ui->cbRadarId->clear();

    if (_radar_module==nullptr) return;

    for (const auto& param:px)
    if (param!=nullptr)
    {
        QString parent_name = QByteArray(param->get_command_group().toHex(0));
        // add only unique elements
        if (ui->cbModuleInquiry->findText(parent_name)==-1)
        {
            ui->cbModuleInquiry->addItem(parent_name);
            ui->cbRadarId->addItem(parent_name);
        }
    }
    ui->cbModuleInquiry->setCurrentText(_radar_module->get_inquiry_moduleid_command_as_string());
    ui->cbRadarId->setCurrentText(_radar_module->get_inquiry_instanceid_command_as_string());
    ui->leInquiryCommand->setText(_radar_module->get_inquiry_moduleid_expectedvalue_as_string());
}
//---------------------------------------
void wndRadarModuleEditor::scanRadarDevices()
{
    QVector<projectItem*> radarModules;
    radarModules.append(_radar_module);
    wndScanModules scanModules(radarModules, _project, this);
    scanModules.exec();
}
//---------------------------------------
void wndRadarModuleEditor::createDevice()
{
    // Radar module must be updated before continuing
    if (QMessageBox::question(this,"Confirm","Do you want to save the current module before proceeding?") == QMessageBox::No)
        return;
    // Save_radar_module will take care of checking if we need to "save as.." or not
    save_radar_module();

    //_source is the module we have to use for the new radarInstance since we need to link to a project item
    radarInstance* new_device = new radarInstance(_source);
    new_device->attach_to_workspace(_project->get_workspace());
    if (new_device==nullptr)
    {
        QMessageBox::critical(this,"Error","Error while creating virtual device");
        return;
    }
    QVector<radarModule*> availableModules; availableModules.append(_source);
        wndRadarInstanceEditor deviceEditor(new_device,availableModules,this);
    if (deviceEditor.exec()==QMessageBox::Ok)
    {

        if (_project->get_last_error().isEmpty())
        {
            _project->save_project_file();
            return;
        }
        QMessageBox::critical(this,"Error",_project->get_last_error());
    }

    delete new_device;
}
