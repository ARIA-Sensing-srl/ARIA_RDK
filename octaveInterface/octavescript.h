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
    QString             _text;
    QFile               _script_file;
    octavews*           _octavews;
    octaveInterface*    _octave_interface;
    bool                _file_existing;  // if the file is missing this is just a placeholder.
    void                setValid(bool bValid = true);
    int                 _debug_line;

    struct breakpoint_info
    {
        bool remove_next;
        int remove_line;
        int do_not_remove_line;
    };

    bool                b_need_parsing;
    breakpoint_info     m_breakpoint_info;
public:
    octaveScript();
    octaveScript(QString filename, projectItem* parent=nullptr);
    octaveScript(const octaveScript& script);
    ~octaveScript();
    void                    set_filename(QString filename, bool save_nload=false);
    QString                 get_fullfilename(){return _script_file.fileName();}

    const octaveScript& operator = (const octaveScript& script);

    static  octaveScript*   load_xml(QDomDocument& owner, QDomElement& script_elem);
    bool                    save_xml(QDomDocument& owner, QDomElement& script_elem);

    QString                 get_text();
    void                    set_text(const QString& input_text);
    bool                    run(const oct_dataset& in, oct_dataset& out);

    void                    load();
    void                    save();

    void                    attach_to_dataengine(octaveInterface* octint);
    octaveInterface*        get_octave_interface();

    bool                    operator == (octaveScript script);
    bool                    isValid() {return _file_existing;}

    //----------------------------------------------------
    // Breakpoints management
    int                     breakpoint_toggle(int line, const QString& cond="");
    int                     breakpoint_add (int line, const QString& cond="");
    void                    breakpoint_remove_all();
    int                     breakpoint_remove (int line);
    bool                    breakpoint_at_line(int line, int& lineeq);
    bool                    breakpoints_has_any();
    int                     breakpoint_get_line(int line);

    //------------------------------------------------------
    // Parsing status
    bool                    needParsing() {return b_need_parsing;};
    void                    setParsed()   {b_need_parsing = false;}

    //

    int   getCurrentDebugPosition() {return _debug_line; }
signals:
    // Signals for script manager (e.g. editor)
    void                    script_stop(const QString& filename, int line);
    void                    script_run(const QString& filename );
    void                    script_run_complete(const QString& filename);
    void                    script_error(const QString& filename, const QString& strError, int line);

public slots:
    // Slot to manage signals coming from the octaveInterface wrapper
    void                    do_interpreter_dbstop(const QString& fname, int line);
    void                    do_interpreter_dbrun(const QString& fname);
    void                    do_interpreter_dbcomplete(const QString& fname);
    void                    do_interpreter_error(const QString& fname, const QString& error, int line);
    // Slot to manage requests from the GUI
    void                    do_request_pause();
    void                    do_request_run();
    void                    do_request_step_in();
    void                    do_request_step_out();
    void                    do_request_continue();
    void                    do_request_stop();
};

typedef octaveScript* octaveScript_ptr;


#endif // OCTAVESCRIPT_H
