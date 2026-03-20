#ifndef RDK_EVENT_MANAGER_H
#define RDK_EVENT_MANAGER_H
#include <QObject>
#include <event-manager.h>

class rdk_event_manager : public QObject, public octave::interpreter_events
{
    Q_OBJECT
public:
    rdk_event_manager();

    void
    enter_debugger_event (const std::string& /*fcn_name*/,
                         const std::string& /*fcn_file_name*/,
                         int /*line*/);

    void
    execute_in_debugger_event (const std::string& /*file*/, int /*line*/);

    void exit_debugger_event ();

    void
    update_breakpoint (bool /*insert*/, const std::string& /*file*/,
                      int /*line*/, const std::string& /*cond*/);


    void interpreter_interrupted ();
signals:
    void            interperter_enter_dbevent(const std::string& fcn_name,
                                   const std::string& fcn_file_name,
                                   int line);
    void            interpreter_dbstop(const QString& fname, int line);
    void            interpreter_dberror(const QString& fname, int line);
private:
    std::string     _priv_fcn_name; // in Windows we have to translate the filename from "c:\a\b\f.m" to "c:/a/b/f.m" before signaling any event
};

#endif // RDK_EVENT_MANAGER_H
