/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef WNDSCANMODULES_H
#define WNDSCANMODULES_H

#include <QDialog>
#include <QtSerialPort>
#include <radarmodule.h>

namespace Ui {
class wndScanModules;
}

class wndScanModules : public QDialog
{
    Q_OBJECT

private:
    radarProject*               _radar_project;
    QVector<projectItem*>       _radar_modules;
    QList<QSerialPortInfo>      _availablePorts;
    QMap<int, QVector<radarInstance*>>  _connected_radars;

    void    refreshSerialPortTable();
    void    scanRadarModules();
public:
    explicit wndScanModules(QVector<projectItem*> radar_modules=QVector<projectItem*>(), radarProject* project = nullptr, QWidget *parent = nullptr);
    ~wndScanModules();

private:
    Ui::wndScanModules *ui;
public slots:
    void        scan();
    void        portSelected();
    void        importRadar();
    void        close();
    void        refresh();
};

#endif // WNDSCANMODULES_H
