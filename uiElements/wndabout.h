/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef WNDABOUT_H
#define WNDABOUT_H

#include <QDialog>

namespace Ui {
class wndAbout;
}

class wndAbout : public QDialog
{
    Q_OBJECT

public:
    explicit wndAbout(QWidget *parent = nullptr);
    ~wndAbout();

private:
    Ui::wndAbout *ui;

    void close_wnd();
};

#endif // WNDABOUT_H
