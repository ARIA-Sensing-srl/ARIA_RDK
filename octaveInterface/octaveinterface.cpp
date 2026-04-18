
/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "octaveinterface.h"
#include <octave.h>
#include <oct.h>
#include <parse.h>
#include <event-manager.h>
#include "octavescript.h"
#include <QFileInfo>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include "radarinstance.h"
#include "builtin-defun-decls.h"
#include "octavethreadhandler.h"
//#include "iostream"


octaveInterface*	octaveInterface::_octave_interface_instance = nullptr;
extern QString          m_current_directory;
/*-----------------------------
 * QSharedData implementation
 *----------------------------*/
/**
 * @brief octaveInterface::octaveInterface
 */
octaveInterface::octaveInterface() :
    _op_list()
    , _workspace(nullptr)
    , _immediate_filename("")
    , _current_device_owner(nullptr)
    , _locked(0)
{

    // Create the handler
    _octave_thread_handler = new octaveThreadHandler(this);
    // Create the thread and move the handler to the second thread
    _octave_thread = new QThread(this);
    _octave_thread_handler->moveToThread(_octave_thread);
    // start the thread in the "idle" mode
    _octave_thread->start();
    // create the connections to...
    create_thread_connections();
    // .. call for engine (aka the interpreter)
    emit signal_engine_init();

    _octave_engine = _octave_thread_handler==nullptr?nullptr : _octave_thread_handler->engine_get_octave_engine();

    // Create the workspace and link it to the interpreter
    _workspace = new octavews(_octave_engine);
    _workspace->data_interface(this);

    _op_current._op_type            = OIP_STRING;
    _op_current._operation.command  = nullptr;

    QString currentPath = QCoreApplication::applicationDirPath();
    QString	octavePath  = QFileInfo(currentPath, QString("../share/")).absolutePath()+"/";
    try
    {
        octave_value_list  pathList = _octave_engine->feval("genpath", std::list<octave_value>({charNDArray(octavePath.toUtf8())}));
        _octave_engine->feval("addpath", pathList);
    }
    catch(...)
    {
    }

    // Toolboxes
    QString pkgPrefix = QFileInfo(currentPath, QString("../octave/toolboxes/")).absolutePath();
    if (!QDir(pkgPrefix).exists())
        QDir().mkdir(pkgPrefix);

    try
    {
#ifdef WIN32
        QString xtmp= (QDir().toNativeSeparators(pkgPrefix)+QDir().separator());
        //xtmp = xtmp.last(xtmp.length()-2);
        std::list<octave_value> prefix({
                                        charNDArray(QString("prefix").toUtf8()),
                                        charNDArray(xtmp.toUtf8())});
#else
        std::list<octave_value> prefix({
                                        charNDArray(QString("prefix").toUtf8()),
                                        charNDArray((QDir().toNativeSeparators(pkgPrefix)+QDir().separator()).toUtf8())});
#endif

        _octave_engine->feval("pkg",octave_value_list(prefix));
    }
    catch(...)
    {
    }
#ifdef TOOLBOXES_IN_PATH
    // Pre-load directories in "toolboxes"
    QDirIterator directories(pkgPrefix, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    while(directories.hasNext()){
        directories.next();
        try
        {
            _octave_engine->feval("addpath", std::list<octave_value>({charNDArray(directories.filePath().toUtf8())}));
        }
        catch(...)
        {
        }
    }
#endif


}
//-----------------------------
/**
 * @brief octaveInterface::~octaveInterface
 */
octaveInterface::~octaveInterface()
{
    _octave_thread->quit();

    //delete _octave_thread;
    _octave_thread = nullptr;

    if (_workspace!=nullptr)
        delete _workspace;
}

void octaveInterface::create_thread_connections()
{
    if (_octave_thread_handler==nullptr) return;

    connect(_octave_thread, &QThread::finished, _octave_thread, &QObject::deleteLater, Qt::DirectConnection);
    // Connect signals to the thread handler
    connect(this, &octaveInterface::signal_execute_continue, _octave_thread_handler, &octaveThreadHandler::execute_continue);
    connect(this, &octaveInterface::signal_execute_run,      _octave_thread_handler, &octaveThreadHandler::execute_run);
    connect(this, &octaveInterface::signal_execute_step_in,  _octave_thread_handler, &octaveThreadHandler::execute_step_in);
    connect(this, &octaveInterface::signal_execute_step_out, _octave_thread_handler, &octaveThreadHandler::execute_step_out);
    connect(this, &octaveInterface::signal_execute_feval_ovl,_octave_thread_handler, &octaveThreadHandler::execute_feval_ovl);
    connect(this, &octaveInterface::signal_execute_feval,    _octave_thread_handler, &octaveThreadHandler::execute_feval);
    connect(this, &octaveInterface::signal_execute_stop,     _octave_thread_handler, &octaveThreadHandler::execute_stop);
    connect(this, &octaveInterface::signal_execute_command,  _octave_thread_handler, &octaveThreadHandler::execute_eval_string);
    connect(this, &octaveInterface::signal_engine_init,      _octave_thread_handler, &octaveThreadHandler::engine_init_and_start, Qt::DirectConnection);
    // Connected signals from the handler to this
    connect(_octave_thread_handler, &octaveThreadHandler::signal_handler_dbstop,        this, &octaveInterface::handle_octave_thread_dbstop);
    connect(_octave_thread_handler, &octaveThreadHandler::signal_handler_dbstep_done,   this, &octaveInterface::handle_octave_thread_dbstep_done);
    connect(_octave_thread_handler, &octaveThreadHandler::signal_handler_dbrun,         this, &octaveInterface::handle_octave_thread_dbrun);
    connect(_octave_thread_handler, &octaveThreadHandler::signal_handler_dbcomplete,    this, &octaveInterface::handle_octave_thread_dbcomplete);
    connect(_octave_thread_handler, &octaveThreadHandler::signal_handler_dberror,       this, &octaveInterface::handle_octave_thread_dberror);
    connect(_octave_thread_handler, &octaveThreadHandler::finished, _octave_thread_handler,   &octaveThreadHandler::deleteLater, Qt::DirectConnection);


}
//-----------------------------
//-----------------------------
// Engine section
//-----------------------------
//-----------------------------
/**
 * @brief octaveInterface::engine_is_ready
 * @return true iff the command queue is empty and we don't have a running script
 */
bool octaveInterface::engine_is_ready()
{
    return _octave_thread_handler == nullptr ? true : _octave_thread_handler->engine_get_status() == octaveThreadHandler::OTH_IDLE;
}
//-----------------------------
/**
 * @brief octaveInterface::engine_set_pwd
 * @param path
 */
void    octaveInterface::engine_set_pwd(const QString& path)
{
    if (_octave_engine==nullptr) return;
    _octave_engine->chdir(path.toStdString());
    m_current_directory = path;

}
//-----------------------------
/**
 * @brief octaveInterface::engine_find_func
 * @param funcName
 * @return 1 if the function exists as a builtin
 *
 */

int octaveInterface::engine_find_func(QString funcName)
{
    int retvalue=0;

    octave_value_list toFind(2);
    toFind(0)=funcName.toStdString();
    toFind(1)="builtin";
    octave_value_list ret = _octave_engine->feval("exist",toFind,1);
    retvalue = ret(0).int_value();

    return retvalue;
}
//-----------------------------
/**
 * @brief octaveInterface::engine_pause
 */
void octaveInterface::engine_pause()
{
    if (_octave_thread_handler!=nullptr)
        delete _octave_thread_handler;
}

int        octaveInterface::engine_get_debug_line()
{
    if (_octave_thread_handler==nullptr) return -1;
    return _octave_thread_handler->engine_get_debug_line();
}
//-----------------------------
/**
 * @brief octaveInterface::engine_get_debug_script
 * @return
 */
const QString& octaveInterface::engine_get_debug_script()
{
    return _octave_thread_handler->engine_get_debug_script();
}
//-----------------------------
/**
 * @brief octaveInterface::engine_get_running_script
 * @return The script currently running by the handler
 */
octaveScript* octaveInterface::engine_get_running_script()
{
    if (_octave_thread_handler==nullptr) return nullptr;
    return _octave_thread_handler->engine_get_running_script();
}
//-----------------------------
/**
 * @brief octaveInterface::engine_get_status
 * @return The status of the engine managed by the handler
 */
octaveThreadHandler::oth_status octaveInterface::engine_get_status()
{
    if (_octave_thread_handler==nullptr) return octaveThreadHandler::OTH_NULL;
    return _octave_thread_handler->engine_get_status();
}
//-----------------------------
/**
 * @brief octaveInterface::engine_reset_script
 * When a script is deleted, it calls for this procedure so that we can
 * signal the threadHandler that the script is no longer valid
 * @param script
 */
void    octaveInterface::engine_reset_script(octaveScript* script)
{
    if (_octave_thread_handler==nullptr) return;
    if (_octave_thread_handler->engine_get_running_script()!=script) return;

}
//-----------------------------
/**
 * @brief octaveInterface::engine_get_octave_engine
 * @return
 */
octave::interpreter*  octaveInterface::engine_get_octave_engine()
{
    //const QMutexLocker locker(&_sync);
    return _octave_engine;
}
//-----------------------------
//-----------------------------
// Operation section
//-----------------------------
//-----------------------------
/**
 * @brief octaveInterface::operation_append_command
 * Append a new command line to the queue of operations
 * @param newCommand
 * @return true if we may start operation immediately
 */

bool octaveInterface::operation_append_command(const QString &newCommand)
{
    _last_output = "";
    QString *op_string = new QString(newCommand);
    octintOperation new_op;
    new_op._op_type = OIP_STRING;
    new_op._operation.command = op_string;
    new_op._op_owner = nullptr;

    //const QMutexLocker locker(&_sync);
    _op_list.append(new_op);

    return operation_execute_next_in_queue();
}
//-----------------------------
/**
 * @brief octaveInterface::operation_append_script
 * Append a new script to the execution list. The execution starts immediately
 * @param script
 * @return true if we started a new operation
 */

bool octaveInterface::operation_append_script(octaveScript* script)
{
    _last_output = "";
    if (script==nullptr) return false;
    octintOperation new_op;
    new_op._op_type = OIP_SCRIPT;
    new_op._operation.script = script;
    new_op._op_owner = nullptr;
    //const QMutexLocker locker(&_sync);
    _op_list.append(new_op);

    return operation_execute_next_in_queue();
}
//-----------------------------
/**
 * @brief octaveInterface::operation_append_script_list
 * Append a list of scripts to the queue
 * @param script_list
 * @return
 */
bool octaveInterface::operation_append_script_list(const QVector<octaveScript*> script_list, class radarInstance* owner)
{
    _last_output = "";
    if (script_list.empty()) return false;

    for (auto script : script_list)
    {
        if (script!=nullptr)
        {
            //const QMutexLocker locker(&_sync);
            octintOperation new_op;
            new_op._op_type = OIP_SCRIPT;
            new_op._op_owner= owner;
            new_op._operation.script = script;
            _op_list.append(new_op);
        }
    }

    if (_op_list.empty()) return false;

    return operation_execute_next_in_queue();
}

//-----------------------------
/**
 * @brief octaveInterface::operation_execute_next_in_queue
 * Executes the next operation
 * @return
 */
bool octaveInterface::operation_execute_next_in_queue()
{
    // We may always execute a command from the command line, even if we have the code
    // halted @ some breakpoint (unless the engine is already running)

    if (_op_list.isEmpty())
    {
        emit signal_operation_all_ops_done();
        if (_current_device_owner!=nullptr)
            _current_device_owner->octave_engine_scripts_done();

        return true;
    }

    if (_octave_thread_handler==nullptr) return false;

    octintOperation op = _op_list.first();
    // update the current owner of the execution
    _current_device_owner = op._op_owner;

    if (op._op_type == OIP_STRING)
    {
        if (op._operation.command==nullptr) return false;

        QString str_command(*(op._operation.command));

        // The string was created during queuing so let's delete it
        delete op._operation.command;
        //const QMutexLocker locker(&_sync);
        _op_list.removeFirst();
        emit signal_execute_command(str_command);

    }
    if (op._op_type == OIP_SCRIPT)
    {

        octaveScript* script = op._operation.script;
        if (script==nullptr) return false;
        //const QMutexLocker locker(&_sync);
        _op_list.removeFirst();

        emit signal_execute_run(script);
    }

    return true;
}
//-----------------------------
/**
 * @brief octaveInterface::operation_device_is_deleting
 */
void    octaveInterface::operation_device_is_deleting(radarInstance* device)
{
    if (device==nullptr) return;
    //const QMutexLocker locker(&_sync);
    for (auto op : _op_list)
    {
        if ((op._op_type==OIP_SCRIPT)&&(op._op_owner==device))
        {
            ////const QMutexLocker locker(&_sync);
            op._op_owner=nullptr;
        }
    }
}
//-----------------------------
/**
 * @brief octaveInterface::operation_wait_and_lock
 * Wait for the octave interpreter to be available and lock it
 */
void    octaveInterface::operation_wait_and_lock()
{
    _sync.lock();
    _locked =1;
}
//-----------------------------
/**
 * @brief octaveInterface::operation_unlock
 * Unlock the operation
 */
void    octaveInterface::operation_unlock()
{
    if (_locked != 1) return;
    _sync.unlock();
    _locked = 0;
}

//-----------------------------
//-----------------------------
// Workspace section
//-----------------------------
//-----------------------------
//---------------------------------------
/**
 * @brief octaveInterface::workspace_clear
 * Clear the current workspace, including octave and RDK variables
 */
void	octaveInterface::workspace_clear()
{
    _octave_engine->clear_variables();
    _octave_engine->clear_global_variables();
    emit signal_workspace_updated();
}
//---------------------------------------
/**
 * @brief octaveInterface::workspace_append_var
 * @param name
 * @param val
 * @param internal
 */
void            octaveInterface::workspace_append_var(QString name, const octave_value& val, bool internal)
{
    if (_workspace == nullptr) return;
    if (name.isEmpty()) return;
    _workspace->add_variable(name.toStdString(), !internal, val);
    if (internal)
        _workspace->workspace_to_interpreter();
}
//---------------------------------------
/**
 * @brief octaveInterface::workspace_refresh
 */
void            octaveInterface::workspace_refresh()
{
    if (_workspace == nullptr) return;
    _workspace->workspace_to_interpreter();
}
//---------------------------------------
/**
 * @brief octaveInterface::workspace_update_interpreter_internal_vars
 */
void            octaveInterface::workspace_update_interpreter_internal_vars()
{
    if (_workspace==nullptr) return;
    _workspace->workspace_to_interpreter();
}

//---------------------------------------
/**
 * @brief octaveInterface::workspace_save_to_file
 * @param filename
 * @return
 */
bool    octaveInterface::workspace_save_to_file(QString filename)
{
    if (_workspace == nullptr) return false;
    _workspace->save_to_file(filename.toStdString());
    return true;
}
//-----------------------------
//-----------------------------
// Execution section
//-----------------------------
//-----------------------------
/**
 * @brief octaveInterface::execute_feval
 * @param command
 * @param in
 * @param n_outputs
 * @return
 * This function is equivalent to the "feval" function in octave API. It execute a command, with a list
 * of inputs and it returns the list of n_outputs (remember that octave has a variable length of output
 * too). If the engine thread is busy, return an empty OVL
 */
void octaveInterface::execute_feval(QString command, octave_value_list& in, int n_outputs)
{

    if (_octave_thread_handler==nullptr) return;

    if (_octave_thread_handler->engine_get_status()!=octaveThreadHandler::OTH_IDLE)
        return;

    emit signal_execute_feval_ovl(command,in,n_outputs);

    return;
}

//-----------------------------
/**
 * @brief octaveInterface::execute_continue
 * @param script
 * @return
 */
void octaveInterface::execute_continue(octaveScript* script)
{
    if (script == nullptr)
        return;

    if (_octave_engine==nullptr)
        return;
    _octave_engine->get_evaluator().server_mode(false);
    _octave_engine->get_evaluator().set_dbstep_flag(-1);
    _octave_engine->get_evaluator().set_break_on_next_statement(false);

    if (_octave_thread_handler->engine_get_status()==octaveThreadHandler::OTH_DEBUG)
    {
        _octave_thread_handler->execute_continue(script);
        return;
    }

    if (_octave_thread_handler->engine_get_status()==octaveThreadHandler::OTH_IDLE)
    {
        emit signal_execute_run(script);
        return;
    }


    return;
}
//---------------------------------------
/**
 * @brief octaveInterface::execute_run
 * @param script
 * @return
 */

void octaveInterface::execute_run(octaveScript* script)
{
    if (script == nullptr)
        return;

    if (_octave_engine==nullptr)
        return;

    if (_octave_thread_handler==nullptr)
        return;

    if (_octave_thread_handler->engine_get_status()!=octaveThreadHandler::OTH_IDLE)
        return;

    emit signal_execute_run(script);
}
//---------------------------------------
/**
 * @brief octaveInterface::execute_step_in
 * @param script
 * @return
 */

void            octaveInterface::execute_step_in(octaveScript* script)
{
    if (_octave_thread_handler==nullptr) return;
    if (_octave_engine==nullptr) return;
    octaveScript* prev = _octave_thread_handler->engine_get_running_script();
    if ((prev!=nullptr)&&(prev!=script)) return;

    if (_octave_thread_handler->engine_get_status()==octaveThreadHandler::OTH_DEBUG)
    {
        // In case of the OTH_DEBUG, the Thread is in the wait cycle so we need to modify the flag
        // from this thread

        _octave_thread_handler->execute_step_in(script);
    }
   if (_octave_thread_handler->engine_get_status()==octaveThreadHandler::OTH_IDLE)
    {

        emit signal_execute_step_in(script);
    }

}
//---------------------------------------
/**
 * @brief octaveInterface::execute_stop
 * @param script
 */

void octaveInterface::execute_stop(octaveScript* script)
{
    if (_octave_thread_handler==nullptr) return;
    if (_octave_engine==nullptr) return;
    // The interruption may be requested from any script
//    octaveScript* prev = _octave_thread_handler->engine_get_running_script();
//    if ((prev!=nullptr)&&(prev!=script)) return;

    if (_octave_thread_handler->engine_get_status()==octaveThreadHandler::OTH_DEBUG)
    {
        // In case of the OTH_DEBUG, the Thread is in the wait cycle so we need to modify the flag
        // from this thread

        _octave_thread_handler->execute_stop(script);
    }
    if (_octave_thread_handler->engine_get_status()==octaveThreadHandler::OTH_RUNNING)
    {
        // In case of the OTH_RUNNING, the octave engine is busy inside some calculations,
        // we'd need to stop it.
        // TBD Check if we need to reset this flags

        _octave_engine->get_evaluator().set_break_on_next_statement(true);


    }

}
//---------------------------------------
/**
 * @brief octaveInterface::execute_feval
 * @param command
 * @param input
 * @param output
 */

void            octaveInterface::execute_feval(QString command, const string_vector& input, const string_vector& output)
{
    if (_workspace==nullptr) return;
    // Build inputs
    octave_value_list in = _workspace->get_var_values(input);
    int nout = output.numel();
    _workspace->set_var_values(output,_octave_engine->feval(command.toLatin1(),in,nout));
}
/**
 * @brief octaveInterface::execute_eval_command Executes a string (e.g. coming from the command line)
 * @param command
 */

void    octaveInterface::execute_eval_command(QString command)
{
    operation_append_command(command);
}
//-----------------------------
//-----------------------------
// Radar-direct interface section
//-----------------------------
//-----------------------------
//---------------------------------------
/**
 * @brief octaveInterface::immediate_remove_interface_file
 */
void octaveInterface::immediate_remove_interface_file()
{
    if (_immediate_filename.empty()) return;
    QFileInfo fi(QString::fromStdString(_immediate_filename));
    if (fi.fileName()!=".rdk_tmp.atp") return;
    std::remove(_immediate_filename.c_str());
    _immediate_filename.clear();
}
//-----------------------------
/**
 * @brief octaveInterface::immediate_update_of_radar_var
 * @param str
 * @param filename
 */
void    octaveInterface::immediate_update_of_radar_var(const std::string& str, const std::string& filename)
{
    _immediate_filename = filename;
    if ((str.empty())||(filename.empty())) {immediate_remove_interface_file(); return;}

    if (!std::filesystem::exists(filename)) return;
    //_vars_immediate_update.insert(str);
    emit signal_immediate_update_radar_variable(str);
    // Delete lock file
    immediate_remove_interface_file();
}

//-----------------------------
/**
 * @brief octaveInterface::immediate_inquiry_of_radar_var
 * @param str
 * @param filename
 */
void    octaveInterface::immediate_inquiry_of_radar_var(const std::string& str, const std::string& filename)
{

    _immediate_filename = filename;
    if ((str.empty())||(filename.empty())) {immediate_remove_interface_file(); return;}

    // Add the variable to the list of modified vars
    //_vars_immediate_update.insert(str);
    if (!std::filesystem::exists(filename)) return;

    emit signal_immediate_inquiry_radar_variable(str);
    // Delete lock file
    immediate_remove_interface_file();
}
//-----------------------------
/**
 * @brief octaveInterface::immediate_command
 * @param str
 * @param filename
 */
void    octaveInterface::immediate_command(const std::string& str, const std::string& filename)
{
    _immediate_filename = filename;
    if ((str.empty())||(filename.empty())) {immediate_remove_interface_file(); return;}

    // Add the variable to the list of modified vars
    if (!std::filesystem::exists(filename)) return;

    emit signal_immediate_send_radar_command(str);
    // Delete lock file
    immediate_remove_interface_file();
}

//-----------------------------
//-----------------------------
// SLOTS: handle signals from
// interpreter
//-----------------------------
//-----------------------------
//-----------------------------
/**
 * @brief octaveInterface::handle_interpreter_dbcomplete
 * @param fname
 */
void  octaveInterface::handle_octave_thread_dbcomplete(const QString& fname)
{
    {
        //const QMutexLocker locker(&_sync);

        // Transfer data to the workspace
        if (_workspace!=nullptr)
            _workspace->interpreter_to_workspace();

        // Announce the update of the workspace
        emit signal_workspace_updated();

        // This is a signal that goes into EVERY slot.
        emit signal_interface_execute_dbcomplete(fname);

        // We have completed a script of an owner. If we don't have any further left from
        // the same owner, we may signal that we are done with it.
        if (_current_device_owner!=nullptr)
        {
            bool b_any_left = false;
            for (auto left: _op_list)
            {
                if ((left._op_type==OIP_SCRIPT)&&(left._op_owner==_current_device_owner))
                {
                    b_any_left = true;
                    break;
                }
            }
            //
            if (!b_any_left)
                _current_device_owner->octave_engine_scripts_done();
        }
    }


    // Go with the next operation (if any)
    if (!_op_list.empty())
        operation_execute_next_in_queue();
}
//-----------------------------
/**
 * @brief octaveInterface::handle_interpreter_dberror
 * @param fname
 * @param line
 */
void  octaveInterface::handle_octave_thread_dberror(const QString& fname, const QString& error,int line)
{
    emit signal_interface_execute_dberror(fname,error,line);
}
//-----------------------------
/**
 * @brief octaveInterface::handle_interpreter_dbstop
 * @param fname
 * @param line
 */

void octaveInterface::handle_octave_thread_dbstop(const QString& fname, int line)
{
    // This function signal to the UX the situation of the octave which is in
    // stand-by waiting for next step
    // Transfer data to the workspace
    if (_workspace!=nullptr)
        _workspace->interpreter_to_workspace();

    // Announce the update of the workspace
    emit signal_workspace_updated();

    //
    emit signal_interface_execute_dbstop(fname,line);
}

/**
 * @brief octaveInterface::handle_octave_thread_dbrun
 * @param fname
 */
void  octaveInterface::handle_octave_thread_dbrun(const QString& fname)
{
    emit signal_interface_execute_dbrun(fname);
}
/**
 * @brief octaveInterface::handle_octave_thread_dbstep_done
 * @param fname
 * @param line
 */
void octaveInterface::handle_octave_thread_dbstep_done(const QString& fname, int line)
{

}



