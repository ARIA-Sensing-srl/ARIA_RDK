/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef WNDRADAR_H
#define WNDRADAR_H

#include <QDialog>

namespace Ui {
class wndradar;
}

class wndradar : public QDialog
{
    Q_OBJECT

public:
    explicit wndradar(QWidget *parent = nullptr);
    ~wndradar();

private:
    Ui::wndradar *ui;
};

#endif // WNDRADAR_H
