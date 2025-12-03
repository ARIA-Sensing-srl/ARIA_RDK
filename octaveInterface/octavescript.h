/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef OCTAVESCRIPT_H
#define OCTAVESCRIPT_H

#include <QString>
#include <QFile>
#include <QStringList>
#include "radarproject.h"
#include <QtXml>
#include <octavews.h>
#include <octaveinterface.h>

class octaveScript : public projectItem
{
    QStringList      _lines;
    QFile            _script_file;
    QVector<bool>    _breakpoints;
    QVector<bool>    _comment;
    octavews*        _octavews;
    QVector<int>    _begin_line, _end_line;

    void            interpret_lines();

    QString         remove_comment_section(int line);
    bool            is_pure_comment(int line);
    bool            has_continuing_line(int line);
    int             end_line(int line);
    int             begin_line(int line);

    octaveInterface*        _octave_interface;

    bool           _file_existing;  // if the file is missing this is just a placeholder.

    void            setValid(bool bValid = true);

public:
    octaveScript();
    octaveScript(QString filename, projectItem* parent=nullptr);
    octaveScript(const octaveScript& script);
    ~octaveScript();
    void                    set_filename(QString filename);

    void                    set_scriptfile(QString filename);
    bool                    has_breakpoints();
    bool                    is_breakpoint(int lineno);
    void                    set_breakpoint(int lineno);
    void                    clear_breakpoint(int lineno);

    int                     get_lines_count();

    const octaveScript& operator = (const octaveScript& script);

    static  octaveScript*   load_xml(QDomDocument& owner, QDomElement& script_elem);
    bool                    save_xml(QDomDocument& owner, QDomElement& script_elem);

    QString                 get_text();
    void                    set_text(const QString& input_text);
    bool                    run(const oct_dataset& in, oct_dataset& out);

    int                     get_line_count() {return _lines.count();}
    std::string             get_std_line(int n);
    QString                 get_line(int n);
    void                    load();
    void                    save();

    void                    attach_to_dataengine(octaveInterface* octint);
    octaveInterface*        get_octave_interface();

    bool    operator == (octaveScript script);

    bool                    isValid() {return _file_existing;}

};

typedef octaveScript* octaveScript_ptr;


#endif // OCTAVESCRIPT_H
