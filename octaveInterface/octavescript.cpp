/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "octavescript.h"
#include <QFileInfo>
#include <QDir>

octaveScript::octaveScript(QString filename, projectItem* parent) : projectItem("newScript",DT_SCRIPT, parent),
    _lines(),
    _script_file(),
    _breakpoints(),
    _octave_interface(nullptr)
{

    set_scriptfile(filename);
}
//------------------------------------
octaveScript::octaveScript() : projectItem("newScript",DT_SCRIPT),
    _lines(),
    _script_file(),
    _breakpoints(),
    _octave_interface(nullptr)
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
    _breakpoints = script._breakpoints;
    _octave_interface = script._octave_interface;
    return *this;
}


void octaveScript::set_filename(QString filename)
{
    QFileInfo fi(filename);
    _filename = fi.fileName();
    _script_file.setFileName(filename);
    if (filename.isEmpty())
        set_name("[noname]");
    else
        set_name(_filename);
}
//------------------------------------
bool octaveScript::has_breakpoints()
{
    for (const auto& bp : _breakpoints)
        if (bp) return true;
    return false;
}
//------------------------------------
bool octaveScript::is_breakpoint(int lineno)
{
    return ((lineno >= 0) && (lineno < _breakpoints.count())) ? _breakpoints[lineno] : false;
}
//------------------------------------
void octaveScript::set_breakpoint(int lineno)
{
    if ((lineno >= 0)&&(lineno < _breakpoints.count())) _breakpoints[lineno] = true;
}
//------------------------------------
void octaveScript::clear_breakpoint(int lineno)
{
    if ((lineno >= 0)&&(lineno < _breakpoints.count())) _breakpoints[lineno] = false;
}
//------------------------------------

/*
void octaveScript::set_scriptfile_name(QString filename)
{
    if (_script_file.isOpen())
        _script_file.close();

    QString old_file_name = get_full_filepath();

    _filename = filename;

    _script_file.setFileName(filename);

    if (_filename!=old_file_name)
    {
        QFile fOrigin(old_file_name);
        if (fOrigin.exists())
        {
            QFile fDest(_filename);
            if (!fDest.exists())
            {
                fOrigin.copy(_filename);
            }
        }
    }

    if (filename.isEmpty())
        set_name("noname");
    else
        set_name(QFileInfo(filename).fileName());
}*/
//------------------------------------
void octaveScript::set_scriptfile(QString filename)
{
    set_filename(filename);
    load();
}

/*
void octaveScript::set_base_dir(QString new_base_dir, QString relative_dir, bool copy)
{
    qDebug() << new_base_dir;
    qDebug() << relative_dir;

    QString old_file(_filename);
    projectItem::set_base_dir(new_base_dir,relative_dir);

    if (copy)
    {
        QString new_file(_filename);
        if (new_file!=old_file)
        {
            QFile fOrigin(old_file);
            if (fOrigin.exists())
            {
                QFile fDest(new_file);
                if (!fDest.exists())
                    fOrigin.copy(new_file);
            }
        }
    }
     set_scriptfile(get_full_filepath());
}
*/
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
    out->set_name(root.attribute("script_name","undefined"));
    out->_filename = (root.attribute("filename",""));
    out->_script_file.setFileName(out->get_full_filepath());
    root = root.nextSiblingElement();
    return out;
}
//------------------------------------
void octaveScript::interpret_lines()
{
    _breakpoints.fill(false, _lines.count());
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
    }
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
void    octaveScript::save()
{
    if (!_script_file.fileName().isEmpty())
    {
        if (!_script_file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream out(&_script_file);
        out << _lines.join("\n");
        _script_file.close();
    }
}
//------------------------------------
void                octaveScript::attach_to_dataengine(octaveInterface* octint)
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



