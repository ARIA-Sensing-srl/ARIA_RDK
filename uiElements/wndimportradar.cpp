/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "wndimportradar.h"
#include "qmessagebox.h"
#include "ui_wndimportradar.h"

wndImportRadar::wndImportRadar(QWidget *parent, radarInstance* radar_device) :
    QDialog(parent),
    _radar_device(radar_device),
    ui(new Ui::wndImportRadar)
{
    ui->setupUi(this);
    if (_radar_device!=nullptr)
    {
        ui->cbSetFixedRadarId->setCheckState(_radar_device->fixed_id()? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        ui->cbSetFixedPort->setCheckState(_radar_device->fixed_port()? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    }

    connect(ui->btnCancel,SIGNAL(clicked()),this,SLOT(cancel()));
    connect(ui->btnOk,SIGNAL(clicked()),this,SLOT(import()));
}

wndImportRadar::~wndImportRadar()
{
    delete ui;
}


void wndImportRadar::cancel()
{
    done(QMessageBox::Cancel);
}

void wndImportRadar::import()
{
    if (_radar_device!=nullptr)
    {
        _radar_device->set_fixed_id(ui->cbSetFixedRadarId->checkState()==Qt::CheckState::Checked);
        _radar_device->set_fixed_port(ui->cbSetFixedPort->checkState()==Qt::CheckState::Checked);
    }
    done(QMessageBox::Ok);
}
