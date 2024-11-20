/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "wndradarliverun.h"
#include "ui_wndradarliverun.h"

wndradarliverun::wndradarliverun(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::wndradarliverun)
{
    ui->setupUi(this);
}

wndradarliverun::~wndradarliverun()
{
    delete ui;
}
