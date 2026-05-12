/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef OCTAVEINTERFACE_H
#define OCTAVEINTERFACE_H

#include <QObject>
#include <QSharedDataPointer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <octave.h>
#include <ovl.h>
#include <octavews.h>
#include <event-manager.h>
#include <octavethreadhandler.h>


#undef OCTAVE_THREAD

enum octintOperationType{OIP_STRING, OIP_SCRIPT};

typedef struct {
    octintOperationType       _op_type;
    bool                      _b_hide_feedback;
    class radarInstance*      _op_owner;    // When adding a list of commands, we add the owner so that we can notify it
                                            // when we are done with the entire sequence
    union
    {
        octaveScript* script;
        QString*      command;
    } _operation;
} octintOperation;



class octaveScript;

class octaveInterface : public QObject
{
    Q_OBJECT
private:
    QList<octintOperation>              _op_list;           // This is the list of operation to be executed (we may mix commands and scripts)
    octintOperation                     _op_current;
    QString                             _last_output;

    std::string                         _immediate_filename;
	QString								_last_error;
	QString								_last_warning;

    octaveThreadHandler*                _octave_thread_handler;
    QThread*                            _octave_thread;
    octave::interpreter*                _octave_engine;
    mutable QMutex                       _sync;
    int                                 _locked;

    void                                create_thread_connections();
    class radarInstance*                _current_device_owner;
    QString                             _last_mutex_owner ="";
    bool                                _is_in_immediate_cycle = false;
    class   radarProject*               _owner_project = nullptr;

public:
    octaveInterface();
    ~octaveInterface();
    //----------------------------------------------------------------------------
    // Operations
    bool                            operation_append_command(const QString &strCommand);
    bool                            operation_append_script(octaveScript* script, bool hide_history = false);
    bool                            operation_append_script_list(const QVector<octaveScript*> script_list, class radarInstance* owner=nullptr);
    bool                            operation_execute_next_in_queue();
    const QString &                 operation_get__last_output() {return _last_output;}
    void                            operation_device_is_deleting(class radarInstance* device);
    void                            operation_wait_and_lock(const QString& fname="");
    void                            operation_unlock(const QString& fname = "");
    void                            operation_trylock();
    void                            operation_set_radar_project(class radarProject* prj = nullptr) {_owner_project = prj;}
    bool                            operation_queue_is_empty() {return _op_list.isEmpty();}
    //----------------------------------------------------------------------------
    // Main octave engine
    octave::interpreter*            engine_get_octave_engine();
    bool                            engine_is_ready();
    void                            engine_set_pwd(const QString& path);
    int                             engine_find_func(QString funcName);
    octaveScript*                   engine_get_running_script();
    int                             engine_get_debug_line();
    const QString&                  engine_get_debug_script();
    void                            engine_pause();
    octaveThreadHandler::oth_status engine_get_status();
    void                            engine_reset_script(octaveScript* script);
    bool                            engine_is_in_immediate_cycle() {return ! _immediate_filename.empty();}

    //----------------------------------------------------------------------------
    // Execution of scripts
    void                            execute_pause(octaveScript* script);
    void                            execute_continue(octaveScript* script);
    void                            execute_run(octaveScript* script);
    void                            execute_step_in(octaveScript* script);
    void                            execute_stop(octaveScript* script=nullptr);
    void                            execute_step_out(octaveScript* script);
    void                            execute_feval(QString command, octave_value_list& in, int n_outputs);
    void                            execute_feval(QString command, const string_vector& input, const string_vector& output); // NB we may have empty in some output var
    void                            execute_eval_command(QString command);
    //----------------------------------------------------------------------------
    // General
    void                            set_last_error_message(QString error = "");
    //----------------------------------------------------------------------------
    //* Workspace commands
    //----------------------------------------------------------------------------
    void                            workspace_clear();
    void                            workspace_append_var(QString name, const octave_value& val);
    bool                            workspace_save_to_file(QString filename);
    void                            variable_set_value(const std::string& varname, const octave_value& val);
    octave_value                    variable_get_value(const std::string& varname, int& context); // Context = 0 (global), 1 (top), 2 ("local"?)
    octave_value                    variable_get_value(const std::string& varname);
    bool                            variable_exists(const std::string& varname);
    QStringList                     variable_get_names();
    //----------------------------------------------------------------------------
    //* Immediate update of radar variables
    //----------------------------------------------------------------------------
    void                            immediate_update_of_radar_var(const std::string& str, const std::string& filename);
    void                            immediate_inquiry_of_radar_var(const std::string& str, const std::string& filename);
    void                            immediate_command(const std::string& str, const std::string& filename);
    void                            immediate_abort();
    void                            immediate_done_success();

public slots:
    void            handle_octave_thread_dbstop(const QString& fname, int line);
    void            handle_octave_thread_dbstep_done(const QString& fname, int line);
    void            handle_octave_thread_dbrun(const QString& fname);
    void            handle_octave_thread_dbcomplete(const QString& fname, bool skip_workspace_update);
    void            handle_octave_thread_dberror(const QString& command, const QString& error,int line);
    void            handle_octave_thread_cmd_debug_done(const QString& cmd);
signals:
    void            signal_workspace_updated();
    void            signal_workspace_deleted();
    void            signal_workspace_cleared();
    void            signal_workspace_var_added();
    void            signal_workspace_var_modified();
    void            signal_workspace_var_deleted();
    void            signal_execution_error();
    void            signal_updated_variable(const std::string& var);
    void            signal_updated_variables(const std::set<std::string>& vars);
    void            signal_immediate_update_radar_variable(const std::string& var);
    void            signal_immediate_inquiry_radar_variable(const std::string& var);
    void            signal_immediate_send_radar_command(const std::string& var);

    // Signals for the thread handler
    void            signal_execute_continue(octaveScript* script);
    void            signal_execute_run(octaveScript* script, bool hide_from_feedback);
    void            signal_execute_step_in(octaveScript* script);
    void            signal_execute_step_out(octaveScript* script);
    void            signal_execute_stop(octaveScript* script);
    void            signal_execute_feval_ovl(QString command, octave_value_list& in, int n_outputs);
    void            signal_execute_feval(QString command, const string_vector& input, const string_vector& output); // NB we may have empty in some output var
    void            signal_execute_command(const QString& command);
    void            signal_engine_init();

    void            signal_operation_all_ops_done(); // The queue is empty


    // Breakpoint management (for higher level management)
    void            signal_interface_execute_dbstop(const QString& fname, int line);
    void            signal_interface_execute_dbrun(const QString& fname);
    void            signal_interface_execute_dbcomplete(const QString& fname, bool skip_history_update);
    void            signal_interface_execute_dberror(const QString& fname, const QString& error, int line);
    void            signal_interface_execute_dbbusy(const QString& commandname);
    void            signal_interface_execute_command_debug_done(const QString& cmd);

};

#endif // OCTAVEINTERFACE_H
