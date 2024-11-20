/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef WNDIMPORTRADAR_H
#define WNDIMPORTRADAR_H

#include <QDialog>
#include "radarinstance.h"
namespace Ui {
class wndImportRadar;
}

class wndImportRadar : public QDialog
{
    Q_OBJECT

    std::shared_ptr<radarInstance>      _radar_device;
public:
    explicit wndImportRadar(QWidget *parent = nullptr, radarInstance* radar_device = nullptr );
    ~wndImportRadar();

private:
    Ui::wndImportRadar *ui;

public slots:
    void    import();
    void    cancel();
};

#endif // WNDIMPORTRADAR_H
