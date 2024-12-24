/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef OPSCHEDULER_H
#define OPSCHEDULER_H

#include <QObject>
#include <QThread>
#include <radarinstance.h>
#include <octaveinterface.h>
#include <octave.h>
#include <ovl.h>
#include <octavews.h>
#include <parse.h>
#include <radarproject.h>

enum SCHEDULER_STATUS{IDLE,
                        INIT_PRE_START,
                        INIT_PRE_DONE,
                        INIT_SCRIPTS_STARTS,
                        INIT_SCRIPTS_DONE,
                        POSTACQ_PARAM_START,
                        POSTACQ_PARAM_DONE,
                        POSTACQ_SCRIPTS_START,
                        POSTACQ_SCRIPTS_DONE,
                        HALT
                    };

class opSchedulerOperations : public QObject
{
    Q_OBJECT
private:
    radarInstance*      _device;
    octaveInterface*    _octint;
    SCHEDULER_STATUS    _status;


public:
    opSchedulerOperations(radarInstance* device, octaveInterface *octint);
    radarInstance*       get_device() {return _device;}
    SCHEDULER_STATUS     get_status() {return _status;}

    void                 connect_device();
    void                 init_device();
    void                 post_acquisition(bool restart=false);
    void                 set_idle();
    void                 halt();

public slots:
    //void   radar_device_updated();
    void   error_during_comm(const QString&);
    void   device_connected_done();
    void   init_params_done();
    void   init_scripts_done();
    void   postacq_params_done();
    void   postacq_scripts_done();

signals:
    void device_connected(radarInstance* device);
    void error_on_connection(radarInstance* device);

    void initDone(opSchedulerOperations* op);
    void initError(opSchedulerOperations* op);

    void postAcquisitionDone(opSchedulerOperations* op);
    void postAcquisitionError(opSchedulerOperations* op);
};


//--------------------------------------------------------
//--------------------------------------------------------
// Main scheduler
//--------------------------------------------------------
//--------------------------------------------------------
enum RUN_POLICY {HALT_ALL=0, HALT_DEVICE=1, CONTINUE_ON_ERROR=2, REINIT=3};
enum TIMEOUT_POLICY{CONTINUE_ON_TIMEOUT=0, HALT_ON_TIMEOUT=1};

class opScheduler : public QObject, public projectItem
{
    Q_OBJECT
private:
    octaveInterface*                _octint;
    QList<radarInstance*>           _devices;
    QList<opSchedulerOperations*>   _workers;
    bool                            _run_scripts;
    QList<octaveScript*>            _scripts;
    QList<QThread*>                 _wthreads;      // Let's have a thread per each radar device
    bool                            _b_running;
    int                             _cycle_time;    // -1 means no timer is applied
    int                             _serial_timeout;
    QTimer                          _main_timer;
    bool                            _timer_done;
    QVector<QString>                _device_names;
    void                            add_radars_from_names();
    RUN_POLICY                      _policy_on_error;
    TIMEOUT_POLICY                  _timeout_policy;
public:
    opScheduler(octaveInterface* octint, projectItem* parent = nullptr);
    opScheduler(const opScheduler& sched);
    ~opScheduler();

    void        add_radar(radarInstance* device);
    void        delete_radar(radarInstance* device);
    void        delete_all_radars();
    void        connect_devices();
    void        init_devices();
    void        run();
    void        stop(radarInstance* device = nullptr);
    bool        isRunning() {return _b_running;}
    void        acquisition_loop();

    bool        is_timed();
    void        set_cycle_time(int milliseconds);

    bool            save_xml(QDomDocument& document);
    bool            save_xml();
    bool            load_xml();

    bool            has_device(QString devname);
    bool            has_device(radarInstance* device);
    int             get_number_of_devices() {return _devices.count();}

    void            set_policy_on_error(RUN_POLICY  policy_on_error)        {_policy_on_error = policy_on_error;}
    void            set_policy_on_timeout(TIMEOUT_POLICY timeout_policy)    {_timeout_policy  = timeout_policy;}

    bool            set_filename(QString filename, bool save = false);

    bool            add_script(octaveScript* script);
    bool            has_script(octaveScript* script);
    void            delete_script(octaveScript* script);

    void            clear_scripts();
    RUN_POLICY      get_policy_on_error()   { return _policy_on_error;}
    TIMEOUT_POLICY  get_policy_on_timeout() { return _timeout_policy;}
    int             get_cycle_time()        { return _cycle_time;}
    int             get_serial_timeout()    { return _serial_timeout;}

    void            set_serial_timeout(int timeout);

public slots:

    void device_connected(radarInstance* device);
    void error_on_connection(radarInstance* device);

    void initDone(opSchedulerOperations* op);
    void initError(opSchedulerOperations* op);

    void postAcquisitionDone(opSchedulerOperations* op);
    void postAcquisitionError(opSchedulerOperations* op);

    void timer_done();

signals:
    void running();
    void halted(radarInstance* device);
    void connection_error(radarInstance* device);
    void connection_done(radarInstance* device);
    void connection_done_all();
    void init_error(radarInstance* device);
    void init_done(radarInstance* device);
    void init_done_all();
    void postacquisition_error(radarInstance* device);
    void postacquisition_done(radarInstance* device);
    void postacquisition_done_all();
    void timeout_error();
    void timing_ok();
    void other_scheduler_running();
};

#endif // OPSCHEDULER_H
