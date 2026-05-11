/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "octavescript.h"
#include <QFileInfo>
#include <QDir>
#include "interpreter.h"
#include "file-ops.h"

octaveScript::octaveScript(QString filename, projectItem* parent) : projectItem("newScript",DT_SCRIPT, parent),
    _text(),
    _script_file(),
    _octave_interface(nullptr),
    b_need_parsing(true),
    _debug_line(-1)
{

    set_filename(filename,false);
}
//------------------------------------
octaveScript::octaveScript() : projectItem("newScript",DT_SCRIPT),
    _text(),
    _script_file(),
    _octave_interface(nullptr),
    _file_existing(false),
    b_need_parsing(true),
    _debug_line(-1)
{

}
//------------------------------------
octaveScript::octaveScript(const octaveScript& script) : projectItem(script.get_name(), DT_SCRIPT)
{
    operator = (script);
}
//------------------------------------
octaveScript::~octaveScript()
{
    if (_script_file.isOpen())
        _script_file.close();

    // Check if we are running this file. In case, terminate the execution
    if (_octave_interface!=nullptr)
        if (_octave_interface->engine_get_octave_engine()!=nullptr)
        {
            if ((_octave_interface->engine_get_running_script()!=nullptr)&&
                (_octave_interface->engine_get_running_script()==this))
            {
                do_request_stop();
                // Set the current script as null
                _octave_interface->engine_reset_script(this);
            }
        }
}
//------------------------------------
const octaveScript& octaveScript::operator = (const octaveScript& script)
{
    projectItem::operator=(script);
    _text = script._text;
    _script_file.setFileName(script._script_file.fileName());

    _octave_interface = script._octave_interface;
    _debug_line = script._debug_line;
    return *this;
}


void octaveScript::set_filename(QString filename, bool save_nload)
{
    QFileInfo fi(filename);
    _filename = fi.fileName();
    _script_file.setFileName(filename);
    if (filename.isEmpty())
        set_name("[noname]");
    else
        set_name(_filename);
    if (save_nload)
        save();
    else
        load();
}



//------------------------------------
bool    octaveScript::save_xml(QDomDocument& owner, QDomElement& script_elem)
{
    QDomElement element = owner.createElement("script");
    element.setAttribute("script_name",get_name());
    element.setAttribute("filename",_filename);
    script_elem.appendChild(element);
    return true;
}
//------------------------------------
octaveScript*  octaveScript::load_xml(QDomDocument& owner, QDomElement& root)
{
    octaveScript* out = nullptr;

    if (root.isNull())
        root = owner.firstChildElement("script");
    if (root.isNull())
        return nullptr;
    return nullptr;
    out->set_name(root.attribute("script_name","undefined"));
    out->_filename = (root.attribute("filename",""));
    out->_script_file.setFileName(out->get_full_filepath());
    root = root.nextSiblingElement();
    return out;
}
//------------------------------------
void  octaveScript::load()
{
    _text.clear();

    if (_script_file.exists())
    {
        if (!_script_file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        set_text(_script_file.readAll());
        _script_file.close();
        _file_existing = true;
    }
    else
        _file_existing = false;

}
//------------------------------------
QString octaveScript::get_text()
{
    return _text;
}
//------------------------------------
void octaveScript::set_text(const QString& input_text)
{
    QString prev(_text);

    _text = input_text;
    if (prev!=_text)
    {
        b_need_parsing = true;
        octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->engine_get_octave_engine();
        if (interp!=nullptr)
        {
            _octave_interface->operation_wait_and_lock("set_text");
            // If the text has been changed, we need to tell Octave that we need to parse again
            // the source file.
            octave::tree_evaluator& tw = interp->get_evaluator ();
            try
            {
                octave_user_code* code = tw.get_user_code(QFileInfo(get_fullfilename()).baseName().toStdString());
                if (code!=nullptr) code->mark_fcn_file_up_to_date(static_cast<OCTAVE_TIME_T> (0));
            }
            catch(...)
            {
            }
            _octave_interface->operation_unlock("set_text");
        }
    }
}

//------------------------------------
void octaveScript::setValid(bool bValid)
{
    _file_existing = bValid;
    projectItem* p = get_root();
    if (p!=nullptr)
        if (p->get_type()==DT_ROOT)
            emit ((radarProject*)p)->item_updated((projectItem*)(this));

}

//------------------------------------
void    octaveScript::save()
{
    //b_need_parsing = true;
    if (!_script_file.fileName().isEmpty())
    {
        if (!_script_file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream out(&_script_file);
        out << _text;
        _script_file.close();
        setValid();
    }
}
//------------------------------------
void    octaveScript::attach_to_dataengine(octaveInterface* octint)
{
    _octave_interface= octint;

    connect(octint, &octaveInterface::signal_interface_execute_dbrun,     this, &octaveScript::do_interpreter_dbrun);
    connect(octint, &octaveInterface::signal_interface_execute_dbstop,    this, &octaveScript::do_interpreter_dbstop);
    connect(octint, &octaveInterface::signal_interface_execute_dbcomplete,this, &octaveScript::do_interpreter_dbcomplete);
    connect(octint, &octaveInterface::signal_interface_execute_dberror,   this, &octaveScript::do_interpreter_error);
}
//------------------------------------
octaveInterface*    octaveScript::get_octave_interface()
{
    return _octave_interface;
}
//------------------------------------
bool    octaveScript::operator == (octaveScript script)
{
    return (_script_file.fileName()==script._script_file.fileName());
}
//-----------------------------------------------------
//-----------------------------------------------------
// Breakpoint management
//-----------------------------------------------------
//-----------------------------------------------------
/**
 * @brief octaveScript::breakpoint_toggle
 * @param line (0 based)
 * @param cond
 * @return
 */
int  octaveScript::breakpoint_toggle(int line, const QString& cond)
{
    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->engine_get_octave_engine();
    if (interp==nullptr) return -1;
    _octave_interface->operation_wait_and_lock("breakpoint_toggle");
    octave::tree_evaluator& tw = interp->get_evaluator ();
    octave::bp_table& bptab = tw.get_bp_table ();


    std::string fname = get_fullfilename().toStdString();
    octave::bp_table::fname_bp_map previous_bp = bptab.get_breakpoint_list(octave_value_list(fname));


    // Get the proper breakpoint line
    int lineeq = bptab.add_breakpoint_in_file( fname, line+1, cond.toStdString());

    bool bp_was_there = false;    

    for (auto& bp_list : previous_bp)
        if (bp_list.first == fname)
        {
            for (auto bp =  bp_list.second.begin(); bp != bp_list.second.end(); bp++)
            {

                if ((bp->line)==lineeq)
                {
                    bp_was_there = true;
                    break;
                }
            }
            if (bp_was_there) break;
        }
    // If the breakpoint was already there, remove it from the file.
    if (bp_was_there) bptab.remove_breakpoint_from_file(fname, lineeq);
    _octave_interface->operation_unlock("breakpoint_toggle");
    return lineeq-1;
}
//--------------------------------------
/**
 * @brief octaveScript::breakpoint_get_line
 * Get the actual line for a breakpoint. This means the breakpoint is going to be placed onto the first valid
 * line at or after the intended position
  * @param line (0 based)
 * @return the line where the breakpoint
 */

int octaveScript::breakpoint_get_line(int line)
{
    std::string fname = get_fullfilename().toStdString();
    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->engine_get_octave_engine();
    if (interp==nullptr) return -1;
    _octave_interface->operation_wait_and_lock("breakpoint_get_line");

    octave::tree_evaluator& tw = interp->get_evaluator ();

    octave::bp_table& bptab = tw.get_bp_table ();

    octave::bp_table::fname_bp_map previous_bp = bptab.get_breakpoint_list(octave_value_list(fname));

       // Get the proper breakpoint line
    int lineeq;
    try
    {
        lineeq = bptab.add_breakpoint_in_file( fname, line+1, std::string(""));
    }
    catch(...)
    {
        lineeq = 0;
    }

    bool bp_was_there = false;

    for (auto& bp_list : previous_bp)
        if (bp_list.first == fname)
        {
            for (auto bp =  bp_list.second.begin(); bp != bp_list.second.end(); bp++)
            {

                if ((bp->line)==lineeq)
                {
                    bp_was_there = true; break;
                }
            }
            if (bp_was_there) break;
        }

    try
    {
    // If the breakpoint wasn't already present, it is removed from the file
    if (!bp_was_there) bptab.remove_breakpoint_from_file(fname, lineeq);
    }
    catch(...)
    {
    }
    _octave_interface->operation_unlock("breakpoint_get_line");
    return lineeq-1;

}
//--------------------------------------
/**
 * @brief octaveScript::breakpoint_add
 * @param line
 * @param cond
 * @return
 */
int octaveScript::breakpoint_add (int line, const QString& cond)
{

    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->engine_get_octave_engine();
    if (interp==nullptr) return -1;

    octave::tree_evaluator& tw = interp->get_evaluator ();
    octave::bp_table& bptab = tw.get_bp_table ();
    std::string fname = get_fullfilename().toStdString();

    // Get the proper breakpoint line
    _octave_interface->operation_wait_and_lock("breakpoint_add");
    int lineeq ;
    try
    {
        lineeq = bptab.add_breakpoint_in_file( fname, line+1, cond.toStdString());
    }
    catch(...)
    {
    }
    _octave_interface->operation_unlock("breakpoint_add");



    return lineeq-1;
}

//--------------------------------------
/**
 * @brief octaveScript::breakpoint_remove
 * Remove the breakpoint at the first available line
 * @param line (1 based)
 * @return
 */

int octaveScript::breakpoint_remove (int line)
{
    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->engine_get_octave_engine();
    if (interp==nullptr) return -1;

    octave::tree_evaluator& tw = interp->get_evaluator ();
    octave::bp_table& bptab = tw.get_bp_table ();

    // Let's see if we have a precise location.
    int lineeq = -1;
    _octave_interface->operation_wait_and_lock("breakpoint_remove");
    try
    {
        bptab.remove_breakpoint_from_file (get_fullfilename().toStdString (), line+1);
    }
    catch(...)
    {
    }
    _octave_interface->operation_unlock("breakpoint_remove");
    return lineeq-1;
}
//--------------------------------------
/**
 * @brief octaveScript::breakpoint_remove_all
 */

void octaveScript::breakpoint_remove_all()
{
    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->engine_get_octave_engine();
    if (interp==nullptr) return;
    octave::tree_evaluator& tw = interp->get_evaluator ();
    octave::bp_table& bptab = tw.get_bp_table ();
    std::string fname = get_fullfilename().toStdString();
    _octave_interface->operation_wait_and_lock("breakpoint_remove_all");
    octave::bp_table::fname_bp_map bp_file = bptab.get_breakpoint_list(octave_value_list(fname));

    bool bp_was_there = false;

    for (auto& bp_list : bp_file)
        if (bp_list.first == fname)
        {
            if (bp_list.second.size()>0)
            {
                bp_was_there = true;
                break;
            }
        }
    try
    {
    if (bp_was_there)
        bptab.remove_all_breakpoints_from_file(get_fullfilename().toStdString (),
                                               true);
    }
    catch(...)
    {
    }

    _octave_interface->operation_unlock("breakpoint_remove_all");
}

//--------------------------------------
/**
 * @brief octaveScript::breakpoint_at_line
 * Check if we have a breakpoint
 * @param line (0 based)
 * @param lineeq ( based)
 * @return
 */

bool octaveScript::breakpoint_at_line(int line, int& lineeq)
{

    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->engine_get_octave_engine();
    if (interp==nullptr) return false;
    line++;

    std::string fname = get_fullfilename().toStdString();
    octave::tree_evaluator& tw = interp->get_evaluator ();
    octave::bp_table& bptab = tw.get_bp_table ();
    octave::bp_table::fname_bp_map previous_bp = bptab.get_breakpoint_list(octave_value_list(fname));

    // Try adding the breakpoint to see which line this is falling into.    

    // Get the proper breakpoint line
    _octave_interface->operation_wait_and_lock("breakpoint_at_line");
    try
    {
        lineeq = bptab.add_breakpoint_in_file( fname, line, "");
    }
    catch(...)
    {
        lineeq=0;
    }

    bool bp_was_there = false;

    for (auto& bp_list : previous_bp)
        if (bp_list.first == fname)
        {
            for (auto bp =  bp_list.second.begin(); bp != bp_list.second.end(); bp++)
            {
                if ((bp->line)==lineeq)
                {
                    bp_was_there= true; break;
                }
            }

            if (bp_was_there) break;
        }

    try
    {
        if (!bp_was_there) bptab.remove_breakpoint_from_file(fname,lineeq);
    }
    catch(...)
    {
    }
    _octave_interface->operation_unlock("breakpoint_at_line");
    lineeq--;
    return bp_was_there;
}


//------------------------------------
/**
 * @brief octaveScript::breakpoints_has_any
 * @return
 */
bool octaveScript::breakpoints_has_any()
{
    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->engine_get_octave_engine();
    if (interp==nullptr) return false;
    _octave_interface->operation_wait_and_lock("breakpoints_has_any");
    octave::tree_evaluator& tw = interp->get_evaluator ();
    octave::bp_table& bptab = tw.get_bp_table ();

    // Try adding the breakpoint to see which line this is falling into.
    std::string fname = get_fullfilename().toStdString();
    octave_value_list bp_list;
    bp_list.append(charNDArray(fname));

    octave::bp_table::fname_bp_map bpmap = bptab.get_breakpoint_list(bp_list);
    _octave_interface->operation_unlock("breakpoints_has_any");

    for (auto& bp_list : bpmap)
        if (bp_list.first == fname)
        {
            if (bp_list.second.size() > 0) return true;
        }
    return false;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
// These are the slot for the signals coming from the octaveInterface
//------------------------------------------------------------------------
//------------------------------------------------------------------------
/**
 * @brief octaveScript::do_interpreter_dbstop
 * Handle the signal coming from the octaveInterface. The octave engine is
 * stopped at some breakpoint. Discard if it is another script's breakpoint
 * @param fname
 * @param line
 */
void octaveScript::do_interpreter_dbstop(const QString& fname, int line)
{

    _debug_line = line;

    emit script_stop(fname, line);

}
//------------------------------------------------------------------------
/**
 * @brief octaveScript::do_interpreter_dbrun
 * The octave Interface signaled that the engine in running a certain script/command
 * @param fname
 */
void octaveScript::do_interpreter_dbrun(const QString& fname)
{
    _debug_line = -1;
    emit script_run(fname);
}
//------------------------------------------------------------------------
/**
 * @brief octaveScript::do_interpreter_dbcomplete
 * @param fname
 */
void octaveScript::do_interpreter_dbcomplete(const QString& fname, bool skip_workspace_update)
{
    _debug_line = -1;
    emit script_run_complete(fname, skip_workspace_update);
}
//------------------------------------------------------------------------
/**
 * @brief octaveScript::do_interpreter_error
 * @param fname
 * @param error
 * @param line
 */

void octaveScript::do_interpreter_error(const QString& fname, const QString& error, int line)
{
    _debug_line = -1;
    emit script_error(fname, error,line);
}
//------------------------------------------------------------------------
//------------------------------------------------------------------------
// Slots for management of the GUI requests
void octaveScript::do_request_pause()
{
    /* TBD
    // We cannot pause the engine if it is not running THIS script
    if (_octave_interface==nullptr) return;

    if (_octave_interface->engine_get_running_script()!=this) return;

    if (_octave_interface->engine_get_status()!=IS_RUNNING) return;

    _octave_interface->engine_pause();
    */

}

//------------------------------------
void octaveScript::do_request_run()
{
    octave::interpreter* engine = _octave_interface==nullptr ? nullptr : _octave_interface->engine_get_octave_engine();
    if (engine==nullptr) return;

    _octave_interface->execute_run(this);
}

//------------------------------------
void octaveScript::do_request_step_in()
{
    if (_octave_interface==nullptr)
        return;

    if (_octave_interface->engine_get_octave_engine()==nullptr)
        return;

    _octave_interface->execute_step_in(this);

}


void octaveScript::do_request_step_out()
{

}

void octaveScript::do_request_continue()
{
    //if (_debug_line<0) return;
    if (_octave_interface==nullptr) return;
    if (_octave_interface->engine_get_octave_engine()==nullptr) return;
    _octave_interface->execute_continue(this);

}

void octaveScript::do_request_stop()
{
    if (_octave_interface==nullptr)
        return;

    if (_octave_interface->engine_get_octave_engine()==nullptr)
        return;

    _octave_interface->execute_stop(this);

}




