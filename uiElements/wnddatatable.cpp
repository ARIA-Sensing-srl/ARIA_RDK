#include "wnddatatable.h"
#include "ui_wnddatatable.h"

wnddatatable::wnddatatable(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::wnddatatable)
{
    ui->setupUi(this);
}

wnddatatable::~wnddatatable()
{
    delete ui;
}
