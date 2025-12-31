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



class octaveScript : public QObject, public projectItem
{
    Q_OBJECT
private:
    QStringList      _lines;
    QFile            _script_file;
    octavews*        _octavews;
    QVector<int>    _begin_line, _end_line;

    void            interpret_lines();

    QString         remove_comment_section(int line);
    bool            is_pure_comment(int line);
    bool            has_continuing_line(int line);
    int             end_line(int line);
    int             begin_line(int line);

    octaveInterface*
                    _octave_interface;

    bool           _file_existing;  // if the file is missing this is just a placeholder.

    void                setValid(bool bValid = true);

    QMap<int,int>      _breakpoint_lines;   // This is a map between the line we ask a breakpoint and the effective one

    struct breakpoint_info
    {
        bool remove_next;
        int remove_line;
        int do_not_remove_line;
    };

    bool                b_need_parsing;
    breakpoint_info     m_breakpoint_info;
    enum InterpStatus   _interp_status;
public:
    octaveScript();
    octaveScript(QString filename, projectItem* parent=nullptr);
    octaveScript(const octaveScript& script);
    ~octaveScript();
    void                    set_filename(QString filename, bool save_nload=false);
    QString                 get_fullfilename(){return _script_file.fileName();}
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

    bool                    operator == (octaveScript script);
    bool                    isValid() {return _file_existing;}

    // Breakpoints management
    void                    add_breakpoint (int line, const QString& cond="");
    void                    remove_all_breakpoints();
    void                    remove_breakpoint (int line);
    bool                    has_breakpoint_at_line(int line);
    bool                    has_breakpoints();

    // Parsing status
    bool                    needParsing() {return b_need_parsing;};
    void                    setParsed()   {b_need_parsing = false;}

signals:
    void                    db_stop(octaveScript* script,int line);
    void                    db_run(octaveScript* script);
    void                    db_complete(octaveScript* script);

public slots:
    void                    interpreter_dbstop(const QString& fname, int line);
    void                    interpreter_dbrun(const QString& fname, int line);
    void                    interpreter_dbcomplete(const QString& fname, int line);
    void                    request_pause();
    void                    request_run();
};

typedef octaveScript* octaveScript_ptr;


#endif // OCTAVESCRIPT_H
