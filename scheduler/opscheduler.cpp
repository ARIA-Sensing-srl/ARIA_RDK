/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "opscheduler.h"
#include "../mainwindow.h"
//----------------------------------------------------------------
// Singe device worker class
opSchedulerOperations::opSchedulerOperations(radarInstance* device, octaveInterface* octint) :
    _device (device),
    _octint (octint),
    _status(HALT)
{
    if (_device != nullptr)
    {
        connect(_device, &radarInstance::init_params_done,  this, &opSchedulerOperations::init_params_done, Qt::QueuedConnection);
        connect(_device, &radarInstance::init_scripts_done, this, &opSchedulerOperations::init_scripts_done,Qt::QueuedConnection);
        connect(_device, &radarInstance::postacq_params_done,this, &opSchedulerOperations::postacq_params_done,Qt::QueuedConnection);
        connect(_device, &radarInstance::postacq_scripts_done,this, &opSchedulerOperations::postacq_scripts_done,Qt::QueuedConnection);
        connect(_device, &radarInstance::connection_done, this, &opSchedulerOperations::device_connected_done,Qt::QueuedConnection);

        connect(_device, &radarInstance::rx_timeout,         this, &opSchedulerOperations::error_during_comm,Qt::QueuedConnection);
        connect(_device, &radarInstance::tx_timeout,         this, &opSchedulerOperations::error_during_comm,Qt::QueuedConnection);
        connect(_device, &radarInstance::serial_error_out,   this, &opSchedulerOperations::error_during_comm,Qt::QueuedConnection);
    }
}
//---------------------------
void   opSchedulerOperations::device_connected_done()
{
    if (_status != IDLE) return;
    emit device_connected(_device);
}

//---------------------------
void   opSchedulerOperations::init_params_done()
{
    if (_status != INIT_PRE_START) return;
    _status = INIT_PRE_DONE;
    init_device();

}
//---------------------------
void   opSchedulerOperations::init_scripts_done()
{
    if (_status != INIT_SCRIPTS_STARTS) return;
    _status = INIT_SCRIPTS_DONE;
    init_device();
}
//---------------------------
void   opSchedulerOperations::postacq_params_done()
{
    if (_status != ACQUISITION_PARAM_START) return;
    _status = ACQUISITION_PARAM_DONE;
    post_acquisition();
}
//---------------------------
void   opSchedulerOperations::postacq_scripts_done()
{
    if (_status != ACQUISITION_SCRIPTS_START) return;
    _status = ACQUISITION_SCRIPTS_DONE;
    post_acquisition();

}
//---------------------------
void   opSchedulerOperations::error_during_comm(const QString&)
{
    if (_status == IDLE)                {_status = IDLE; emit error_on_connection(_device);}
    if ((_status == INIT_PRE_START)||(_status== INIT_SCRIPTS_STARTS)||( _status == INIT_PRE_DONE))
    {
        _status = IDLE;
        emit initError(this);
    }

    if ((_status== ACQUISITION_PARAM_START)||( _status == ACQUISITION_PARAM_DONE)||(_status==ACQUISITION_SCRIPTS_START))
    {
        _status = IDLE;
        emit postAcquisitionError(this);
    }
}
//---------------------------
void  opSchedulerOperations::connect_device()
{
    if (_status == HALT)
    {
        halt();
        return;
    }

    if (_device != nullptr)
    {
        if (!_device ->is_connected())
        {
            _device ->auto_connect();
            if (!_device ->is_connected())
            {
                emit error_on_connection(_device);
                return;
            }
        }
        else
            emit device_connected(_device);
    }
}
//----------------------------------------------------------------
void    opSchedulerOperations::init_device()
{
    if (_status == HALT)
    {
        halt();
        return;
    }

    if (_device != nullptr)
    {
        if (_status == IDLE)
        {
            _status = INIT_PRE_START;
            // Catch early errors

            if (!_device->init_pre())
                emit initError(this);

            return;
        }

        if (_status == INIT_PRE_DONE)
        {
            _status = INIT_SCRIPTS_STARTS;
            if (!_device->init_scripts())
                emit initError(this);
            else
            {
                _status = INIT_SCRIPTS_DONE;
                emit initDone(this);
            }

            return;
        }

        if (_status == INIT_SCRIPTS_DONE)
        {
            emit initDone(this);
            return;
        }
    }
    else
    {
        emit initError(this);
    }
}

void opSchedulerOperations::set_idle()
{
    if ((_status==HALT)||(_status==IDLE))
        _status = IDLE;
}
//----------------------------------------------------------------
void opSchedulerOperations::halt()
{
    _status = HALT;
    //_device->disconnect();
    //if (_status==HALT) _device->disconnect(); //_status=IDLE;} else {_device->disconnect(); _status=HALT;}
}
//----------------------------------------------------------------
void    opSchedulerOperations::post_acquisition()
{
    if (_status == HALT)
    {
        halt();
        return;
    }
    if (_device != nullptr)
    {
        if (_status == ACQUISITION_PARAM_DONE)
        {
            _status = ACQUISITION_SCRIPTS_START;
            if (!_device->postacquisition_scripts())
                emit postAcquisitionError(this);
            return;
        }

        if ((_status == INIT_SCRIPTS_DONE)||(_status == ACQUISTION_RESTART))
        {
            _status = ACQUISITION_PARAM_START;
            // Catch early errors
            if (!_device->postacquisition_pre())
                emit postAcquisitionError(this);

        }

        if (_status == ACQUISITION_SCRIPTS_DONE)
        {
            _status = ACQUISTION_RESTART;
            emit postAcquisitionDone(this);

            return;
        }
    }
    else
    {
        emit postAcquisitionError(this);
    }
}
//----------------------------------------------------------------
// Controller
opScheduler::opScheduler(const opScheduler& sched)
{
    // Should we following lines this to operator =?
    projectItem::operator = (sched);
    _cycle_time     = sched._cycle_time;
    _serial_timeout = sched._serial_timeout;
    _timeout_policy = sched._timeout_policy;
    _policy_on_error= sched._policy_on_error;

    for (auto& worker: sched._workers)
        if (worker!=nullptr)
            add_radar(worker->get_device());

    _scripts = sched._scripts;

    connect(&_main_timer, &QTimer::timeout, this, &opScheduler::timer_done);
}
//----------------------------
opScheduler::opScheduler(octaveInterface* octint, projectItem* parent) : projectItem("newScheduler",DT_SCHEDULER, parent),
    _octint(octint),
    _devices(),
    _workers(),
    _wthreads(),
    _b_running(false),
    _cycle_time(-1),
    _serial_timeout(1200),
    _policy_on_error(HALT_ALL),
    _timeout_policy(CONTINUE_ON_TIMEOUT)
{
    _main_timer.setSingleShot(true);
    _main_timer.blockSignals(false);
    connect(&_main_timer, &QTimer::timeout, this, &opScheduler::timer_done);
}
//----------------------------
opScheduler::~opScheduler()
{

    for (auto &thread: _wthreads)
    {
        thread->quit();
        thread->wait();
        delete thread;
    }
}
//----------------------------
void    opScheduler::add_radars_from_names()
{
    while (!_devices.isEmpty())
    {
        delete_radar(_devices.at(0));
    }

    radarProject* root = get_root();
    if (root==nullptr) return;
    QVector<radarInstance*> available_devices = root->get_available_radars();

    for (auto& device: available_devices)
    {
        if (_device_names.contains(device->get_device_name()))
            add_radar(device);
    }
}
//----------------------------
void    opScheduler::add_radar(radarInstance * device)
{
    if (device==nullptr) return;
    if (_devices.contains(device)) return;
    _devices.append(device);
    device->set_serial_timeout(_serial_timeout);

    opSchedulerOperations *opworker = new opSchedulerOperations(device, _octint);
    _workers.append(opworker);
#ifdef RADAR_IN_A_THREAD
    QThread* newThread = new QThread(this);
    _wthreads.append(newThread);

    opworker->moveToThread(newThread);
    //device->moveToThread(newThread)
    connect(newThread, &QThread::finished, opworker, &QObject::deleteLater);
#endif
    // Signals connection for the proper order of operations
    connect(opworker, &opSchedulerOperations::device_connected,     this, &opScheduler::device_connected);
    connect(opworker, &opSchedulerOperations::error_on_connection,  this, &opScheduler::error_on_connection);
    connect(opworker, &opSchedulerOperations::initDone,             this, &opScheduler::initDone);
    connect(opworker, &opSchedulerOperations::initError,            this, &opScheduler::initError);
    connect(opworker, &opSchedulerOperations::postAcquisitionDone,   this, &opScheduler::postAcquisitionDone);
    connect(opworker, &opSchedulerOperations::postAcquisitionError,  this, &opScheduler::postAcquisitionError);

    //connect(opworker, &Worker::resultReady, this, &Controller::handleResults);
    _device_names.append(device->get_device_name());
}
//----------------------------
void        opScheduler::connect_devices()
{
    // first, connect all devices
    for (auto& worker : _workers)
        if (worker!=nullptr)
            worker->connect_device();

}
//----------------------------
void        opScheduler::init_devices()
{
    // first, connect all devices
    for (auto& worker : _workers)
        if (worker!=nullptr)
            worker->init_device();
}
//----------------------------
// This is a default operation
void        opScheduler::run()
{
    radarProject* project = get_root();
    if (project!=nullptr)
    {
        QVector<opScheduler*> scheds = project->get_available_scheduler();
        for (auto& sched: scheds)
            if (sched!=nullptr)
            {
                if (sched->isRunning())
                {
                    emit other_scheduler_running();

                    return;
                }
            }
    }

    _b_running = true;
    for (auto& worker: _workers)
        if (worker!=nullptr)
            worker->set_idle();

    for (auto& worker: _workers)
        if (worker->get_status()!=IDLE)
            return;

    // first, connect all devices
    for (auto& worker : _workers)
        if (worker!=nullptr)
        {
            worker->get_device()->set_serial_timeout(_serial_timeout);
            worker->connect_device();
        }

    emit running();
}
//----------------------------
bool    opScheduler::has_device(QString devname)
{
    return _device_names.contains(devname);
}
//----------------------------
bool    opScheduler::has_device(radarInstance* device)
{
    return _devices.contains(device);
}
//----------------------------
void        opScheduler::stop(radarInstance* device)
{
    if (device == nullptr)
    {
        for (auto& worker : _workers)
            if (worker!=nullptr)
                worker->halt();

        _b_running = false;
        emit halted(nullptr);
		if (MainWindow::mainWnd!=nullptr)
			MainWindow::mainWnd->cleanUpFiles();
    }
    else
    {
        for (auto& worker : _workers)
		{
            if (worker!=nullptr)
                if (worker->get_device() == device)
                {
                    worker->halt();
                    emit halted(device);
                }
		}
        //delete_radar(device);
		if (MainWindow::mainWnd!=nullptr)
			MainWindow::mainWnd->cleanUpFiles();

        if (_workers.isEmpty())
        {
            _b_running = false;
        }
    }
}
//----------------------------
void    opScheduler::delete_all_radars()
{
    while (!_workers.isEmpty())
        delete_radar(_workers[0]->get_device());
}
//----------------------------
void    opScheduler::   delete_radar(radarInstance* device)
{
    if (!(_devices.contains(device))) return;
    for (auto& worker : _workers)
    {
        if (worker->get_device()==device)
        {
#ifdef RADAR_IN_THREAD
            QThread* corresponding_thread = worker->thread();
            _wthreads.removeAll(corresponding_thread);
            corresponding_thread->terminate();
            delete corresponding_thread;
#endif
            worker->halt();
            _workers.removeAll(worker);
            delete worker;
            return;
        }
    }
    _device_names.removeAll(device->get_device_name());
}
//----------------------------
void opScheduler::acquisition_loop()
{
    // Put all states into a vector
    QVector<SCHEDULER_STATUS> status;
    status.reserve(_workers.count());

    for (auto& worker: _workers)
        if (worker!=nullptr)
            status.append(worker->get_status());

    if (status.contains(INIT_SCRIPTS_DONE))
    {
        // Wait til all are SCRIPTS_DONE
        for (auto ss: status)
        { if (ss!=INIT_SCRIPTS_DONE) return;}
    }
    // Here we have all INIT_SCRIPTS_DONE
    if (status.contains(INIT_SCRIPTS_DONE))
    {
        if (is_timed())
        {
            _timer_done = false;
            _main_timer.start();
        }

        for (auto &worker:_workers)
        {
            worker->post_acquisition();
        }
        return;
    }

    if (status.contains(ACQUISTION_RESTART))
    {
        // Wait til all are SCRIPTS_DONE
        for (auto ss: status)
        { if (ss!=ACQUISTION_RESTART) return;}
    }

    // Here we have all ACQUISTION_RESTART
    if (status.contains(ACQUISTION_RESTART))
    {
        if (_run_scripts)
        {
            for (auto& script: _scripts)
            {

            }
            _run_scripts = false;
        }
        if ((is_timed())&&(!_timer_done)) return;

        if (is_timed())
        {
            _timer_done = false;
            _main_timer.start();
        }

        for (auto &worker:_workers)
        {            
            worker->post_acquisition();
        }
        return;
    }
}
//----------------------------
void opScheduler::timer_done()
{
    _timer_done=true;

    for (auto &worker:_workers)
        if ((worker!=nullptr)&&(worker->get_status()!=ACQUISITION_SCRIPTS_DONE))
            if (worker->get_status()!=HALT)
            {
                emit timeout_error();
                if (_timeout_policy==HALT_ON_TIMEOUT)
                {
                    emit halted(worker->get_device());
                    stop();
                    return;
                }
            }

    acquisition_loop();
    emit timing_ok();
}
//----------------------------
void opScheduler::initDone(opSchedulerOperations* op)
{
    for (auto& worker: _workers)
        if (worker->get_status()==INIT_SCRIPTS_DONE)
            emit init_done(worker->get_device());

    for (auto& worker: _workers)
        if (worker->get_status()!=INIT_SCRIPTS_DONE)
            return;
    // Go to pre-acquisition
    emit init_done_all();

    acquisition_loop();
}
//----------------------------
void opScheduler::initError(opSchedulerOperations* op)
{
    emit init_error(op->get_device());
    if (_policy_on_error == HALT_ALL)
    {
        stop();
        return;
    }

    if (_policy_on_error == CONTINUE_ON_ERROR)
        return;


    if (_policy_on_error == REINIT)
    {
        op->init_device();
        return;
    }

    if (_policy_on_error == HALT_DEVICE)
    {
        op->halt();
        delete_radar(op->get_device());
        return;
    }
}
//----------------------------
void opScheduler::postAcquisitionDone(opSchedulerOperations* op)
{
    for (auto& worker: _workers)
        if (worker->get_status()==ACQUISTION_RESTART)
            emit postacquisition_done(worker->get_device());

    for (auto& worker: _workers)
        if (worker->get_status()!=ACQUISTION_RESTART)
            return;

    emit postacquisition_done_all();

    acquisition_loop();
}
//----------------------------
void opScheduler::postAcquisitionError(opSchedulerOperations* op)
{
    emit postacquisition_error(op->get_device());
    if (_policy_on_error == HALT_ALL)
    {
        stop();
        return;
    }

    if (_policy_on_error == CONTINUE_ON_ERROR)
        return;


    if (_policy_on_error == REINIT)
    {
        op->init_device();
        return;
    }

    if (_policy_on_error == HALT_DEVICE)
    {
        op->halt();
        //delete_radar(op->get_device());
        return;
    }
}
//----------------------------
void opScheduler::device_connected(radarInstance* device)
{
    for (auto& worker: _workers)
        if (worker->get_device()->is_connected())
            emit connection_done(worker->get_device());

    for (auto& device: _devices)
        if (!device->is_connected())
            return;

    emit connection_done_all();
    // we may go thru init phase
    init_devices();
}

//----------------------------
void opScheduler::error_on_connection(radarInstance* device)
{
    emit connection_error(device);
}
//----------------------------
bool opScheduler::is_timed()
{
    return _cycle_time>0;
}

//----------------------------
void opScheduler::set_cycle_time(int milliseconds)
{
    _cycle_time = milliseconds;
    if (is_timed()) _main_timer.setInterval(_cycle_time);
}
//----------------------------
bool    opScheduler::save_xml(QDomDocument& document)
{
    QDomElement root = document.createElement("ARIA_Project_Scheduler");
    QVersionNumber xml_ver({XML_RADARFILE_MAJOR,XML_RADARFILE_MINOR,XML_RADARFILE_SUBVER});

    document.appendChild(root);

    QDomElement scheduler_node = document.createElement("scheduler");
    scheduler_node.setAttribute("cycle_time",_cycle_time);
    scheduler_node.setAttribute("serial_timeout",_serial_timeout);
    QString str_policy="";
    switch (_policy_on_error)
    {
    case HALT_ALL:
        str_policy = "halt_all";
        break;
    case HALT_DEVICE:
        str_policy = "halt_device";
        break;
    case CONTINUE_ON_ERROR:
        str_policy = "continue";
        break;
    case REINIT:
        str_policy= "reinit_device";
        break;
    default:
        str_policy = "halt_all";
    }

    scheduler_node.setAttribute("policy_on_error", str_policy);

    QString str_timeout = "";
    switch (_timeout_policy)
    {
    case CONTINUE_ON_TIMEOUT:
        str_timeout = "continue_on_timeout";
        break;
    case HALT_ON_TIMEOUT:
        str_timeout = "halt_on_timeout";
        break;
    default:
        str_timeout = "continue_on_timeout";
        break;
    }
    scheduler_node.setAttribute("policy_on_timeout",str_timeout);
    // Put active devices
    QDomElement active_devices = document.createElement("devices");

    QVector<radarInstance*> devices = get_root()->get_available_radars();
    QVector<radarInstance*> actual_devices;
    for (auto& worker: _workers)
    {
        if (worker!=nullptr)
            if (devices.contains(worker->get_device()))
                actual_devices.append(worker->get_device());
    }
    active_devices.setAttribute("count",actual_devices.count());
    for (auto& device: actual_devices)
    {
        QDomElement device_enabled = document.createElement("device");
        device_enabled.setAttribute("device_name",device->get_filename());
        active_devices.appendChild(device_enabled);
    }
    scheduler_node.appendChild(active_devices);
    // Save scripts
    QDomElement scheduler_scripts = document.createElement("scripts");

    QVector<octaveScript*>  available_scripts = get_root()->get_available_scripts();
    QVector<octaveScript*>  actual_scripts;

    for (auto script: _scripts)
    {
        if (script!=nullptr)
            if (available_scripts.contains(script))
                actual_scripts.append(script);
    }
    scheduler_scripts.setAttribute("count",actual_scripts.count());
    for (auto &script: actual_scripts)
    {
        QDomElement script_enabled = document.createElement("script");
        script_enabled.setAttribute("script_name",script->get_name());
        scheduler_scripts.appendChild(script_enabled);

    }
    scheduler_node.appendChild(scheduler_scripts);
    root.appendChild(scheduler_node);

    return true;
}
//----------------------------
bool    opScheduler::save_xml()
{
    QFile _rfile(get_full_filepath());
    if (!_rfile.open(QFile::WriteOnly | QFile::Text ))
    {
        qDebug() << "Already opened or permission issue";
        _rfile.close();
        return false;
    }
    QTextStream xmlContent(&_rfile);

    QDomDocument document;

    if (!save_xml(document)) {_rfile.close(); return false;}
    xmlContent << document.toString();

    _rfile.close();
    return true;

}
//----------------------------
bool    opScheduler::load_xml()
{
    radarProject* project = get_root();

    if (project==nullptr) return false;

    QFile _rfile(get_full_filepath());
    QDomDocument document;

    if (!_rfile.open(QIODevice::ReadOnly ))
    {
        return false;
    }
    document.setContent(&_rfile);
    _rfile.close();
    QDomElement root_main = document.firstChildElement("ARIA_Project_Scheduler");
    if (root_main.isNull()) return false;

    QDomElement root = root_main.firstChildElement("scheduler");
    if (root.isNull()) return false;

    delete_all_radars();
    clear_scripts();

    _cycle_time = root.attribute("cycle_time","0").toInt();
    _serial_timeout = root.attribute("serial_timeout","0").toInt();
    QString str_policy= root.attribute("policy_on_error","halt_all");


    if (str_policy == "halt_all")       set_policy_on_error(HALT_ALL);
    if (str_policy == "halt_device")    set_policy_on_error(HALT_DEVICE);
    if (str_policy == "continue")       set_policy_on_error(CONTINUE_ON_ERROR);
    if (str_policy == "reinit_device")  set_policy_on_error(REINIT);

    QString str_timeout = root.attribute("policy_on_timeout","continue_on_timeout");

    QVector<radarInstance*> available_devices = project->get_available_radars();
    if (str_timeout=="continue_on_timeout") set_policy_on_timeout(CONTINUE_ON_TIMEOUT);
    if (str_timeout=="halt_on_timeout")     set_policy_on_timeout(HALT_ON_TIMEOUT);
    // Load devices
    QDomElement devices = root.firstChildElement("devices");
    if (!devices.isNull())
    {
        //int expected_count = devices.attribute("count","0").toInt();
        QDomElement device_child = devices.firstChildElement("device");
        while (!device_child.isNull())
        {
            QString dev_name = device_child.attribute("device_name","");
            for (auto& available_radar : available_devices)
            {
                if (available_radar->get_filename() == dev_name)
                {
                    add_radar(available_radar);
                    break;
                }
            }
            device_child = device_child.nextSiblingElement("device");
        }
    }
    // script
    QDomElement scripts = root.firstChildElement("scripts");

    if (!scripts.isNull())
    {
        QDomElement script_node = scripts.firstChildElement("script");
        while (!script_node.isNull())
        {
            QString script_name = script_node.attribute("script_name","");
            octaveScript* script = (octaveScript*)(project->get_child(script_name,DT_SCRIPT));
            if (script!=nullptr)
            {
                _scripts.append(script);
            }
        }
    }

    return true;
}

//----------------------------
bool opScheduler::set_filename(QString filename, bool save)
{
    QFileInfo fi(filename);
    _filename = fi.fileName();

    if (filename.isEmpty())
        set_name("[noname]");
    else
        set_name(_filename);

    if (!save)
        return load_xml();
    else
        return true;
}
//----------------------------
bool    opScheduler::add_script(octaveScript* script)
{
    if (script==nullptr) return false;
    if (has_script(script)) return false;
    _scripts.append(script);
    return true;
}
//----------------------------
bool        opScheduler::has_script(octaveScript* script)
{
    return _scripts.contains(script);
}
//----------------------------
void        opScheduler::delete_script(octaveScript* script)
{
    _scripts.removeAll(script);
}
//----------------------------
void        opScheduler::clear_scripts()
{
    _scripts.clear();
}

//----------------------------
void        opScheduler::set_serial_timeout(int timeout)
{
    _serial_timeout = timeout;
    for (auto& worker: _workers)
    {
        worker->get_device()->set_serial_timeout(timeout);
    }

}
