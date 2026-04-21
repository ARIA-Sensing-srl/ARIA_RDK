#include "rdk_event_manager.h"

rdk_event_manager::rdk_event_manager() {}


void rdk_event_manager::enter_debugger_event (const std::string& fcn_name,
                     const std::string& fcn_file_name,
                     int line)
{
#ifdef  _WIN32
    QString temp=QString::fromStdString(fcn_name);
    temp.replace(QString("\\"), QString("/"));
    _priv_fcn_name = temp.toStdString();
#else
    _priv_fcn_name = fcn_name;
#endif


   emit interperter_enter_dbevent(_priv_fcn_name, _priv_fcn_name,line-1);
}

void rdk_event_manager::execute_in_debugger_event (const std::string& file, int line)
{
    // Outside octave, everything is 0-based so we decrease line by 1
    //emit interpreter_dbstop(QString::fromStdString(file),line-1);
}

void rdk_event_manager::exit_debugger_event ()
{

}

void rdk_event_manager::update_breakpoint (bool insert, const std::string& file,
                  int line, const std::string& cond)
{

}


void rdk_event_manager::interpreter_interrupted ()
{
    //emit interpreter_dbstop("",0);
}

void rdk_event_manager::post_input_event()
{
    emit interpreter_post_input_event();
}


void rdk_event_manager::pre_input_event()
{
    emit interpreter_pre_input_event();
}