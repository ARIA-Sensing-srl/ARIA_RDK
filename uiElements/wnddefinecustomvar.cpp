/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "wnddefinecustomvar.h"
#include "ui_wnddefinecustomvar.h"
#include <octavews.h>

wndDefineCustomVar::wndDefineCustomVar(const variable& var, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::wndDefineCustomVar)
{
    ui->setupUi(this);
}

wndDefineCustomVar::~wndDefineCustomVar()
{
    delete ui;
}
