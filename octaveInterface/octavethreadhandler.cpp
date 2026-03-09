#include "octavethreadhandler.h"
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
#include "cmd-edit.h"
#include "builtin-defun-decls.h"


octaveThreadHandler::octaveThreadHandler(octaveInterface *owner) : QObject(),
    _oth_status(OTH_IDLE),
    _running_script(nullptr),
    _debug_script_fname(""),
    _debug_line(-1),
    _owner(owner),
    _octave_engine(nullptr)

{

}

octaveThreadHandler::~octaveThreadHandler()
{
//    engine_shutdown();
//    emit finished();
}

//-----------------------------
//-----------------------------
// Engine section
//-----------------------------
//-----------------------------


/**
 * @brief octaveThreadHandler::engine_init_and_start
 * Slot to initialize the octave engine INSIDE its own thread, rather than in the
 * main thread
 */
void octaveThreadHandler::engine_init_and_start()
{
    if (_owner==nullptr) return;
    if (_octave_engine!=nullptr) return;
    // Initialize Octave Interpreter

    _octave_engine = new octave::interpreter();
    _octave_engine->interactive(true);
    _octave_engine->initialize_history(true);
    _octave_engine->initialize_load_path();
    _octave_engine->initialize();
    _octave_engine->get_error_system().debug_on_caught(false);
    _octave_engine->get_evaluator().quiet_breakpoint_flag(false);
    _octave_engine->get_evaluator().server_mode(false);
//    _octave_engine->execute();

    // Setup output system

    octave::output_system& os = _octave_engine->get_output_system();

    os.flushing_output_to_pager(true);
    os.really_flush_to_pager(true);
    os.page_screen_output(false);

    //--------------------------------------------------------
    // Set-up an event manager
    _events = std::make_shared<rdk_event_manager>();

    octave::event_manager& evmgr = _octave_engine->get_event_manager ();
    evmgr.connect_link (_events);
    evmgr.enable ();
    connect(_events.get(), &rdk_event_manager::interperter_enter_dbevent,   this, &octaveThreadHandler::handle_interpreter_enter_debugger_event);
    connect(_events.get(), &rdk_event_manager::interpreter_dbstop,          this, &octaveThreadHandler::handle_interpreter_dbstop);
    connect(_events.get(), &rdk_event_manager::interpreter_dberror,         this, &octaveThreadHandler::handle_interpreter_dberror);
    //--------------------------------------------------------
    // This file is needed to intercept and interact with breakpoints
    _fdebug = fopen("dbg_in.txt","w+");
     octave::command_editor::set_input_stream(_fdebug);
    //--------------------------------------------------------

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
    return;
}
//-----------------------------
/**
 * @brief octaveThreadHandler::engine_reset_script
 * Reset the current script
 * @param script. If null, it is reset anyway
 */

void octaveThreadHandler::engine_reset_script(octaveScript* script)
{
    if (script==nullptr)
        _running_script=nullptr;
    else
        if (script==_running_script)
        _running_script = nullptr;

}
//-----------------------------
/**
 * @brief octaveThreadHandler::engine_shutdown
 * This must be called from the primary thread
 */

void  octaveThreadHandler::engine_shutdown()
{
    _debug_script_fname = "";
    _debug_line = -1;
    _running_script = nullptr;
    if (_octave_engine!=nullptr)
    {
        _octave_engine->cleanup_tmp_files();

        _octave_engine->get_output_system().reset();
        _octave_engine->clear_all();
        try {
            _octave_engine->quit(0,true,true);
        } catch (...) {
        }

        fclose(_fdebug);        
        _octave_engine = nullptr;
    }
}

//-----------------------------
//-----------------------------
// Execution section
//-----------------------------
//-----------------------------
/**
 * @brief octaveThreadHandler::execute_continue
 * @param script
 * @return
 */
void octaveThreadHandler::execute_continue(octaveScript* script)
{
    if ((_debug_script_fname != "")&&(_debug_script_fname!=script->get_fullfilename()))
        return;
    if ((_running_script != nullptr)&&(_running_script!=script))
        return;

    if (_octave_engine==nullptr) return;

    // We are starting a new execution
    if (_oth_status==OTH_IDLE)
    {
        {
            const QMutexLocker locker(&_sync);
            _oth_next_action = OTH_NA_CONTINUE;
        }

        _debug_script_fname = "";
        _debug_line = -1;
        _running_script = script;

        internal_execute_run(script, false);
        // We need
    }

    if (_oth_status==OTH_DEBUG)
    {
        // We just need to update the next_action flag since the
        // thread is in the wait cycle
        const QMutexLocker locker(&_sync);
        _oth_next_action = OTH_NA_CONTINUE;
    }

    return;

}
/**
 * @brief octaveThreadHandler::internal_execute_run
 * @param script
 * @param single_step
 */
void octaveThreadHandler::internal_execute_run(octaveScript* script, bool single_step)
{

    if (_octave_engine==nullptr) return;
    if (script == nullptr)       return;

    if (_oth_status!=OTH_IDLE)
        return;



    QString fname = script==nullptr? "ScriptDeleted":script->get_fullfilename();

    try
    {
        {
            //const QMutexLocker locker(&_sync);
            _oth_status     = OTH_RUNNING;
            _parse_result   = OTH_LR_UNKNOWN;
            _oth_next_action = OTH_NA_CONTINUE;
            _running_script = script;
            _debug_script_fname   = single_step ? script->get_fullfilename() : "";
            _debug_line = single_step ? 0 : 1;
        }

        emit signal_handler_dbrun(fname);
        bool b_parse_incomplete;
        if (single_step)
            _octave_engine->get_evaluator().reset_debug_state(true);
        _octave_engine->get_evaluator().quiet_breakpoint_flag(true);
        _octave_engine->get_evaluator().set_dbstep_flag(single_step ? 1:0);
        _octave_engine->get_evaluator().set_break_on_next_statement(single_step);
        _octave_engine->get_evaluator().quiet_breakpoint_flag(single_step);
        _owner->operation_wait_and_lock();
        _octave_engine->source_file(fname.toStdString(),"",false,true);
    }
    catch (const octave::exit_exception& ex)
    {
        {
            const QMutexLocker locker(&_sync);
            _oth_status     = OTH_NULL;
            _parse_result   = OTH_LR_EXIT;
            _debug_script_fname = "";
            _debug_line = -1;
            _running_script = nullptr;
        }
        _owner->operation_unlock();

        emit signal_handler_dberror(fname, "Octave abnormally exited", -1);

        return;
    }
    catch (const octave::execution_exception& ex)
    {
        {
            const QMutexLocker locker(&_sync);
            _oth_status     = OTH_IDLE;
            _parse_result   = OTH_LR_EXCEPTION;
            _debug_script_fname = "";
            _debug_line = -1;
            _owner->operation_unlock();

            _running_script = nullptr;

        }

        _octave_engine->get_error_system().save_exception(ex);
        QString str_error = QString::fromStdString(_octave_engine->get_error_system().last_error_message());

        emit signal_handler_dberror(fname, str_error, -1);
        return;
    }

    {
        const QMutexLocker locker(&_sync);
        _oth_status     = OTH_IDLE;
        _parse_result   = OTH_LR_OK;
        _running_script = nullptr;
        _debug_script_fname = "";
        _debug_line = -1;

    }
    _owner->operation_unlock();
    emit signal_handler_dbcomplete(fname);
}
//-----------------------------
/**
 * @brief octaveThreadHandler::execute_run
 * @param script
 * @return
 */
void octaveThreadHandler::execute_run(octaveScript* script)
{
    internal_execute_run(script, false);
    return;
}

/**
 * @brief octaveThreadHandler::execute_step_in
 * @param script
 * @return
 */
void octaveThreadHandler::execute_step_in(octaveScript* script)
{
    if ((_debug_script_fname != nullptr)&&(_debug_script_fname!=script->get_fullfilename()))
        return;
    if ((_running_script != nullptr)&&(_running_script!=script))
        return;
    if (_octave_engine==nullptr) return;

    // We are starting a new execution run with a single step to be done
    if (_oth_status==OTH_IDLE)
    {
        {
            const QMutexLocker locker(&_sync);
            _oth_next_action    = OTH_NA_STEP;
            _running_script     = script;
            _debug_script_fname = script->get_fullfilename();
            _debug_line = 0;
        }

        internal_execute_run(script, true);
        // We need
    }

    if (_oth_status==OTH_DEBUG)
    {
        // We just need to update the next_action flag since the
        // thread is in the wait cycle
        const QMutexLocker locker(&_sync);
        _oth_next_action = OTH_NA_STEP;
    }

    return;
}

void octaveThreadHandler::execute_stop(octaveScript* script)
{
    /*if ((_running_script != nullptr)&&(_running_script!=script))
        return;
    if ((_debug_script_fname != "")&&(_debug_script_fname!=script->get_fullfilename()))
        return;*/

    if (_octave_engine==nullptr) return;

    // We are starting a new execution run with a single step to be done
    if (_oth_status==OTH_RUNNING)
    {
        // Try setting this while the engine is in its internal cycle
        _octave_engine->get_evaluator().breaking(1);
    }

    if (_oth_status==OTH_DEBUG)
    {
        // We just need to update the next_action flag since the
        // thread is in the wait cycle
        const QMutexLocker locker(&_sync);
        _oth_next_action = OTH_NA_STOP;
    }

    return;


}

/**
 * @brief octaveThreadHandler::execute_step_out
 * @param script
 * @return
 */
void octaveThreadHandler::execute_step_out(octaveScript* script)
{
 // TBD
    return;
}

/**
 * @brief octaveThreadHandler::execute_feval
 * @param command
 * @param in
 * @param n_outputs
 * @return
 */
void octaveThreadHandler::execute_feval_ovl(QString command, octave_value_list& in, int n_outputs)
{

    _last_results.clear();
    _running_script       = nullptr;
    _debug_script_fname   = "";
    _debug_line = -1;
    if (_octave_engine==nullptr) return;

    if (_oth_status!=OTH_IDLE)
        return;
    const QMutexLocker locker(&_sync);

    try
    {
        _oth_status     = OTH_RUNNING;
        _parse_result   = OTH_LR_UNKNOWN;

        emit signal_handler_dbrun(command);
        _last_results = _octave_engine->feval(command.toLatin1(),in,n_outputs);

    }
    catch (const octave::exit_exception& ex)
    {
        _oth_status = OTH_NULL;
        _parse_result = OTH_LR_EXIT;

        emit signal_handler_dberror(command, "Octave abnormally exited", -1);
        return;
    }
    catch (const octave::execution_exception& ex)
    {
        _oth_status = OTH_IDLE;
        _parse_result = OTH_LR_EXCEPTION;

        _octave_engine->get_error_system().save_exception(ex);
        QString str_error = QString::fromStdString(_octave_engine->get_error_system().last_error_message());

        emit signal_handler_dberror(command, str_error, -1);
        return;
    }
    _oth_status = OTH_IDLE;
    _parse_result = OTH_LR_OK;

    handle_interpreter_dbcomplete(command);

    return;
}

//-----------------------------------------------------
/**
 * @brief execute_feval
 * @param command
 * @param input
 * @param output
 */
void    octaveThreadHandler::execute_feval(QString command, const string_vector& input, const string_vector& output)
{
    _running_script = nullptr;
    _debug_script_fname   = "";
    _debug_line = -1;
}
//-----------------------------------------------------
/**
 * @brief execute_eval_string
 * @param command
 * @return  0  on successful operation
 *          -1 if we have an execution error,
 *          -2 if we have an exit exception,
 *          -3 if the thread is missing
 */
void     octaveThreadHandler::execute_eval_string(QString command)
{
    _parse_result   = OTH_LR_UNKNOWN;
    _running_script = nullptr;
    _debug_script_fname   = "";
    _debug_line = -1;
    if (_owner==nullptr) { return; }

    try
    {
        octave::flush_stdout();
        // Update all workspace variables into the interpreter workspace
        _owner->workspace_update_interpreter_internal_vars();
        {
            const QMutexLocker locker(&_sync);
            _oth_status = OTH_RUNNING;
        }
        // Signal the start of the operation
        emit signal_handler_dbrun(command);

        int parse_status;
        _octave_engine->get_evaluator().set_break_on_next_statement(false);
        _owner->operation_wait_and_lock();

        _octave_engine->eval_string(command.toStdString(), false, parse_status);
    }
    catch (const octave::exit_exception& ex)
    {
        {
            const QMutexLocker locker(&_sync);
            _oth_status = OTH_IDLE;
            _parse_result= OTH_LR_EXIT;
        }
        _owner->operation_unlock();
        emit signal_handler_dberror(command, "Octave exited abnormally", -1);


        return;
    }
    catch (const octave::execution_exception& ex)
    {
        _octave_engine->get_error_system().save_exception(ex);
        QString str_error = QString::fromStdString(_octave_engine->get_error_system().last_error_message());

        const QMutexLocker locker(&_sync);
        {
            _oth_status = OTH_IDLE;
            _parse_result = OTH_LR_EXCEPTION;
        }
        _owner->operation_unlock();
        emit signal_handler_dberror(command, str_error, -1);
        return;
    }

    {
        const QMutexLocker locker(&_sync);
        _oth_status     = OTH_IDLE;
        _parse_result   = OTH_LR_OK;
    }
    emit signal_handler_dbcomplete(command);
}

//-----------------------------------------------------
/**
 * @brief octaveThreadHandler::handle_interpreter_dbstop
 * This procedure is called by the event handler of Octave API
 *
 * @param fname
 * @param line
 */
void            octaveThreadHandler::handle_interpreter_dbstop(const QString& fname, int line)
{

    {
        const QMutexLocker locker(&_sync);
        _oth_next_action = OTH_NA_WAIT;
        _oth_status      = OTH_DEBUG;
        _debug_script_fname = fname;
        _debug_line = line;
        _running_script = nullptr;
    }
    // We are getting here from a breakpoint.
    _owner->operation_unlock();
    emit signal_handler_dbstop(fname,line);

    octave::output_system& os = _octave_engine->get_output_system();

    while (_oth_next_action == OTH_NA_WAIT)
    {
        // Wait for 20ms
        //os.flush_stdout();
        QObject().thread()->usleep(20000);
        // and then inquiry for the next operation
    }
    _owner->operation_wait_and_lock();
    // The GUI signaled to execute a single step
    if (_oth_next_action == OTH_NA_STEP)
    {
        _octave_engine->get_evaluator().set_dbstep_flag(1);
    }

    if (_oth_next_action == OTH_NA_CONTINUE)
    {
        emit signal_handler_dbrun(fname);
        _octave_engine->get_evaluator().dbcont();
    }

    if (_oth_next_action == OTH_NA_STOP)
    {
        _octave_engine->get_evaluator().set_break_on_next_statement(true);
        _octave_engine->get_evaluator().breaking(1);
        _octave_engine->get_evaluator().returning(1);
        _octave_engine->get_evaluator().dbquit();
        _debug_script_fname = "";
        _running_script = nullptr;
        _debug_line = -1;

        emit signal_handler_dbcomplete(fname);
    }
}

//-----------------------------------------------------
/**
 * @brief octaveThreadHandler::handle_interpreter_dbrun
 * @param fname
 */
void            octaveThreadHandler::handle_interpreter_dbrun(const QString& fname)
{

}

//-----------------------------------------------------
/**
 * @brief octaveThreadHandler::handle_interpreter_dbcomplete
 * @param fname
 */
void            octaveThreadHandler::handle_interpreter_dbcomplete(const QString& fname)
{
    _running_script = nullptr;
    _debug_script_fname = "";
    _debug_line = -1;
}

//------------------------------------------------------
/**
 * @brief octaveThreadHandler::handle_interpreter_dberror
 * @param command
 * @param line
 */
void            octaveThreadHandler::handle_interpreter_dberror(const QString& command, int line)
{

}

//------------------------------------------------------
/**
 * @brief octaveThreadHandler::handle_interpreter_enter_debugger_event
 * @param fcn_name
 * @param fcn_file_name
 * @param line
 */
void            octaveThreadHandler::handle_interpreter_enter_debugger_event (const std::string& fcn_name,
                                             const std::string& fcn_file_name,
                                             int line)
{


    QString fname = QString::fromStdString(fcn_file_name);

    handle_interpreter_dbstop(fname,line);

}
