/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "wndscheduler.h"
#include "ui_wndscheduler.h"
#include "../scheduler/opscheduler.h"

#include <QCheckBox>
#include <QPixmap>
#include <QMessageBox>

#define COL_NAME            0
#define COL_ENABLED         1
#define COL_CONNECTION      2
#define COL_STATUS          3

extern octaveInterface         *interfaceData;
wndScheduler::wndScheduler(QWidget *parent, radarProject* root, opScheduler* opsched)
    : QDialog(parent)
    , ui(new Ui::wndScheduler)
    , _root(root)
    , _scheduler(opsched)
    , _b_new(false)
{
    ui->setupUi(this);

    connect(ui->btnSave,    &QPushButton::clicked, this, &wndScheduler::save);    
    connect(ui->btnDone,    &QPushButton::clicked, this, &wndScheduler::done_btn);
    connect(ui->btnStart,   &QPushButton::clicked, this, &wndScheduler::start);
    connect(ui->btnStop,    &QPushButton::clicked, this, &wndScheduler::stop);

    if (_scheduler==nullptr)
    {
        _b_new = true;
        _scheduler = new opScheduler(interfaceData, nullptr);
        _scheduler->set_temporary_project(_root);

    }
    connect_scheduler_signals();

    _pixConnected   = QPixmap(":/icons/link-hyperlink-icon.png").scaled(24,24, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    _pixDisconnected= QPixmap(":/icons/broken-link-icon.png").scaled(24,24, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    _pixRunning     = QPixmap(":/icons/business-management-icon.png").scaled(24,24, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    _pixHalted      = QPixmap(":/icons/stop-road-sign-icon.png").scaled(24,24, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Init UI with scheduler data
    ui->leSchedulerName->setText(_scheduler->get_name());
    ui->sbSerialTimeout->setValue(_scheduler->get_serial_timeout());
    ui->sbTimeout->setValue(_scheduler->get_cycle_time());
    ui->cbErrorHandling->setCurrentIndex(_scheduler->get_policy_on_error());
    ui->cbTimeOutHandling->setCurrentIndex(_scheduler->get_policy_on_timeout());

    init_table();
}
//--------------------------------
void wndScheduler::connect_scheduler_signals()
{
    connect(_scheduler, &opScheduler::running,                  this, &wndScheduler::running);
    connect(_scheduler, &opScheduler::halted,                   this, &wndScheduler::halted);
    connect(_scheduler, &opScheduler::connection_done,          this, &wndScheduler::connection_done);
    connect(_scheduler, &opScheduler::connection_error,         this, &wndScheduler::connection_error);
    connect(_scheduler, &opScheduler::init_error,               this, &wndScheduler::init_error);
    connect(_scheduler, &opScheduler::init_done,                this, &wndScheduler::init_done);
    connect(_scheduler, &opScheduler::postacquisition_error,    this, &wndScheduler::postacquisition_error);
    connect(_scheduler, &opScheduler::postacquisition_done,     this, &wndScheduler::postacquisition_done);
    connect(_scheduler, &opScheduler::timeout_error,            this, &wndScheduler::timeout_error);
    connect(_scheduler, &opScheduler::timing_ok,                this, &wndScheduler::timing_ok);
    connect(_scheduler, &opScheduler::other_scheduler_running,  this, &wndScheduler::cannot_start);

}
//--------------------------------
wndScheduler::~wndScheduler()
{
    if (_b_new) delete _scheduler;
    delete ui;
}

//--------------------------------
void    wndScheduler::save()
{
    _b_save_ok = false;

    if (_scheduler== nullptr) return;
    if (_root == nullptr) return;

    QString name = ui->leSchedulerName->text();
    if (name.isEmpty())
    {
        QMessageBox::critical(this,"Error","Provide a name for the scheduler");
    }

    QFileInfo fi(name);
    if (fi.suffix()!="ars")
        name += ".ars";

    radarProject* root = _scheduler->get_root();
    QString prev_name  = _scheduler->get_name();
    // We are saving to a new scheduler
    if (name!=_scheduler->get_name())
    {
        if (root != nullptr)
        {
            if (root->get_child(name,DT_SCHEDULER)!=nullptr)
            {
                QMessageBox::critical(this,"Error","Device already defined");
                return;
            }
        }
    }

    bool add_new = false;
    if (name != prev_name)
    {
        opScheduler *new_scheduler;
        new_scheduler = new opScheduler(*_scheduler);
        _scheduler = new_scheduler;
        add_new = true;
    }

    _scheduler->set_name(name);
    _scheduler->set_filename(name,true);
    _scheduler->set_cycle_time(ui->sbTimeout->value());
    _scheduler->set_serial_timeout(ui->sbSerialTimeout->value());
    _scheduler->set_policy_on_timeout((TIMEOUT_POLICY)(ui->cbTimeOutHandling->currentIndex()));
    _scheduler->set_policy_on_error((RUN_POLICY)(ui->cbErrorHandling->currentIndex()));
    _scheduler->save_xml();
    if ((add_new)||(_b_new))
    {
        root->add_scheduler(_scheduler);

        if (!root->get_last_error().isEmpty())
        {
            QMessageBox::critical(this, "Error", root->get_last_error());
            return;
        }
        _b_new = false;
    }
    _b_save_ok = true;
    root->save_project_file();
    return;
}

//--------------------------------
/*
void    wndScheduler::save_as()
{
    if (_scheduler== nullptr) return;
    if (_root == nullptr) return;
}*/
//--------------------------------
void    wndScheduler::done_btn()
{
    if (_scheduler== nullptr) return;
    if (_root == nullptr) return;

    if (QMessageBox::question(this,"Confirm", "Do you want to save the scheduler before closing?") == QMessageBox::Yes)
        save();

    if (_b_save_ok)
    {
        done (QMessageBox::Ok);
        this->parentWidget()->close();
    }
}
//--------------------------------
void wndScheduler::update_buttons()
{
    if (_scheduler!=nullptr)
    {
        ui->btnStart->setVisible(!_scheduler->isRunning());
        ui->btnStop->setVisible(_scheduler->isRunning());
        ui->btnStart->setEnabled(!_scheduler->isRunning());
        ui->btnStop->setEnabled(_scheduler->isRunning());

        ui->cbErrorHandling->setEnabled(!_scheduler->isRunning());
        ui->cbTimeOutHandling->setEnabled(!_scheduler->isRunning());
        ui->sbTimeout->setEnabled(!_scheduler->isRunning());
        ui->tblDevices->setEnabled(!_scheduler->isRunning());
        return;
    }

    ui->btnStart->setVisible(true);
    ui->btnStop->setVisible(true);

    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(false);

}
//--------------------------------
void wndScheduler::init_table()
{
    ui->tblDevices->clear();

    ui->tblDevices->setColumnCount(4);
    ui->tblDevices->setHorizontalHeaderLabels(QStringList({"Device Name","Enable","Connected","Running"}));
    if (_root == nullptr)
        return;
    // Put in the table all devices. From _scheduler set the ones that are enabled
    QVector<radarInstance*> devices = _root->get_available_radars();
    int row = 0;
    for (auto& device : devices)
    {
        ui->tblDevices->setRowCount(row+1);
        // Name
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        item->setText(device->get_device_name());
        item->setData(Qt::UserRole, QVariant::fromValue<void*>(device));
        ui->tblDevices->setItem(row, COL_NAME, item);
        // Enabled
        bool enabled = _scheduler->has_device(device->get_device_name());
        QCheckBox* cb = new QCheckBox();
        cb->setChecked(enabled);
        ui->tblDevices->setCellWidget(row, COL_ENABLED, cb);
        connect(cb, &QCheckBox::checkStateChanged, this, &wndScheduler::enable_disable);
        // Connected
        QLabel* lblConnection = new QLabel();
        lblConnection->setPixmap( device->is_connected() ? _pixConnected : _pixDisconnected);
        ui->tblDevices->setCellWidget(row, COL_CONNECTION, lblConnection);
        // Running
        QLabel* lblRunning = new QLabel();
        lblRunning->setPixmap( device->is_connected() && (_scheduler->has_device(device)) && (_scheduler->isRunning()) ? _pixRunning : _pixHalted);
        ui->tblDevices->setCellWidget(row, COL_STATUS, lblRunning);

        row++;
    }
}
//--------------------------------
void wndScheduler::enable_disable(Qt::CheckState enabled)
{
    if  (_scheduler==nullptr) return;
    int row = -1;
    QWidget *w = qobject_cast<QWidget *>(sender());
    if(w)
        row = ui->tblDevices->indexAt(w->pos()).row();

    if ((row<0)||(row>=ui->tblDevices->rowCount())) return;

    QTableWidgetItem* item = ui->tblDevices->item(row, COL_NAME);
    radarInstance* device = (radarInstance*)(item->data(Qt::UserRole).value<void*>());
    if (device == nullptr) return;

    if (enabled)
        _scheduler->add_radar(device);
    else
        _scheduler->delete_radar(device);
}
//--------------------------------
int  wndScheduler::get_row_device(radarInstance* device)
{
    if (device == nullptr) return -1;

    for (int r = 0; r < ui->tblDevices->rowCount(); r++)
    {
        QTableWidgetItem* item = ui->tblDevices->item(r, COL_NAME);
        if ((radarInstance*)(item->data(Qt::UserRole).value<void*>())==device)
            return r;
    }
    return -1;
}
//--------------------------------
void wndScheduler::running()
{
    update_buttons();
}

//--------------------------------
void wndScheduler::halted(radarInstance* device)
{
    for (int r = 0; r < ui->tblDevices->rowCount(); r++)
    {
        QTableWidgetItem* item = ui->tblDevices->item(r, COL_NAME);
        radarInstance* device_at_row = (radarInstance*)(item->data(Qt::UserRole).value<void*>());
        if ((device==nullptr)||(device_at_row==device))
        {
            set_status_at_row(r, false);
        }
    }

    update_buttons();
}
//--------------------------------
void wndScheduler::connection_error(radarInstance* device)
{
    int row = get_row_device(device);
    if (row == -1) return;

    QLabel* lblConnection = (QLabel*)(ui->tblDevices->cellWidget(row,COL_CONNECTION));
    lblConnection->setPixmap( _pixDisconnected);
    ui->tblDevices->setCellWidget(row, COL_CONNECTION, lblConnection);
}
//--------------------------------
void wndScheduler::connection_done(radarInstance* device)
{
    int row = get_row_device(device);
    if (row == -1) return;

    QLabel* lblConnection = (QLabel*)(ui->tblDevices->cellWidget(row,COL_CONNECTION));
    lblConnection->setPixmap( _pixConnected);
    ui->tblDevices->setCellWidget(row, COL_CONNECTION, lblConnection);
}
//--------------------------------
void wndScheduler::set_status_at_row(int row, bool ok)
{
    if ((row<0)||(row>=ui->tblDevices->rowCount())) return;

    QLabel* lblStatus = (QLabel*)(ui->tblDevices->cellWidget(row,COL_STATUS));
    lblStatus->setPixmap( ok ? _pixRunning : _pixHalted);
    ui->tblDevices->setCellWidget(row, COL_STATUS, lblStatus);

}
//--------------------------------
void wndScheduler::init_error(radarInstance* device)
{
    int row = get_row_device(device);
    if (row == -1) return;
    set_status_at_row(row,false);
    _scheduler->stop(device);
}
//--------------------------------
void wndScheduler::init_done(radarInstance* device)
{
    int row = get_row_device(device);
    if (row == -1) return;
    set_status_at_row(row,true);
}
//--------------------------------
void wndScheduler::postacquisition_error(radarInstance* device)
{
    int row = get_row_device(device);
    if (row == -1) return;
    set_status_at_row(row,false);
}
//--------------------------------
void wndScheduler::postacquisition_done(radarInstance* device)
{
    int row = get_row_device(device);
    if (row == -1) return;
    set_status_at_row(row,true);
}
//--------------------------------
void wndScheduler::timeout_error()
{
    ui->lblTimeout->setText("Timeout error");
    ui->lblTimeout->setStyleSheet("QLabel { color : red; }");
}

//--------------------------------
void wndScheduler::timing_ok()
{
    ui->lblTimeout->setText("Timing ok");
    ui->lblTimeout->setStyleSheet("QLabel { color : black; }");

}
//--------------------------------
void wndScheduler::cannot_start()
{
    QMessageBox::critical(this, "Error", "Cannot start this process, due to another already running");
    update_buttons();
}
//--------------------------------
void wndScheduler::start()
{
    if (_scheduler==nullptr)
        return;

    if (_scheduler->get_number_of_devices()==0)
    {
        QMessageBox::critical(this, "Error", "Select at least one device");
        return;
    }
     ui->lblTimeout->setText("Init");
    _scheduler->set_policy_on_error((RUN_POLICY)ui->cbErrorHandling->currentIndex());
    _scheduler->set_policy_on_timeout((TIMEOUT_POLICY)(ui->cbTimeOutHandling->currentIndex()));
    _scheduler->set_cycle_time(ui->sbTimeout->value());

    _scheduler->run();

}
//--------------------------------
void wndScheduler::stop()
{
    _scheduler->stop();

}
