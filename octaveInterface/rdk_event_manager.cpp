#include "rdk_event_manager.h"

rdk_event_manager::rdk_event_manager() {}


void rdk_event_manager::enter_debugger_event (const std::string& fcn_name,
                     const std::string& fcn_file_name,
                     int line)
{

   emit interperter_enter_dbevent(fcn_file_name, fcn_file_name,line-1);
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
