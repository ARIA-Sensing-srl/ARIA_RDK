/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "wndabout.h"
#include "ui_wndabout.h"

wndAbout::wndAbout(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::wndAbout)
{
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::clicked, this, &wndAbout::close_wnd);
}

wndAbout::~wndAbout()
{
    delete ui;
}


void wndAbout::close_wnd()
{
    close();
}
