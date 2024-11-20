/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef WNDSCHEDULER_H
#define WNDSCHEDULER_H

#include <QDialog>
#include <QLabel>
#include "../scheduler/opscheduler.h"


namespace Ui {
class wndScheduler;
}

class wndScheduler : public QDialog
{
    Q_OBJECT

public:
    explicit wndScheduler(QWidget *parent = nullptr, radarProject* root=nullptr, opScheduler* opsched=nullptr);
    ~wndScheduler();
    opScheduler* getScheduler() {return _scheduler;}

private:
    Ui::wndScheduler *ui;
    radarProject*    _root;
    opScheduler*     _scheduler;
    bool             _b_new;
    bool             _b_save_ok;
    QPixmap           _pixConnected;
    QPixmap           _pixDisconnected;
    QPixmap           _pixRunning;
    QPixmap           _pixHalted;

    void connect_scheduler_signals();
    void update_buttons();
    void init_table();
    int  get_row_device(radarInstance* device);
    void set_connected_image(bool connected, radarInstance* device);
    void set_running_image(bool running, radarInstance* device);
    void set_status_at_row(int row, bool ok=false);
public slots:
    void             save();
    //void             save_as();
    void             done_btn();
    void             start();
    void             stop();
    void             enable_disable(Qt::CheckState enabled);

//----------------------------
// Scheduler related signals
    void running();
    void halted(radarInstance* device);
    void connection_error(radarInstance* device);
    void connection_done(radarInstance* device);
    void init_error(radarInstance* device);
    void init_done(radarInstance* device);
    void preacquisition_error(radarInstance* device);
    void preacquisition_done(radarInstance* device);
    void postacquisition_error(radarInstance* device);
    void postacquisition_done(radarInstance* device);
    void timeout_error();
    void timing_ok();
    void cannot_start();

public:


};

#endif // WNDSCHEDULER_H
