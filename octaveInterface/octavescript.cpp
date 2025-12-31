/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "octavescript.h"
#include <QFileInfo>
#include <QDir>
#include "interpreter.h"

octaveScript::octaveScript(QString filename, projectItem* parent) : projectItem("newScript",DT_SCRIPT, parent),
    _lines(),
    _script_file(),
    _octave_interface(nullptr),
    _breakpoint_lines(),
    b_need_parsing(true)
{

    set_filename(filename,false);
}
//------------------------------------
octaveScript::octaveScript() : projectItem("newScript",DT_SCRIPT),
    _lines(),
    _script_file(),
    _octave_interface(nullptr),
    _file_existing(false),
    _breakpoint_lines(),
    b_need_parsing(true)
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
}
//------------------------------------
const octaveScript& octaveScript::operator = (const octaveScript& script)
{
    projectItem::operator=(script);
    _lines = script._lines;
    _script_file.setFileName(script._script_file.fileName());

    _octave_interface = script._octave_interface;
    _breakpoint_lines = script._breakpoint_lines;
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
void octaveScript::interpret_lines()
{
    _begin_line.resize(_lines.count());
    _end_line.resize(_lines.count());

    int n=0;
    while (n < _lines.count())
    {
        int ne = end_line(n);
        for (int k=n ; k < ne;k++)
        {_begin_line[k] = n; _end_line[k]=ne-1;}
        n=ne+1;
    }
}
//------------------------------------
void  octaveScript::load()
{
    _lines.clear();

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
QString         octaveScript::remove_comment_section(int line)
{
    QString out;
    if ((line<0)||(line>=_lines.count())) return out;

    out = _lines[line].section('%',0,0);

    return out.simplified();
}
//------------------------------------
bool            octaveScript::is_pure_comment(int line)
{
    if ((line<0)||(line>=_lines.count())) return true;
    if (_lines[line].simplified().isEmpty()) return true;
    return _lines[line].simplified().startsWith('%');
}
//------------------------------------
bool            octaveScript::has_continuing_line(int line)
{
    if ((line<0)||(line>=_lines.count())) return false;
    if (is_pure_comment(line)) return false;
    return remove_comment_section(line).endsWith("...");
}
//------------------------------------
int     octaveScript::end_line(int line)
{
    if ((line<0)||(line>=_lines.count())) return line;
    while (line <= _lines.count())
    {
        if (!has_continuing_line(line))
            break;
        line++;
    }
    return line;
}
//------------------------------------
int octaveScript::begin_line(int line)
{
    if ((line<0)||(line>=_lines.count())) return line;
    line--;
    while (line >=0 )
    {
        if (!has_continuing_line(line))
            break;
        line--;
    }
    return line+1;
}
//------------------------------------
QString octaveScript::get_text()
{
    return _lines.join('\n');
}
//------------------------------------
void octaveScript::set_text(const QString& input_text)
{

    _lines = input_text.split('\n');
    //save();
}
//------------------------------------
std::string     octaveScript::get_std_line(int n)
{
    if ((n<0)||(n>=_lines.count())) return std::string("");
    return _lines[n].toStdString();
}
//------------------------------------
QString         octaveScript::get_line(int n)
{
    if ((n<0)||(n>=_lines.count())) return QString("");
    return _lines[n];
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
    b_need_parsing = true;

    if (!_script_file.fileName().isEmpty())
    {
        if (!_script_file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream out(&_script_file);
        out << _lines.join("\n");
        _script_file.close();

        setValid();
    }
}
//------------------------------------
void    octaveScript::attach_to_dataengine(octaveInterface* octint)
{
    _octave_interface= octint;
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

void octaveScript::add_breakpoint (int line, const QString& cond)
{
    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->get_octave_engine();
    if (interp==nullptr) return;

    octave::tree_evaluator& tw = interp->get_evaluator ();
    octave::bp_table& bptab = tw.get_bp_table ();

    int lineeq = bptab.add_breakpoint_in_file(get_fullfilename().toStdString (), line, cond.toStdString());
    _breakpoint_lines[line] = lineeq;


}


void octaveScript::remove_breakpoint (int line)
{
    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->get_octave_engine();
    if (interp==nullptr) return;
    if (_breakpoint_lines.find(line)==_breakpoint_lines.end()) return;

    octave::tree_evaluator& tw = interp->get_evaluator ();
    octave::bp_table& bptab = tw.get_bp_table ();

    bptab.remove_breakpoint_from_file (get_fullfilename().toStdString (), line);
    _breakpoint_lines.remove(line);
}

void octaveScript::remove_all_breakpoints()
{
    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->get_octave_engine();
    if (interp==nullptr) return;
    octave::tree_evaluator& tw = interp->get_evaluator ();
    octave::bp_table& bptab = tw.get_bp_table ();

    bptab.remove_all_breakpoints_from_file(get_fullfilename().toStdString (),
                                           true);

    _breakpoint_lines.clear();
}


bool octaveScript::has_breakpoint_at_line(int line)
{
    octave::interpreter *interp = _octave_interface == nullptr ? nullptr : _octave_interface->get_octave_engine();
    if (interp==nullptr) return false;

    return (_breakpoint_lines.find(line)!=_breakpoint_lines.end());
}


//------------------------------------
bool octaveScript::has_breakpoints()
{
    return !(_breakpoint_lines.empty());
}

// These are the slot for the signals coming from the octaveInterface

void octaveScript::interpreter_dbstop(const QString& fname, int line)
{
    if (fname != _script_file.fileName()) return;
    emit db_stop(this,line);

}
void octaveScript::interpreter_dbrun(const QString& fname, int line)
{
    if (fname != _script_file.fileName()) return;
    emit db_run(this);
}

void octaveScript::interpreter_dbcomplete(const QString& fname, int line)
{
    if (fname != _script_file.fileName()) return;
    emit db_complete(this);
}

//------------------------------------
void octaveScript::request_pause()
{
    // We cannot pause the engine if it is not running THIS script
}

//------------------------------------
void octaveScript::request_run()
{
    octave::interpreter* engine = _octave_interface==nullptr ? nullptr : _octave_interface->get_octave_engine();
    if (engine==nullptr) return;

    _octave_interface->run(this);


}



