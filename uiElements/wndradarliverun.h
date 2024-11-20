/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef WNDRADARLIVERUN_H
#define WNDRADARLIVERUN_H

#include <QDialog>

namespace Ui {
class wndradarliverun;
}

class wndradarliverun : public QDialog
{
    Q_OBJECT

public:
    explicit wndradarliverun(QWidget *parent = nullptr);
    ~wndradarliverun();

private:
    Ui::wndradarliverun *ui;
};

#endif // WNDRADARLIVERUN_H
