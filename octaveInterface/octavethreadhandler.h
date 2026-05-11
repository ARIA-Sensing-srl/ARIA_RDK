#ifndef OCTAVETHREADHANDLER_H
#define OCTAVETHREADHANDLER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <octave.h>
#include <ovl.h>
#include <octavews.h>
#include <event-manager.h>
#include "rdk_event_manager.h"
#include <ostream>

//----------------------------------------------
/**
 * @brief The octaveThreadHandler class handles the
 * interface to Octave as a separate thread. This is
 * to allow for pausing
 */

class octaveScript;
class octaveInterface;




class octaveThreadHandler : public QObject
{
    Q_OBJECT
public:
    typedef enum {OTH_NULL, OTH_IDLE, OTH_RUNNING, OTH_DEBUG} oth_status;
    typedef enum {OTH_LR_UNKNOWN, OTH_LR_OK, OTH_LR_EXCEPTION, OTH_LR_EXIT, OTH_LR_ERROR} oth_lr_status;

private:

    oth_status                           _oth_status;
    octave::interpreter                 *_octave_engine;    // This is the main _octave_engine
    std::shared_ptr<rdk_event_manager>  _events;
    octaveScript*                       _running_script;
    QString                             _debug_script_fname;
    int                                 _debug_line;
    octaveInterface*                    _owner;

    oth_lr_status                       _parse_result;      // Result's flag of the last operation
    octave_value_list                   _last_results;      // Numeric results of the last operation

    void                                delay();
    typedef enum {OTH_NA_WAIT, OTH_NA_STEP, OTH_NA_CONTINUE, OTH_NA_STOP} oth_next_action;
    oth_next_action                     _oth_next_action;   // Flag to tell what to do next, during a breakpoint wait cycle

    void                                internal_execute_run(octaveScript* script, bool single_step, bool skip_workspace_update);
    // We need to add a Mutex to avoid race during breakpoint events
    mutable QMutex                      _sync;
    // Debug pipe management
    FILE* input_write = nullptr;   // write end
    int   input_read_fd = -1;      // read end (per Octave)
    FILE* input_stream= nullptr;
    QString             _last_cmd = "";
    bool                _is_db_cmd = false;
    bool                _last_skip_workspace_update = false;


public:
    octaveThreadHandler(octaveInterface* owner=nullptr);
    ~octaveThreadHandler();
    // The following methods are defined as public slots so that
public slots:
    //----------------------------------------------------------------------------
    // Execution of scripts
    void                            execute_continue(octaveScript* script);
    void                            execute_run(octaveScript* script,bool skip_workspace_update = false);
    void                            execute_step_in(octaveScript* script);
    void                            execute_step_out(octaveScript* script);
    void                            execute_stop(octaveScript* script);
    void                            execute_feval_ovl(QString command, octave_value_list& in, int n_outputs);
    void                            execute_feval(QString command, const string_vector& input, const string_vector& output); // NB we may have empty in some output var
    void                            execute_eval_string(QString command);
    void                            execute_send_command_during_debug(const QString& cmd);
    void                            execute_send_reply_during_immediate(const QString& cmd);
    //----------------------------------------------------------------------------
    // Main octave engine
public slots:
    void                            engine_init_and_start();
public:
    inline octave::interpreter*     engine_get_octave_engine() {return _octave_engine;}

    bool                            engine_is_ready();
    void                            engine_set_pwd(const QString& path);
    int                             engine_find_func(QString funcName);
    octaveScript*                   engine_get_running_script() {return _running_script;}
    const QString&                  engine_get_debug_script()   {return _debug_script_fname;}
    int                             engine_get_debug_line()     {return _debug_line;}
    void                            engine_pause();
    oth_status                      engine_get_status() {return _oth_status;}
    void                            engine_reset_script(octaveScript* script);
    void                            engine_shutdown();

public slots:
    void            handle_interpreter_dbstop(const QString& fname, int line);
    void            handle_interpreter_dbrun(const QString& fname);
    void            handle_interpreter_dbcomplete(const QString& fname, bool skip_workspace_update);
    void            handle_interpreter_dberror(const QString& command, int line);
    void            handle_interpreter_enter_debugger_event (const std::string& fcn_name,
                                                 const std::string& fcn_file_name,
                                                 int line);
    void            handle_interpreter_post_input_event();
    void            handle_interpreter_pre_input_event();



signals:

    // Breakpoint management
    void            signal_handler_dbstop(const QString& fname, int line);
    void            signal_handler_dbstep_done(const QString& fname, int line);
    void            signal_handler_dbrun(const QString& fname);
    void            signal_handler_dbcomplete(const QString& fname, bool skip_workspace_update);
    void            signal_handler_dberror(const QString& fname, const QString& error, int line);
    void            signal_handler_cmd_complete_during_debug(const QString& cmd);
    void            finished();
    void            output_ready(const QString& text);

};

#endif // OCTAVETHREADHANDLER_H
