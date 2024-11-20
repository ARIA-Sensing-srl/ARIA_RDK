/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "wndradar.h"
#include "ui_wndradar.h"

wndradar::wndradar(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::wndradar)
{
    ui->setupUi(this);
}

wndradar::~wndradar()
{
    delete ui;
}
