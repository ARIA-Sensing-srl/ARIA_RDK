/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "wndscanmodules.h"
#include "qmessagebox.h"
#include "radarinstance.h"
#include "ui_wndscanmodules.h"
#include "wndimportradar.h"


using namespace std;

wndScanModules::wndScanModules(QVector<projectItem*> radar_modules, radarProject* project, QWidget *parent) :
    QDialog(parent),
    _radar_project(project),
    _radar_modules(radar_modules),
    ui(new Ui::wndScanModules)
{
    ui->setupUi(this);
    connect(ui->btnScan, &QPushButton::clicked, this, &wndScanModules::scan);
    connect(ui->tblSerialPorts, &QTableWidget::itemSelectionChanged, this, &wndScanModules::portSelected);
    connect(ui->btnImport,&QPushButton::clicked, this, &wndScanModules::importRadar);
    connect(ui->btnDone,&QPushButton::clicked, this, &wndScanModules::close);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &wndScanModules::refresh);

    refreshSerialPortTable();

}

wndScanModules::~wndScanModules()
{
    /*
    for (auto& module: _connected_radars)
        for(auto& radar :  module)
        {
            module.removeAll(radar);
            if (radar!=nullptr)
            {
                delete radar;
            }
        }
    _connected_radars.clear();*/
    delete ui;
}

void wndScanModules::scan()
{
    scanRadarModules();
}

void wndScanModules::scanRadarModules()
{
    projectItem* item;

    //int n=0;

    auto selected = ui->tblSerialPorts->selectionModel()->selectedRows();
    QList<QSerialPortInfo> selectedPorts;

    for (auto sel : selected)
    {
        int row = sel.row();
        selectedPorts.append(_availablePorts[row]);

        for (auto instance : _connected_radars[row])
            if (instance!=nullptr)
                delete instance;

        _connected_radars[row].clear();
    }

    if (selectedPorts.isEmpty())
    {
        QMessageBox::warning(this, "Warning", "No port selected");
        return;
    }

    for (auto sel : selected)
    {
        int n = sel.row();
        auto &port = _availablePorts[n];

        _connected_radars[n] = QVector<radarInstance*>();
        qDebug() << port.portName();
        qDebug() << port.description();

        foreach (item, _radar_modules)
        {
            if (item->get_type()!=DT_RADARMODULE) continue;
            radarModule* module =(radarModule*)(item);
            radarInstance* instance = new radarInstance(module);
            instance->setParent(this);
            if (instance == nullptr)
                return;
            radarProject * owner = module->get_root();

            if (owner != nullptr)
                instance->attach_to_workspace(owner->get_workspace());

            instance->set_fixed_port(true);
            instance->set_fixed_id(false);

            if (!instance->set_port(port, module->get_serial_port_configuration(), false)) continue;

            if (instance->is_connected())
            {
                QByteArray uid = instance->get_actual_uid();
                if (uid.isEmpty())
                    uid = QByteArray::fromHex("FFFFFFFF");

                instance->set_uid(uid);
                instance->set_expected_portname(port.portName());
                _connected_radars[n].append(instance);
            }
            else
                delete instance;

            instance->disconnect();
        }
    }
    portSelected();
}

void wndScanModules::portSelected()
{
    int row = ui->tblSerialPorts->currentRow();
    ui->tblDevices->clear();
    ui->tblDevices->setColumnCount(3);
    ui->tblDevices->setHorizontalHeaderLabels(QStringList({"#","Radar UID", "Radar Module"}));
    ui->tblDevices->setRowCount(_connected_radars[row].count());
    for (int r = 0; r < _connected_radars[row].count(); r++)
    {
        QTableWidgetItem *item;
        item = new QTableWidgetItem();
        item->setText(QString::number(r+1));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tblDevices->setItem(r, 0, item);

        item = new QTableWidgetItem();
        item->setText(_connected_radars[row][r]->get_uid_string());
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tblDevices->setItem(r, 1, item);

        item = new QTableWidgetItem();
        radarModule* module =_connected_radars[row][r]->get_module();
        item->setText(module==nullptr? "unknown model": module->get_name());
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tblDevices->setItem(r, 2, item);
    }
}

void wndScanModules::importRadar()
{
    if (_radar_project==nullptr) return;
    int n = ui->tblSerialPorts->currentRow();
    int m = ui->tblDevices->currentRow();
    if ((n<0)||(m<0)) return;

    QVector<radarInstance*> project_devices =  _radar_project->get_available_radars();

    wndImportRadar importDlg(this,_connected_radars[n][m]);
    if (importDlg.exec()!=QMessageBox::Ok)
        return;

    QString fname = QFileInfo(_connected_radars[n][m]->get_module()->get_name()).baseName();
    QString suffix = "";

    QString tentative_name = fname+suffix+".ard";
    int n_new = 0;

    bool bfound = false;
    do
    {
        bfound = false;
        for (const auto& device : project_devices)
            if (device != nullptr)
                if (device->get_name() == tentative_name)
                {bfound = true; break;}

        if (bfound)
        {
            suffix = QString::number(n_new);
            tentative_name = fname+suffix+".ard";
            n_new++;
        }
    }
    while (bfound);

    radarInstance *newRadar  = new radarInstance(*_connected_radars[n][m]);

    if (newRadar==nullptr)
    {
        QMessageBox::critical(this, "Error", "Error while creating new radar");
        return;
    }
    newRadar->attach_to_workspace(_radar_project->get_workspace());
    newRadar->set_name(tentative_name);
    newRadar->set_filename(tentative_name);

    _radar_project->add_radar_instance(newRadar);
    _radar_project->save_project_file();

    if (!(_radar_project->get_last_error().isEmpty()))
    {
        QMessageBox::critical(this, "Error", _radar_project->get_last_error());
    }
}


void    wndScanModules::refreshSerialPortTable()
{
    _availablePorts = QSerialPortInfo::availablePorts();
    ui->tblSerialPorts->clear();
    ui->tblDevices->clear();
    ui->tblSerialPorts->setColumnCount(5);
    ui->tblSerialPorts->setHorizontalHeaderLabels(QStringList({"Description","Manufacturer","Port Name","vendorId","productId"}));


    const QList<QSerialPortInfo>& avports = _availablePorts;
    int row =0;
    for (const QSerialPortInfo &info : (avports)) {
        const QString description = info.description();
        const QString manufacturer = info.manufacturer();
        const QString portName     = info.portName();
        int vendorId = (int)info.vendorIdentifier();
        int productId = (int)info.productIdentifier();

        ui->tblSerialPorts->setRowCount(row+1);
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(description);

        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tblSerialPorts->setItem(row, 0, item);

        item = new QTableWidgetItem();
        item->setText(manufacturer);

        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tblSerialPorts->setItem(row, 1, item);

        item = new QTableWidgetItem();
        item->setText(portName);

        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tblSerialPorts->setItem(row, 2, item);

        item = new QTableWidgetItem();
        item->setText(QByteArray::number(vendorId,16));

        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tblSerialPorts->setItem(row, 3, item);

        item = new QTableWidgetItem();
        item->setText(QByteArray::number(productId,16));

        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->tblSerialPorts->setItem(row, 4, item);
        row++;
    }
}

void wndScanModules::close()
{
    done(0);
}

void wndScanModules::refresh()
{
    refreshSerialPortTable();
}
