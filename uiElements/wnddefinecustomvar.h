/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef WNDDEFINECUSTOMVAR_H
#define WNDDEFINECUSTOMVAR_H

#include <QDialog>
#include "octavews.h"
namespace Ui {
class wndDefineCustomVar;
}

class wndDefineCustomVar : public QDialog
{
    Q_OBJECT

public:
    explicit wndDefineCustomVar(const variable& var, QWidget *parent = nullptr);
    ~wndDefineCustomVar();

private:
    Ui::wndDefineCustomVar *ui;
};

#endif // WNDDEFINECUSTOMVAR_H
