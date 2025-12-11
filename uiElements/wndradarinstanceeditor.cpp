/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "wndradarinstanceeditor.h"
#include "ui_wndradarinstanceeditor.h"
#include "wndscanmodules.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QEvent>
#include <QWidget>
#include <QDialog>
#include <QSpinBox>
#include <QPainter>

#define EXPORT_VOID

#define COL_NAME 0
#define COL_DEFAULT_VALUE 1
#define COL_SETVALUE 2
#define COL_READVALUE 3
#define COL_EXPORT_OCTAVE 4
//#define COL_PLOTTYPE 5
#define COL_ALIAS 5
#define COL_COMPOUND_NAME 6

extern QDir             ariasdk_modules_path;
extern QDir             ariasdk_projects_path;
extern QDir             ariasdk_scripts_path;
extern QDir             ariasdk_antennas_path;
extern QDir             ariasdk_antennaff_path;

//---------------------------------------------------------------
wndRadarInstanceEditor::wndRadarInstanceEditor(radarInstance* radar, QVector<radarModule*> available_models, QWidget *parent) :
    QDialog(parent),
    _b_handling_script(false),
    ui(new Ui::wndRadarInstanceEditor),
    _project(nullptr),
    _radar_instance(nullptr),
    _available_models(available_models),
    _b_transmitting(false),
    _scheduler(nullptr)
{
    octavews* ws = nullptr;
    if (available_models.count()>0)
    {
        if (_project==nullptr)
            _project= available_models[0]->get_root();
        ws = _project->get_workspace();
    }

    ui->setupUi(this);
    if (radar==nullptr)
    {
        _radar_instance = new radarInstance(available_models[0]);
        connect_serial_updates();
        for (auto module: available_models)
        {
            if (module==nullptr) continue;
            ui->cbAvailableModules->addItem(module->get_name());
            if (available_models.count() > 0)
            {
                _radar_instance->attach_to_workspace(ws);
            }
        }
        _backup = nullptr;
    }
    else
    {
        _radar_instance = radar;

        ui->btnConnect->setText(_radar_instance->is_connected()? "Disconnect" : "Connect");

        connect_serial_updates();
        if (_radar_instance->get_module()!=nullptr)
        {
            ui->cbAvailableModules->addItem(_radar_instance->get_module()->get_name());
            ui->cbAvailableModules->setEnabled(false);
        }
        else
        {
            QMessageBox::critical(this,"Error","No modules available, skipping");
            done(-1);
        }
        _project = _radar_instance->get_root();

        _backup = new radarInstance();
        (*_backup) = (*_radar_instance);
    }
    _available_scripts = _project->get_available_scripts();

    ui->cbFixedId->setChecked(_radar_instance->fixed_id());
    ui->cbFixedPort->setChecked(_radar_instance->fixed_port());
    ui->leAliasName->setText(QFileInfo(_radar_instance->get_name()).baseName());
    ui->leRadarId->setText(_radar_instance->get_uid_string());
    ui->leRadarPort->setText(_radar_instance->get_expected_portname());


    connect(ui->btnSave,    &QPushButton::clicked, this, &wndRadarInstanceEditor::confirm);
    connect(ui->btnCancel,  &QPushButton::clicked, this, &wndRadarInstanceEditor::cancel);
    connect(ui->btnConnect, &QPushButton::clicked, this, &wndRadarInstanceEditor::connect_radar);
    connect(ui->btnScan,    &QPushButton::clicked, this, &wndRadarInstanceEditor::scan);
    connect(ui->btnSendCustomString,
                            &QPushButton::clicked, this, &wndRadarInstanceEditor::send_custom_string);

    connect(ui->btbImportFromOctave,
                            &QPushButton::clicked, this, &wndRadarInstanceEditor::importFromOctave);
    connect(ui->btnExportToOctave,
                            &QPushButton::clicked, this, &wndRadarInstanceEditor::exportToOctave);

    connect(ui->btnInit,    &QPushButton::clicked, this, &wndRadarInstanceEditor::init);
    connect(ui->btnClear,   &QPushButton::clicked, this, &wndRadarInstanceEditor::clearSerialOutput);

    connect(ui->btnRun, &QPushButton::clicked, this, &wndRadarInstanceEditor::run);

    // Create an icon for the missing scripts
    QString iconSource = ":/icons/broken-link-icon.png";
    QPixmap px(iconSource);
    QPainter painter(&px);
    QColor iconColor("red");
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.setBrush(iconColor);
    painter.setPen(iconColor);
    painter.drawRect(px.rect());
    _icon = QIcon(px);

    init_script_tables();
    init_parameter_table();

    ui->teSerialOutput->setReadOnly(true);
    ui->tabWidget->setCurrentIndex(2);
}
//------------------------------------------------------------  ---
wndRadarInstanceEditor::~wndRadarInstanceEditor()
{
    // The disconnect on delete must be done by each radarInstance
    /*
    if (_radar_instance == nullptr)
        return;
*/

    if (_backup != nullptr) delete _backup;
    if (_radar_instance!=nullptr)
        if (_radar_instance->is_connected())
            _radar_instance->disconnect();

    if (_scheduler!=nullptr)
        if (_scheduler->isRunning())
        {
            _scheduler->stop();
            delete _scheduler;
            _scheduler=nullptr;
        }
    delete ui;
}
//---------------------------------------------------------------
void wndRadarInstanceEditor::init_script_tables()
{
    init_script_table(ui->tblScriptsInit, _radar_instance->get_init_scripts());

    init_script_table(ui->tblScriptsPostAcq, _radar_instance->get_postacquisition_scripts());
}
//---------------------------------------------------------------
void wndRadarInstanceEditor::init_script_table(QTableWidget* table, QVector<octaveScript_ptr> list)
{
    table->clear();
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(QStringList({"#","Script",""}));

    table->setRowCount(0);
    int row = 0;
    for (const auto& script : list)
    {
        if (script==nullptr)
            continue;
        table->setRowCount(row+1);
        QComboBox* cb = new QComboBox();

        fill_script_table_widget(cb, script);
        QTableWidgetItem* item = new QTableWidgetItem();
        item->setText(QString::number(row+1));
        table->setItem(row,0,item);
        table->setCellWidget(row,1,cb);
        cb->setCurrentText(script->get_name());

        if (script->isValid())
        {
            table->setItem(row,2,new QTableWidgetItem());
        }
        else
        {
            QTableWidgetItem* newItem = new QTableWidgetItem();
            newItem->setIcon(_icon);
            table->setItem(row,2,newItem);
        }

        if (table == ui->tblScriptsInit)
            connect(cb,&QComboBox::currentIndexChanged, this, &wndRadarInstanceEditor::cbInitScriptChanged);
        if (table == ui->tblScriptsPostAcq)
            connect(cb,&QComboBox::currentIndexChanged, this, &wndRadarInstanceEditor::cbPostacqScriptChanged);

        row++;
    }
    add_script_table_empty_row(table);
}
//---------------------------------------------------------------
void wndRadarInstanceEditor::fill_script_table_widget(QComboBox* cb,octaveScript_ptr script)
{
    cb->clear();
    cb->addItem("[None]");
    cb->addItem("[New..]");
    int row = 2;
    for (const auto& av_script: _available_scripts)
    {
        if (cb==nullptr) continue;
        cb->addItem(av_script->get_name());
        if (!(av_script->isValid()))
        {
            QBrush b (Qt::red);
            cb->setItemData(row, QBrush(Qt::red), Qt::ForegroundRole);
        }
        row++;
    }
}
//---------------------------------------------------------------
void wndRadarInstanceEditor::add_script_table_empty_row(QTableWidget* table)
{
    if (table==nullptr) return;
    int row = table->rowCount();
    table->setRowCount(row+1);
    // Add an extra row
    QComboBox* cb = new QComboBox();
    fill_script_table_widget(cb);


    if (table == ui->tblScriptsInit)
        connect(cb,&QComboBox::currentIndexChanged, this, &wndRadarInstanceEditor::cbInitScriptChanged);
    if (table == ui->tblScriptsPostAcq)
        connect(cb,&QComboBox::currentIndexChanged, this, &wndRadarInstanceEditor::cbPostacqScriptChanged);
    QTableWidgetItem* item = new QTableWidgetItem();
    item->setText(QString::number(row+1));
    table->setItem(row,0,item);
    table->setCellWidget(row,1,cb);
}
//---------------------------------------------------------------
octaveScript*   wndRadarInstanceEditor::load_new_script()
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
    _available_scripts = _project->get_available_scripts();
    if (script!=nullptr)
        _project->save_project_file();

    return script;
}
//---------------------------------------------------------------
void    wndRadarInstanceEditor::cbInitScriptChanged(int index)
{
    // Recast current parameter
    // Warning : changing the parameter class will result in loss of previous data

    octaveScript* script = nullptr;
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblScriptsInit->indexAt(w->pos()).row();

    if ((row<0)||(row>=ui->tblScriptsInit->rowCount())) return;

    if ((row == ui->tblScriptsInit->rowCount()-1)&&(index == 0)) return;
    QString script_name = ((QComboBox*)(sender()))->currentText();

    if (index==1)
    {
        script = load_new_script();
        if (script == nullptr) return;

        QVector<octaveScript*> scripts = _radar_instance->get_init_scripts();

        scripts.insert(row, script);
        _radar_instance->set_init_scripts(scripts);
        init_script_table(ui->tblScriptsInit, _radar_instance->get_init_scripts());

        return;
    }

    if (index==0)
    {
        QVector<octaveScript*> scripts = _radar_instance->get_init_scripts();
        scripts.remove(row);
        _radar_instance->set_init_scripts(scripts);
        init_script_table(ui->tblScriptsInit, _radar_instance->get_init_scripts());

        return;
    }

    script = (octaveScript*)(_project->get_child(script_name,DT_SCRIPT));
    if (script == nullptr)
    {
        QMessageBox::critical(this, tr("Error"), tr("Script not found in project"));
        return;
    }

    if (row == ui->tblScriptsInit->rowCount()-1)
    {
        QVector<octaveScript*> scripts = _radar_instance->get_init_scripts();
        scripts.insert(row, script);
        _radar_instance->set_init_scripts(scripts);

        add_script_table_empty_row(ui->tblScriptsInit);
    }
    else
    {
        _radar_instance->set_init_script(row, script);
    }

    if (script->isValid())
    {
        ui->tblScriptsInit->setItem(row,2,new QTableWidgetItem());
    }
    else
    {
        QTableWidgetItem* newItem = new QTableWidgetItem();
        newItem->setIcon(_icon);
        ui->tblScriptsInit->setItem(row,2,newItem);
    }
}

//---------------------------------------------------------------
void    wndRadarInstanceEditor::cbPostacqScriptChanged(int index)
{
    // Recast current parameter
    // Warning : changing the parameter class will result in loss of previous data

    octaveScript* script = nullptr;
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblScriptsPostAcq->indexAt(w->pos()).row();

    if ((row<0)||(row>=ui->tblScriptsPostAcq->rowCount())) return;

    if ((row == ui->tblScriptsPostAcq->rowCount()-1)&&(index == 0)) return;
    QString script_name = ((QComboBox*)(sender()))->currentText();

    if (index==1)
    {
        script = load_new_script();
        if (script == nullptr) return;

        QVector<octaveScript*> scripts = _radar_instance->get_postacquisition_scripts();

        scripts.insert(row, script);
        _radar_instance->set_postacquisition_scripts(scripts);
        init_script_table(ui->tblScriptsPostAcq, _radar_instance->get_postacquisition_scripts());

        return;
    }

    if (index==0)
    {
        QVector<octaveScript*> scripts = _radar_instance->get_postacquisition_scripts();
        scripts.remove(row);
        _radar_instance->set_postacquisition_scripts(scripts);
        init_script_table(ui->tblScriptsPostAcq, _radar_instance->get_postacquisition_scripts());

        return;
    }

    script = (octaveScript*)(_project->get_child(script_name,DT_SCRIPT));
    if (script == nullptr)
    {
        QMessageBox::critical(this, tr("Error"), tr("Script not found in project"));
        return;
    }
    if (row == ui->tblScriptsPostAcq->rowCount()-1)
    {
        QVector<octaveScript*> scripts = _radar_instance->get_postacquisition_scripts();
        scripts.insert(row, script);
        _radar_instance->set_postacquisition_scripts(scripts);

        add_script_table_empty_row(ui->tblScriptsPostAcq);
    }
    else
    {
        _radar_instance->set_postacquisition_script(row, script);
    }

    if (script->isValid())
    {
        ui->tblScriptsPostAcq->setItem(row,2,new QTableWidgetItem());
    }
    else
    {
        QTableWidgetItem* newItem = new QTableWidgetItem();
        newItem->setIcon(_icon);
        ui->tblScriptsPostAcq->setItem(row,2,newItem);
    }
}

//---------------------------------------------------------------
void wndRadarInstanceEditor::confirm()
{
    if (_radar_instance!=nullptr)
    {
        QString radarName = ui->leAliasName->text();
        if (radarName.isEmpty())
        {
            QMessageBox::critical(this,"Error","Name cannot be empty");
            return;
        }

        QFileInfo fi(radarName);
        if (fi.suffix()!="ard")
            radarName += ".ard";

        QString prev_name = _radar_instance->get_name();

        radarProject* root = _radar_instance->get_root();
        if (prev_name != radarName)
            if (root != nullptr)
            {
                if (root->get_child(radarName,DT_RADARDEVICE)!=nullptr)
                {
                    QMessageBox::critical(this,"Error","Device already defined");
                    return;
                }
            }
        bool add_new = false;
        if (prev_name != radarName)
        {
            radarInstance *new_radar;
            new_radar = new radarInstance(*_radar_instance);

            if (_backup == nullptr)
                _backup = new radarInstance (*_radar_instance);
            else
                (*_backup) = (*_radar_instance);
            _radar_instance = new_radar;
            connect_serial_updates();
            add_new = true;
        }
        _radar_instance->set_name(radarName);
        _radar_instance->set_filename(radarName);

        _radar_instance->set_device_name(ui->leAliasName->text());
        _radar_instance->set_fixed_id(ui->cbFixedId->isChecked());
        _radar_instance->set_fixed_port(ui->cbFixedPort->isChecked());
        _radar_instance->set_uid_string(ui->leRadarId->text());
        _radar_instance->set_expected_portname(ui->leRadarPort->text());

        if (add_new)
		{
			root->add_radar_instance(_radar_instance);
            if (!root->get_last_error().isEmpty())
            {
                QMessageBox::critical(this, "Error", root->get_last_error());
				(*_radar_instance) = (*_backup);
                return;
            }
            root->save_project_file();
        }
        _radar_instance->save_xml();
    }
}
//-----------------------------------------------------------
void wndRadarInstanceEditor::cancel()
{
    QMessageBox::StandardButton answer =
        QMessageBox::question(this,tr("Confirm"),tr("Do you want to save current device?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );

    if (answer == QMessageBox::Yes)
    {
        confirm();
        done(QMessageBox::Cancel);
        this->parentWidget()->close();
        return;
    }

    if (answer == QMessageBox::Cancel)
        return;

    if (answer == QMessageBox::No)
    {
        if (_backup!=nullptr)
            (*_radar_instance) = (*_backup);
    }

    done(QMessageBox::Cancel);
    this->parentWidget()->close();
}
//---------------------------------------------------------------
// Closing stuff
void wndRadarInstanceEditor::closeEvent(QCloseEvent *event)
{

    //cancel();
}
//-----------------------------------------------------------
void    wndRadarInstanceEditor::init_parameter_table()
{
    ui->tblParams->clear();
    ui->tblParams->setColumnCount(7);


    radarModule*            ref = _radar_instance->get_module();
    if (ref==nullptr)
    {
        QMessageBox::critical(this,"Error","Unspecified model for ptable fill");
        return;
    }

    QVector<radarParamPointer> params = _radar_instance->get_param_table();
    QVector<radarParamPointer> p_to_del;
    for (int row = 0; row < params.count(); row++)
    {
        radarParamPointer param = params[row];      // This is the actual value.
        radarParamPointer defpar= ref->get_param(param->get_name());
        if (defpar==nullptr)
        {
            QMessageBox::critical(this, "Error", QString("Parameter :")+param->get_name()+" does not have reference");
            p_to_del.append(param);
            continue;
        }
    }

    for (auto p : p_to_del)
    {
        _radar_instance->remove_param(p->get_name());
    }
    params = _radar_instance->get_param_table();

    ui->tblParams->setHorizontalHeaderLabels(QStringList({"Parameter","Default Value","Current Value","Current Value", "Exported to Octave","Alias", "Compound"}));
    connect(ui->tblParams, &QTableWidget::itemChanged, this, &wndRadarInstanceEditor::itemChanged);
    ui->tblParams->setRowCount(params.count());


   for (int row = 0; row < params.count(); row++)
   {
        radarParamPointer param = params[row];      // This is the actual value.
        radarParamPointer defpar= ref->get_param(param->get_name());
        param_to_table_row(row, param, defpar);
        update_read_value(row);
   }
}
//-----------------------------------------------------------
void    wndRadarInstanceEditor::param_to_table_row(int row, radarParamPointer current_param, radarParamPointer default_value)
{
    if (default_value==nullptr) return;
    if (current_param==nullptr) return;
    QTableWidgetItem* item = new QTableWidgetItem();
    item->setText(current_param->get_name());
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);

    ui->tblParams->setItem(row,COL_NAME,item);
    // Default value
    QVector<QVariant> data = default_value->value_to_variant();
    item = new QTableWidgetItem();
    QString val_text;

    if (default_value->get_type()==RPT_VOID)
    {
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        item->setText("void");
        ui->tblParams->setItem(row,COL_DEFAULT_VALUE,item);
    }
    else
    {
        if (default_value->get_type()==RPT_ENUM)
        {
            if (data.count()==0)
                val_text = default_value->get_io_type() == RPT_IO_INPUT ? "":"[ empty data ]";
            else
            {
                enumElem d0 = data[0].value<enumElem>();
                val_text = d0.first;
            }

        }
        else
        {
            if (data.count()==0)
                val_text = default_value->get_io_type() == RPT_IO_INPUT ? "":"[ empty data ]";
            else
            {
                QString val0 = (default_value->get_type() == RPT_INT8) ||  (default_value->get_type() == RPT_UINT8 )? QString::number(data[0].toInt()) : data[0].toString();
                val_text = data.count() == 1 ? val0 : QString("[") + val0 + "... ]";
            }
        }
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        item->setText(val_text);
        ui->tblParams->setItem(row,COL_DEFAULT_VALUE,item);
    }
    // Current value (editor)
    current_param_to_table(row, current_param);

    //
    item = new QTableWidgetItem();
    ui->tblParams->setItem(row,COL_READVALUE, item);
    update_read_value(row);
    // Current value (shown)

   // Exported to Octave
    QCheckBox* cb = new QCheckBox();
    cb->setChecked(current_param->is_linked_to_octave()
#ifndef EXPORT_VOID
                   && (current_param->get_type()!=RPT_VOID)
#endif
                   );
    connect(cb, &QCheckBox::checkStateChanged, this, &wndRadarInstanceEditor::linkToOctaveChanged);
    ui->tblParams->setCellWidget(row,COL_EXPORT_OCTAVE,cb);
#ifndef EXPORT_VOID
    cb->setEnabled(current_param->get_type() != RPT_VOID);
#endif

    // alias
    QTableWidgetItem* item_alias = new QTableWidgetItem();
    item_alias->setText(current_param->get_alias_octave_name());
#ifdef EXPORT_VOID
    if (!current_param->is_linked_to_octave())
#else
    if ((current_param->get_type()==RPT_VOID)||(!current_param->is_linked_to_octave()))
#endif
        item_alias->setFlags(item_alias->flags() ^ Qt::ItemIsEditable);

    ui->tblParams->setItem(row,COL_ALIAS, item_alias);

    // compound
    QCheckBox *cb_comp = new QCheckBox();
    cb_comp->setChecked(current_param->is_compound_name());
#ifndef EXPORT_VOID
    cb_comp->setEnabled((current_param->get_type()!=RPT_VOID)&&(current_param->is_linked_to_octave())&&(!current_param->get_alias_octave_name().isEmpty()));
#else
    cb_comp->setEnabled((current_param->is_linked_to_octave())&&(!current_param->get_alias_octave_name().isEmpty()));
#endif

    connect(cb_comp, &QCheckBox::checkStateChanged, this, &wndRadarInstanceEditor::compoundNameChanged);
    ui->tblParams->setCellWidget(row, COL_COMPOUND_NAME,cb_comp);

}


//-----------------------------------------------------------
void    wndRadarInstanceEditor::current_param_to_table(int row, radarParamPointer current_value)
{
    if (current_value==nullptr) return;

    RADARPARAMTYPE datatype = current_value->get_type();

    if (current_value->get_io_type() == RPT_IO_OUTPUT)
    {
        QPushButton* btnInquiry = new QPushButton();
        btnInquiry->setText("Inquiry");
        if (btnInquiry!=nullptr) ui->tblParams->setCellWidget(row,COL_SETVALUE,btnInquiry);
        connect(btnInquiry, SIGNAL(clicked()), this, SLOT(inquiry_parameter()));

        return;
    }

    if (current_value->is_scalar())
    {
		int imin = 0;
		int imax=255;
		switch (datatype)
        {
            case RPT_VOID:
            {
            QPushButton* btnSendCommand = ui->tblParams->cellWidget(row, COL_SETVALUE)==nullptr? new QPushButton() :
                                                  (QPushButton*)(ui->tblParams->cellWidget(row, COL_SETVALUE));

            btnSendCommand->setText("Send cmd");
            if (btnSendCommand!=nullptr) ui->tblParams->setCellWidget(row,COL_SETVALUE,btnSendCommand);
            connect(btnSendCommand, &QPushButton::clicked, this, &wndRadarInstanceEditor::send_command);

            }
			break;
            case RPT_UINT8:
            {
                imin = 0;
                imax = UCHAR_MAX;
                if (ui->tblParams->item(row,COL_SETVALUE)!=nullptr)
                {
                    ui->tblParams->item(row,COL_SETVALUE)->setText("");
                    ui->tblParams->item(row,COL_SETVALUE)->setFlags(ui->tblParams->item(row,COL_SETVALUE)->flags() ^ Qt::ItemIsEditable);
                }

				QSpinBox * spin_box = ui->tblParams->cellWidget(row, COL_SETVALUE)==nullptr? new QSpinBox(): (QSpinBox*)(ui->tblParams->cellWidget(row, COL_SETVALUE));

                if (current_value->has_min_max())
                {
                    imin = current_value->get_min().toInt();
                    imax = current_value->get_max().toInt();
                }
				spin_box->setMinimum(imin);
				spin_box->setMaximum(imax);
				QVector<QVariant> var_array = current_value->value_to_variant();
                if (var_array.size()>0)
					spin_box->setValue(var_array[0].toInt());
                else
					spin_box->setValue(imin);
				ui->tblParams->setCellWidget(row,COL_SETVALUE,spin_box);
                RADARPARAMIOTYPE  io_type = current_value->get_io_type();
				spin_box->setEnabled(io_type != RPT_IO_OUTPUT);

				if (io_type != RPT_IO_OUTPUT)
					connect(spin_box,&QSpinBox::valueChanged,this,&wndRadarInstanceEditor::spinBoxValueChanged);
            }
                break;
            case RPT_INT8:
            {
				imin = SCHAR_MIN;
                imax = SCHAR_MAX;
				if (ui->tblParams->item(row,COL_SETVALUE)!=nullptr)
				{
					ui->tblParams->item(row,COL_SETVALUE)->setText("");
					ui->tblParams->item(row,COL_SETVALUE)->setFlags(ui->tblParams->item(row,COL_SETVALUE)->flags() ^ Qt::ItemIsEditable);
				}

				QSpinBox * spin_box = ui->tblParams->cellWidget(row, COL_SETVALUE)==nullptr? new QSpinBox(): (QSpinBox*)(ui->tblParams->cellWidget(row, COL_SETVALUE));

				if (current_value->has_min_max())
				{
					imin = current_value->get_min().toInt();
					imax = current_value->get_max().toInt();
				}
				spin_box->setMinimum(imin);
				spin_box->setMaximum(imax);
				QVector<QVariant> var_array = current_value->value_to_variant();
				if (var_array.size()>0)
					spin_box->setValue(var_array[0].toInt());
				else
					spin_box->setValue(imin);
				ui->tblParams->setCellWidget(row,COL_SETVALUE,spin_box);
				RADARPARAMIOTYPE  io_type = current_value->get_io_type();
				spin_box->setEnabled(io_type != RPT_IO_OUTPUT);

				if (io_type != RPT_IO_OUTPUT)
					connect(spin_box,&QSpinBox::valueChanged,this,&wndRadarInstanceEditor::spinBoxValueChanged);
			}
                break;
            case RPT_UINT16:
            {
                imin = 0;
                imax = USHRT_MAX;
				if (ui->tblParams->item(row,COL_SETVALUE)!=nullptr)
				{
					ui->tblParams->item(row,COL_SETVALUE)->setText("");
					ui->tblParams->item(row,COL_SETVALUE)->setFlags(ui->tblParams->item(row,COL_SETVALUE)->flags() ^ Qt::ItemIsEditable);
				}

				QSpinBox * spin_box = ui->tblParams->cellWidget(row, COL_SETVALUE)==nullptr? new QSpinBox(): (QSpinBox*)(ui->tblParams->cellWidget(row, COL_SETVALUE));

				if (current_value->has_min_max())
				{
					imin = current_value->get_min().toInt();
					imax = current_value->get_max().toInt();
				}
				spin_box->setMinimum(imin);
				spin_box->setMaximum(imax);
				QVector<QVariant> var_array = current_value->value_to_variant();
				if (var_array.size()>0)
					spin_box->setValue(var_array[0].toInt());
				else
					spin_box->setValue(imin);
				ui->tblParams->setCellWidget(row,COL_SETVALUE,spin_box);
				RADARPARAMIOTYPE  io_type = current_value->get_io_type();
				spin_box->setEnabled(io_type != RPT_IO_OUTPUT);

				if (io_type != RPT_IO_OUTPUT)
					connect(spin_box,&QSpinBox::valueChanged,this,&wndRadarInstanceEditor::spinBoxValueChanged);
			}
                break;
            case RPT_INT16:
            {
                imin = SHRT_MIN;
                imax = SHRT_MAX;
				if (ui->tblParams->item(row,COL_SETVALUE)!=nullptr)
				{
					ui->tblParams->item(row,COL_SETVALUE)->setText("");
					ui->tblParams->item(row,COL_SETVALUE)->setFlags(ui->tblParams->item(row,COL_SETVALUE)->flags() ^ Qt::ItemIsEditable);
				}

				QSpinBox * spin_box = ui->tblParams->cellWidget(row, COL_SETVALUE)==nullptr? new QSpinBox(): (QSpinBox*)(ui->tblParams->cellWidget(row, COL_SETVALUE));

				if (current_value->has_min_max())
				{
					imin = current_value->get_min().toInt();
					imax = current_value->get_max().toInt();
				}
				spin_box->setMinimum(imin);
				spin_box->setMaximum(imax);
				QVector<QVariant> var_array = current_value->value_to_variant();
				if (var_array.size()>0)
					spin_box->setValue(var_array[0].toInt());
				else
					spin_box->setValue(imin);
				ui->tblParams->setCellWidget(row,COL_SETVALUE,spin_box);
				RADARPARAMIOTYPE  io_type = current_value->get_io_type();
				spin_box->setEnabled(io_type != RPT_IO_OUTPUT);

				if (io_type != RPT_IO_OUTPUT)
					connect(spin_box,&QSpinBox::valueChanged,this,&wndRadarInstanceEditor::spinBoxValueChanged);
			}
                break;
            case RPT_INT32:
            {
				imin = INT_MIN;
				imax = INT_MAX;
				if (ui->tblParams->item(row,COL_SETVALUE)!=nullptr)
				{
					ui->tblParams->item(row,COL_SETVALUE)->setText("");
					ui->tblParams->item(row,COL_SETVALUE)->setFlags(ui->tblParams->item(row,COL_SETVALUE)->flags() ^ Qt::ItemIsEditable);
				}

				QSpinBox * spin_box = ui->tblParams->cellWidget(row, COL_SETVALUE)==nullptr? new QSpinBox(): (QSpinBox*)(ui->tblParams->cellWidget(row, COL_SETVALUE));

				if (current_value->has_min_max())
				{
					imin = current_value->get_min().toInt();
					imax = current_value->get_max().toInt();
				}
				spin_box->setMinimum(imin);
				spin_box->setMaximum(imax);
				QVector<QVariant> var_array = current_value->value_to_variant();
				if (var_array.size()>0)
					spin_box->setValue(var_array[0].toInt());
				else
					spin_box->setValue(imin);
				ui->tblParams->setCellWidget(row,COL_SETVALUE,spin_box);
				RADARPARAMIOTYPE  io_type = current_value->get_io_type();
				spin_box->setEnabled(io_type != RPT_IO_OUTPUT);

				if (io_type != RPT_IO_OUTPUT)
					connect(spin_box,&QSpinBox::valueChanged,this,&wndRadarInstanceEditor::spinBoxValueChanged);
			}
                break;
            case RPT_UINT32:
            {
				imin = 0;
				imax = UINT_MAX;
				QSpinBox * spin_box = ui->tblParams->cellWidget(row, COL_SETVALUE)==nullptr? new QSpinBox(): (QSpinBox*)(ui->tblParams->cellWidget(row, COL_SETVALUE));

				if (current_value->has_min_max())
				{
					imin = current_value->get_min().toInt();
					imax = current_value->get_max().toInt();
				}
				spin_box->setMinimum(imin);
				spin_box->setMaximum(imax);
				QVector<QVariant> var_array = current_value->value_to_variant();
				if (var_array.size()>0)
					spin_box->setValue(var_array[0].toInt());
				else
					spin_box->setValue(imin);
				ui->tblParams->setCellWidget(row,COL_SETVALUE,spin_box);
				RADARPARAMIOTYPE  io_type = current_value->get_io_type();
				spin_box->setEnabled(io_type != RPT_IO_OUTPUT);

				if (io_type != RPT_IO_OUTPUT)
					connect(spin_box,&QSpinBox::valueChanged,this,&wndRadarInstanceEditor::spinBoxValueChanged);
            }
				break;
			case RPT_FLOAT:
            case RPT_STRING:
            {
                if (ui->tblParams->cellWidget(row,COL_SETVALUE )!=nullptr)
                    ui->tblParams->removeCellWidget(row, COL_SETVALUE);

                QTableWidgetItem* item = ui->tblParams->currentItem() == nullptr ? new QTableWidgetItem() :
                                             ui->tblParams->currentItem();

                QVector<QVariant> vals = current_value->value_to_variant();
                if (vals.count()>0)
                    item->setText(vals[0].toString());
                ui->tblParams->setItem(row,COL_SETVALUE,item);
                RADARPARAMIOTYPE  io_type = current_value->get_io_type();
               if (io_type == RPT_IO_OUTPUT)
                   item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                break;
            }
            case RPT_UNDEFINED:
            {
                if (ui->tblParams->cellWidget(row,COL_SETVALUE )!=nullptr)
                    ui->tblParams->removeCellWidget(row, COL_SETVALUE);

                QTableWidgetItem* item = ui->tblParams->currentItem() == nullptr ? new QTableWidgetItem() :
                                             ui->tblParams->currentItem();

                item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                item->setText("[ERROR : Undefined type]");
                ui->tblParams->setItem(row,COL_SETVALUE,item);
                break;
            }
            case RPT_ENUM:
            {
                if (ui->tblParams->item(row,COL_SETVALUE)!=nullptr)
                {
                    ui->tblParams->item(row,COL_SETVALUE)->setText("");
                    ui->tblParams->item(row,COL_SETVALUE)->setFlags(ui->tblParams->item(row,COL_SETVALUE)->flags() ^ Qt::ItemIsEditable);
                }
                QComboBox* cb = ui->tblParams->cellWidget(row, COL_SETVALUE)==nullptr? new QComboBox() :
                                    (QComboBox*)(ui->tblParams->cellWidget(row, COL_SETVALUE));
                std::shared_ptr<radarParameter<enumElem>> rpenum = std::dynamic_pointer_cast<radarParameter<enumElem>>(current_value);
                int currentIndex=-1;
                QVector<QVariant> def = current_value->value_to_variant();
                enumElem rpdef;
                if (def.count() > 0)
                    rpdef = def[0].value<enumElem>();

                radarEnum available = rpenum->get_list_available_values();

                for (int n=0; n < available.count(); n++)
                {
                    enumElem pair = available[n];
                    cb->addItem(pair.first);

                    if ((pair==rpdef)&&(def.count()==1))
                        currentIndex=n;
                }
                if (currentIndex>=0)
                    cb->setCurrentIndex(currentIndex);
                RADARPARAMIOTYPE  io_type = current_value->get_io_type();
                cb->setEnabled(io_type != RPT_IO_OUTPUT);
                ui->tblParams->setCellWidget(row,COL_SETVALUE,cb);
                connect(cb, &QComboBox::currentIndexChanged,this, &wndRadarInstanceEditor::cbEnumIndexChange);
            }
        } // switch
    } // if scalar
    else
    {
        RADARPARAMTYPE datatype = current_value->get_type();
        switch (datatype)
        {
            case RPT_VOID:
            {
            QPushButton* btnSendCommand = ui->tblParams->cellWidget(row, COL_SETVALUE)==nullptr? new QPushButton() :
                                              (QPushButton*)(ui->tblParams->cellWidget(row, COL_SETVALUE));

                btnSendCommand->setText("Send cmd");
                if (btnSendCommand!=nullptr) ui->tblParams->setCellWidget(row,COL_SETVALUE,btnSendCommand);
                connect(btnSendCommand, &QPushButton::clicked, this, &wndRadarInstanceEditor::send_command);
            }
            break;
        case RPT_UNDEFINED:
        {
                QTableWidgetItem* item = ui->tblParams->currentItem() == nullptr ? new QTableWidgetItem() :
                                             ui->tblParams->currentItem();
            if (ui->tblParams->cellWidget(row,COL_SETVALUE )!=nullptr)
                ui->tblParams->removeCellWidget(row, COL_SETVALUE);

            if (item!=nullptr) item->setFlags(item->flags() ^ Qt::ItemIsEditable);
            item->setText("[ERROR: Undefined type]");
            ui->tblParams->setItem(row,COL_SETVALUE,item);
        }
        break;
            case RPT_INT8:
            case RPT_UINT8:
            case RPT_INT16:
            case RPT_UINT16:
            case RPT_INT32:
            case RPT_UINT32:
            case RPT_FLOAT:
            case RPT_STRING:
            {
                QTableWidgetItem* item = ui->tblParams->currentItem() == nullptr ? new QTableWidgetItem() :
                                             ui->tblParams->currentItem();
                if (ui->tblParams->cellWidget(row,COL_SETVALUE )!=nullptr)
                    ui->tblParams->removeCellWidget(row, COL_SETVALUE);

                if (item!=nullptr) item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                QVector<QVariant> vals =  current_value->value_to_variant();
                QString val("[");
                if (vals.count()>1)
                {
                    if ((current_value->get_type() == RPT_INT8)||(current_value->get_type()==RPT_UINT8))
                        val+=QString::number(vals[0].toInt()) + " , "+QString::number(vals[1].toInt());
                    else
                        val+=vals[0].toString()+" , "+vals[1].toString();
                }
                val+="...]";
                item->setText(val);
                ui->tblParams->setItem(row,COL_SETVALUE,item);
            }
            break;
        case RPT_ENUM:
        {
            QTableWidgetItem* item = ui->tblParams->currentItem() == nullptr ? new QTableWidgetItem() :
                                         ui->tblParams->currentItem();

            if (ui->tblParams->cellWidget(row,COL_SETVALUE )!=nullptr)
                ui->tblParams->removeCellWidget(row, COL_SETVALUE);


            if (item!=nullptr) item->setFlags(item->flags() ^ Qt::ItemIsEditable);
            QVector<QVariant> vals =  current_value->value_to_variant();
            QString val("[");
            if (vals.count()>1)
            {
                val+=vals[0].value<enumElem>().first+ ", "+vals[1].value<enumElem>().first;
            }
            val+="...]";
            item->setText(val);
            ui->tblParams->setItem(row,COL_SETVALUE,item);
        }
        break;
         } // switch
    } // if ! scalar
    //update_read_value(row);
}
//-----------------------------------------------------------
void wndRadarInstanceEditor::spinBoxValueChanged(int value)
{
	if (_b_transmitting) return;
	int row = -1;
	QWidget *w = qobject_cast<QWidget *>(sender());
	if(w)
		row = ui->tblParams->indexAt(w->pos()).row();

	if (row<0) return;

	QSpinBox* spin_box = (QSpinBox*)ui->tblParams->cellWidget(row, COL_SETVALUE);

	if (spin_box == nullptr) return;

	radarParamPointer param = _radar_instance->get_param(row);

	if (param->get_status()==RPS_IDLE)
		param->set_status(RPS_MODIFIED);

	octave_value val(value);
	_b_transmitting = true;
	if (param->is_valid(QVariant::fromValue<int>(value)))
		_radar_instance->set_param_value(param,val,true, true);

	//update_serial_output();
	_b_transmitting = false;

}
//-----------------------------------------------------------
void wndRadarInstanceEditor::paramSliderReleased()
{
    if (_b_transmitting) return;
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParams->indexAt(w->pos()).row();

    if (row<0) return;

    QSlider* slider = (QSlider*)ui->tblParams->cellWidget(row, COL_SETVALUE);

    if (slider == nullptr) return;

    radarParamPointer param = _radar_instance->get_param(row);

    if (param->get_status()==RPS_IDLE)
        param->set_status(RPS_MODIFIED);

    int value = slider->sliderPosition();
    octave_value val(value);
    _b_transmitting = true;
    if (param->is_valid(QVariant::fromValue<int>(value)))
        _radar_instance->set_param_value(param,val,true, true);

    //update_serial_output();
    _b_transmitting = false;
}

//-----------------------------------------------------------
void wndRadarInstanceEditor::cbEnumIndexChange(int index)
{
    if (_b_transmitting) return;
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParams->indexAt(w->pos()).row();

    if (row<0) return;

    QComboBox* cb = (QComboBox*)(ui->tblParams->cellWidget(row,COL_SETVALUE));
    if (cb==nullptr) return;
    QString value  = cb->currentText();

    std::shared_ptr<radarParameter<enumElem>> rpEnum =  std::dynamic_pointer_cast<radarParameter<enumElem>>(_radar_instance->get_param(row));
    if (rpEnum==nullptr) return;
    radarEnum avail = rpEnum->get_list_available_values();
    uint8NDArray val;
    val.resize(dim_vector({1,1}));

    for (auto& item : avail)
    {
        if (item.first == value)
        {
            val(0) = item.second; break;
        }
    }
    octave_value oval; oval = val;
    _b_transmitting = true;
    radarParamPointer param = _radar_instance->get_param(row);
    if (param->get_status()==RPS_IDLE)
        param->set_status(RPS_MODIFIED);

    _radar_instance ->set_param_value(row, oval,true, true);

    //update_serial_output();
    _b_transmitting = false;
}
//------------------------------------------------------------
void    wndRadarInstanceEditor::compoundNameChanged(Qt::CheckState newCompState)
{
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParams->indexAt(w->pos()).row();

    if (row<0) return;

    radarParamPointer param = _radar_instance->get_param(row);
    if (param==nullptr) return;
    QCheckBox* cb = (QCheckBox*)ui->tblParams->cellWidget(row, COL_COMPOUND_NAME);
    if (cb==nullptr) return;
    param->set_compound_name(cb->isChecked());
}

//-----------------------------------------------------------
void    wndRadarInstanceEditor::linkToOctaveChanged(Qt::CheckState newLinkState)
{
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParams->indexAt(w->pos()).row();

    if (row<0) return;

    radarParamPointer param = _radar_instance->get_param(row);
    if (param==nullptr) return;
    QCheckBox* cb = (QCheckBox*)ui->tblParams->cellWidget(row, COL_EXPORT_OCTAVE);
    if (cb==nullptr) return;
    bool linked =cb->checkState() == Qt::Checked
#ifndef EXPORT_VOID
                    && (param->get_type()!=RPT_VOID)
#endif
        ;

    QTableWidgetItem* item = ui->tblParams->item(row,COL_ALIAS);
    if (linked)
    {
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setText(param->get_alias_octave_name());
    }
    else
    {
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        item->setText("");
    }

    linked = linked && (!param->get_alias_octave_name().isEmpty());
    QCheckBox* cbcomp = (QCheckBox*)(ui->tblParams->cellWidget(row,COL_COMPOUND_NAME));
    cbcomp->setEnabled(linked);
    cbcomp->setChecked(linked && param->is_compound_name());
}

//-----------------------------------------------------------
/*
void    wndRadarInstanceEditor::currentPlotChanged(int index)
{
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParams->indexAt(w->pos()).row();

    if (row<0) return;

    radarParamPointer param = _radar_instance->get_param(row);
    if (param==nullptr) return;

    QComboBox* plotcb = (QComboBox*)ui->tblParams->cellWidget(row,COL_PLOTTYPE);
    if (plotcb==nullptr) return;

    if (index==0)
        param->set_plotted(false);
    else
    {
        param->set_plotted(true);
        param->set_plot_type((PLOT_TYPE)(index-1));
    }
}*/

//-----------------------------------------------------------
void    wndRadarInstanceEditor::itemChanged(QTableWidgetItem* item)
{
    if (_b_transmitting) return;
    int row = item->row();
    int col = item->column();
        if (row<0) return;
    if (col < 0) return;

    radarParamPointer param = _radar_instance->get_param(row);
    if (param==nullptr) return;

    if (col==COL_SETVALUE)
    {
        if (param->is_scalar())
        {
            QVector<QVariant> val;
            val.resize(1);
            val[0] = item->text();

            RP_STATUS param_stat = param->get_status();
            if (param_stat == RPS_IDLE)
                param->set_status(RPS_MODIFIED);
            else
            {
                param->set_status(RPS_IDLE);
                //qDebug() << "\n**** " << param->get_name() << " modification overlapping with current transfer. Discarded";
            }

            if (param->get_status()==RPS_MODIFIED)
                _radar_instance->set_param_value(param,val,true, true);
        }
    }

    if (col==COL_ALIAS)
    {
        QString alias = item->text();
        param->set_alias_octave_name(alias);
        QCheckBox* cb = (QCheckBox*)(ui->tblParams->cellWidget(row, COL_COMPOUND_NAME));
        if (cb==nullptr)
            return;
        cb->setEnabled(!alias.isEmpty());

    }
}
//-----------------------------------------------------------
void wndRadarInstanceEditor::update_connect_btn()
{
    if (_radar_instance==nullptr)
        return;
    if (_radar_instance->is_connected())
    {
        ui->btnConnect->setText("Disconnect");
        ui->leCustomString->setEnabled(true);
        ui->btnSendCustomString->setEnabled(true);
    }

    if (!_radar_instance->is_connected())
    {
        ui->btnConnect->setText("Connect");
        ui->leCustomString->setEnabled(false);
        ui->btnSendCustomString->setEnabled(false);
    }
}

//-----------------------------------------------------------
void wndRadarInstanceEditor::connect_radar()
{
    if (_radar_instance==nullptr)
        return;

    if (_radar_instance->is_connected())
    {
        _radar_instance->disconnect();
        if (_scheduler!=nullptr)
        {
            if (_scheduler->isRunning())
                _scheduler->stop();
        }

    }
    else
    {
        // Put temp data into current device
        _radar_instance->set_fixed_id(ui->cbFixedId->isChecked());
        _radar_instance->set_fixed_port(ui->cbFixedPort->isChecked());
        if (_radar_instance->fixed_port())
            _radar_instance->set_expected_portname(ui->leRadarPort->text());
        if (_radar_instance->fixed_id())
            _radar_instance->set_uid(QByteArray::fromHex(ui->leRadarId->text().toLatin1()));

        // Since we want to have a feedback of the serial, we init each param
        QVector<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
        if (_radar_instance->fixed_port())
        {
            for (auto& port: ports)
            {
                if (port.portName() == _radar_instance->get_expected_portname())
                {
                    _radar_instance->set_port(port);
                    break;
                }
            }
       }
        else
            _radar_instance->set_ports(ports);
    }
}
//-----------------------------------------------------------
void    wndRadarInstanceEditor::scan()
{
    if (_radar_instance==nullptr) return;
    if (_radar_instance->get_module()==nullptr) return;
    QVector<projectItem*> radar_modules;
    radar_modules.append(_radar_instance->get_module());

    wndScanModules* scanWnd = new wndScanModules(radar_modules,_radar_instance->get_root(), this);
    scanWnd->exec();
}

//-----------------------------------------------------------
void    wndRadarInstanceEditor::send_custom_string()
{
    QString text = ui->leCustomString->text();
    QByteArray data_tx = QByteArray::fromHex(text.toLatin1());
    _last_operation_result =  _radar_instance->transmit_custom(data_tx) ? "" : " .... error during transmission";
    //update_serial_output();


}
//-----------------------------------------------------------
void    wndRadarInstanceEditor::update_read_value(int row)
{
    if (_radar_instance == nullptr) return;
    QTableWidgetItem* item = ui->tblParams->item(row, COL_READVALUE);
    if (item==nullptr) return;
    radarParamPointer param = _radar_instance->get_param(row);
    if (param==nullptr) return;
     item->setFlags(item->flags() ^ Qt::ItemIsEditable);

    RADARPARAMTYPE datatype = param->get_type();

    if (param->is_scalar())
    {
        switch (datatype)
        {
        case RPT_VOID:
            item->setText("[void]");
            break;
        case RPT_UINT8:
        case RPT_INT8:
        case RPT_UINT16:
        case RPT_INT16:
        case RPT_INT32:
        case RPT_UINT32:
        case RPT_FLOAT:
        case RPT_STRING:
        {
            QVector<QVariant> value = param->value_to_variant();
            QVariant v0 = value.count()>=1 ? value[0] : QVariant();
            QString pString = (datatype == RPT_UINT8 || datatype == RPT_INT8)? QString::number(v0.toInt()) : v0.toString();
            item->setText(pString);            
            break;
        }
        case RPT_UNDEFINED:
        {
            item->setText("[ERROR : Undefined type]");
            ui->tblParams->setItem(row,COL_SETVALUE,item);
            item->setFlags(item->flags() ^ Qt::ItemIsEditable);
            break;
        }
        case RPT_ENUM:
        {
            std::shared_ptr<radarParameter<enumElem>> rpenum = std::dynamic_pointer_cast<radarParameter<enumElem>>(param);
            QVector<QVariant> def = param->value_to_variant();
            enumElem rpdef;
            if (def.count() == 1)
                rpdef = def[0].value<enumElem>();
            item->setFlags(item->flags() ^ Qt::ItemIsEditable);
            item->setText(rpdef.first);
        }
        } // switch
    } // if scalar
    else
    {
        RADARPARAMTYPE datatype = param->get_type();
        switch (datatype)
        {
        case RPT_VOID:
        {
            item->setText("[void]");
        }
        break;
        case RPT_UNDEFINED:
        {
            item->setText("[ERROR: Undefined type]");

        }
        break;
        case RPT_INT8:
        case RPT_UINT8:
        case RPT_INT16:
        case RPT_UINT16:
        case RPT_INT32:
        case RPT_UINT32:
        case RPT_FLOAT:
        case RPT_STRING:
        {
            QVector<QVariant> vals =  param->value_to_variant();
            QString val("[");
            if (vals.count()>1)
            {
                QString string1 = (datatype == RPT_UINT8 || datatype == RPT_INT8) ?   QString::number(param->value_to_variant()[0].toUInt()) : param->value_to_variant()[0].toString();
                QString string2 = (datatype == RPT_UINT8 || datatype == RPT_INT8) ?   QString::number(param->value_to_variant()[1].toUInt()) : param->value_to_variant()[1].toString();

                val+=string1+" , "+string2;
            }
            val+="...]";
            item->setText(val);
        }
        break;
        case RPT_ENUM:
        {
            QVector<QVariant> vals =  param->value_to_variant();
            QString val("[");
            if (vals.count()>1)
            {
                val+=vals[0].value<enumElem>().first+ ", "+vals[1].value<enumElem>().first;
            }
            val+="...]";
            item->setText(val);
           }
        break;
        } // switch
    } // if ! scalar


}
//----------------------------------------------------
void    wndRadarInstanceEditor::init()
{
    if (_radar_instance==nullptr) return;
    _radar_instance->init();

}

//----------------------------------------------------
void    wndRadarInstanceEditor::clearSerialOutput()
{
    ui->teSerialOutput->clear();

}
//----------------------------------------------------
void    wndRadarInstanceEditor::run()
{
    if (_scheduler == nullptr)
    {
        _scheduler = new opScheduler(_radar_instance->get_workspace()->data_interface());
        _scheduler->set_policy_on_error(HALT_ALL);
        _scheduler->set_policy_on_timeout(HALT_ON_TIMEOUT);
        _scheduler->add_radar(_radar_instance);

        connect(_scheduler, &opScheduler::running,                  this, &wndRadarInstanceEditor::scheduler_running);
        connect(_scheduler, &opScheduler::halted,                   this, &wndRadarInstanceEditor::scheduler_halted);
        connect(_scheduler, &opScheduler::connection_done,          this, &wndRadarInstanceEditor::connection_done);
        connect(_scheduler, &opScheduler::connection_done_all,      this, &wndRadarInstanceEditor::connection_done_all);
        connect(_scheduler, &opScheduler::connection_error,         this, &wndRadarInstanceEditor::connection_error);
        connect(_scheduler, &opScheduler::init_done,                this, &wndRadarInstanceEditor::init_done);
        connect(_scheduler, &opScheduler::init_done_all,            this, &wndRadarInstanceEditor::init_done_all);
        connect(_scheduler, &opScheduler::init_error,               this, &wndRadarInstanceEditor::init_error);
        connect(_scheduler, &opScheduler::postacquisition_error,    this, &wndRadarInstanceEditor::postacquisition_error);
        connect(_scheduler, &opScheduler::postacquisition_done_all, this, &wndRadarInstanceEditor::postacquisition_done_all);

        _scheduler->run();
        return;
    }

    if (_scheduler->isRunning())
    {
        // Here we may create different radar_devices (confirm
        _scheduler->stop();
//        delete _scheduler;
//        _scheduler = nullptr;
    }
    else
        _scheduler->run();


}

//----------------------------------------------------
void    wndRadarInstanceEditor::exportToOctave()
{
    _radar_instance->export_to_octave();

}
//----------------------------------------------------
void    wndRadarInstanceEditor::importFromOctave()
{
    _radar_instance->import_from_octave();
    init_parameter_table();
}

//----------------------------------------------------
void    wndRadarInstanceEditor::variable_updated(const std::string& varname)
{
    if (varname.empty()) return;
    if (_radar_instance==nullptr) return;
    octavews *ws = _project == nullptr? nullptr : _project->get_workspace();
    if (ws == nullptr) return;

    octave_value val = ws->var_value(varname);

    for (auto& param: _radar_instance->get_param_table())
    {
        if (_radar_instance->get_mapped_name(param).toStdString() == varname)
        {
            QString pure_name = param->get_name();
            for (int n=0; n < ui->tblParams->rowCount(); n++)
            {
                if (ui->tblParams->item(n,COL_NAME)->text()==pure_name)
                {
                    current_param_to_table(n,param);
                    update_read_value(n);
                    return;
                }
            }
        }
    }

}
//----------------------------------------------------
void    wndRadarInstanceEditor::variables_updated(const std::set<std::string>& varlist)
{
    for (auto &var : varlist)
        variable_updated(var);

}
//----------------------------------------------------
void    wndRadarInstanceEditor::send_command()
{
    if (_radar_instance==nullptr)
        return;
    if (_b_transmitting) return;
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParams->indexAt(w->pos()).row();

    radarParamPointer param = _radar_instance->get_param(row);
    if (param==nullptr) return;
    _b_transmitting = true;
    _radar_instance->transmit_command(param);
    _b_transmitting = false;
    //update_serial_output();
}
//----------------------------------------------------
void    wndRadarInstanceEditor::inquiry_parameter()
{
    if (_radar_instance==nullptr)
        return;
    if (_b_transmitting) return;


    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblParams->indexAt(w->pos()).row();

    radarParamPointer param = _radar_instance->get_param(row);
    if (param==nullptr) return;

    _b_transmitting = true;
    _radar_instance->inquiry_parameter(param);
    _b_transmitting = false;
    update_read_value(row);
    //update_serial_output();

}
//----------------------------------------------------
void    wndRadarInstanceEditor::connect_serial_updates()
{
    if (_radar_instance==nullptr) return;
    connect(_radar_instance,&radarInstance::connection_done,     this, &wndRadarInstanceEditor::update_connect_btn);
    connect(_radar_instance,&radarInstance::disconnection,       this, &wndRadarInstanceEditor::update_connect_btn);
    connect(_radar_instance,&radarInstance::new_read_completed,  this, &wndRadarInstanceEditor::rx_updated);
    connect(_radar_instance,&radarInstance::new_write_completed, this, &wndRadarInstanceEditor::tx_updated);
    connect(_radar_instance,&radarInstance::rx_timeout,          this, &wndRadarInstanceEditor::rx_timeout);
    connect(_radar_instance,&radarInstance::tx_timeout,          this, &wndRadarInstanceEditor::tx_timeout);
    connect(_radar_instance,&radarInstance::serial_error_out,    this, &wndRadarInstanceEditor::serial_error);
    connect(_radar_instance,&radarInstance::param_updated,       this, &wndRadarInstanceEditor::radar_param_update_byserial);
}

//----------------------------------------------------
void    wndRadarInstanceEditor::tx_updated(const QByteArray& tx)
{
    if (!ui->cbSerialEcho->isChecked()) return;
    QString tx_string = tx.toHex(' ');
    ui->teSerialOutput->append(QString("--->\t")+tx_string);
}
//----------------------------------------------------
void    wndRadarInstanceEditor::rx_updated(const QByteArray& rx)
{
    if (!ui->cbSerialEcho->isChecked()) return;
    QString rx_string = rx.toHex(' ');
    ui->teSerialOutput->append(QString("<---\t")+rx_string);
    ui->teSerialOutput->update();
}
//----------------------------------------------------
void    wndRadarInstanceEditor::tx_timeout(const QString& strerror)
{
    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::DemiBold );
    ui->teSerialOutput->setTextColor( QColor( "red" ) );
    ui->teSerialOutput->append( strerror );
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void    wndRadarInstanceEditor::rx_timeout(const QString& strerror)
{
    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::DemiBold );
    ui->teSerialOutput->setTextColor( QColor( "red" ) );
    ui->teSerialOutput->append( strerror );
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );

}
//----------------------------------------------------
void    wndRadarInstanceEditor::serial_error(const QString& strerror)
{
    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::Bold );
    ui->teSerialOutput->setTextColor( QColor( "red" ) );
    ui->teSerialOutput->append( strerror );
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );

}

//----------------------------------------------------
void wndRadarInstanceEditor::radar_param_update_byserial(radarParamPointer ptr)
{
    _b_transmitting = true;

    for (int row = 0; row < ui->tblParams->rowCount(); row++)
    {
        if (ui->tblParams->item(row,COL_NAME)->text()==ptr->get_name())
        { update_read_value(row); break; }
    }

    _b_transmitting = false;

}
//----------------------------------------------------
void    wndRadarInstanceEditor::scheduler_running()
{
    ui->btnRun->setText("Stop");
}
//----------------------------------------------------
void    wndRadarInstanceEditor::scheduler_halted()
{
    ui->btnRun->setText("Run");
}

//----------------------------------------------------
void    wndRadarInstanceEditor::connection_done(radarInstance* device)
{
    if (device==nullptr) return;
    // append
    ui->teSerialOutput->setTextColor( QColor( "green" ) );
    ui->teSerialOutput->append( tr("Radar: ")+device->get_device_name()+tr(" connected on port :")+device->get_current_portname() );
    // restore
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void    wndRadarInstanceEditor::connection_done_all()
{
    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::Bold );
    ui->teSerialOutput->setTextColor( QColor( "green" ) );
    ui->teSerialOutput->append( tr("All devices connected") );
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void    wndRadarInstanceEditor::connection_error(radarInstance* device)
{
    if (device==nullptr) return;
    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::Bold );
    ui->teSerialOutput->setTextColor( QColor( "red" ) );
    ui->teSerialOutput->append( tr("Error while connecting   radar ")+device->get_device_name());
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void    wndRadarInstanceEditor::init_done(radarInstance* device)
{
    if (device==nullptr) return;
    // append
    ui->teSerialOutput->setTextColor( QColor( "green" ) );
    ui->teSerialOutput->append( tr("Radar : ")+device->get_device_name()+tr(" initialized") );
    // restore
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void    wndRadarInstanceEditor::init_done_all()
{
    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::Bold );
    ui->teSerialOutput->setTextColor( QColor( "green" ) );
    ui->teSerialOutput->append( tr("All devices initialized") );
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void    wndRadarInstanceEditor::init_error(radarInstance* device)
{
    if (device==nullptr) return;
    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::Bold );
    ui->teSerialOutput->setTextColor( QColor( "red" ) );
    ui->teSerialOutput->append( tr("Error while initializing radar ")+device->get_device_name());
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void wndRadarInstanceEditor::postacquisition_error(radarInstance* device)
{
    if (device==nullptr) return;
    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::Bold );
    ui->teSerialOutput->setTextColor( QColor( "red" ) );
    ui->teSerialOutput->append( tr("Error during pre acquisition phase at ")+device->get_device_name());
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void wndRadarInstanceEditor::preacquisition_error(radarInstance* device)
{
    if (device==nullptr) return;
    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::Bold );
    ui->teSerialOutput->setTextColor( QColor( "red" ) );
    ui->teSerialOutput->append( tr("Error during post acquisition phase at ")+device->get_device_name());
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void wndRadarInstanceEditor::scheduler_timing_error()
{

    int fw = ui->teSerialOutput->fontWeight();
    // append
    ui->teSerialOutput->setFontWeight( QFont::Bold );
    ui->teSerialOutput->setTextColor( QColor( "red" ) );
    ui->teSerialOutput->append( tr("Frame rate cannot be guaranteed "));
    // restore
    ui->teSerialOutput->setFontWeight( fw );
    ui->teSerialOutput->setTextColor( QColor("grey") );
}
//----------------------------------------------------
void wndRadarInstanceEditor::postacquisition_done_all()
{
    // Here we should update the relevant graphs after acquisition
}
//----------------------------------------------------
void wndRadarInstanceEditor::preacquisition_done_all()
{
    // Here we should update the relevant graphs pre acquisition
}

