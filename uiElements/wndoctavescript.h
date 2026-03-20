/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef WNDOCTAVESCRIPT_H
#define WNDOCTAVESCRIPT_H

#include <QDialog>
#include <QFile>
#include <QFileSystemWatcher>
#include "octavesyntaxhighlighter.h"
#include "octavescript.h"
#include "Qsci/qscilexer.h"
#include "Qsci/qsciscintilla.h"
#include "Qsci/qsciapis.h"
#include <marker.h>
#include <QKeyEvent>

typedef std::function<void ()> fcn_callback;
typedef std::function<void (octave::interpreter&)> meth_callback;

namespace Ui {
class wndOctaveScript;
}

#undef USE_NATIVE_LEXER

namespace octave
{
class interpreter;
}

class wndOctaveScript : public QDialog
{
    Q_OBJECT

public:
    explicit wndOctaveScript(radarProject* proj, QString filename="", class octaveInterface* dataEngine=nullptr,QWidget *parent = nullptr, QString basedir="");
    explicit wndOctaveScript(octaveScript* script, class octaveInterface* dataEngine=nullptr,QWidget *parent = nullptr,QString basedir="");
    ~wndOctaveScript();
private:
    static int              nNoname;
    radarProject*           _project;
    octaveInterface*        _data_interface;
    octaveScript*           _script;
    QFileSystemWatcher      watcher;
    bool                    _bInternal;
    bool                    _b_need_save_as;
    bool					_b_modified;
    bool					_b_closed;
    QIntList                _bp_lines;
    QStringList             _bp_conditions;
    int                     _n_rows;

#ifdef USE_NATIVE_LEXER
    octaveSyntaxHighlighter *hl;

#else
    QsciLexer				*_lexer;
    QsciAPIs				*_api;
#endif
    Ui::wndOctaveScript *ui;

    void setDefaultColor(QsciLexer* lexer);
    void createShortcutActions();
    void setModified(bool bmodified);
    void updateTitle();
    void setupScintilla();
    void add_breakpoint_event (int line, const QString& cond);

    void setInterpreterConnections();

    void create_new_script();
    void link_script_signals(octaveScript* script);
    void breakpoint_update_all();
    void update_path();


signals:
    void update_tree(projectItem* item);
    void interpreter_event (const fcn_callback& fcn);
    void interpreter_event (const meth_callback& meth);
    void maybe_remove_next (int);
    void remove_breakpoint_via_debugger_linenr (int debugger_linenr);
    void request_remove_breakpoint_via_editor_linenr (int editor_linenr);
    void remove_all_breakpoints_signal ();
    void find_translated_line_number (int original_linenr,
                                     int& translated_linenr, marker *&);
    void find_linenr_just_before (int linenr, int& original_linenr,
                                 int& editor_linenr);
    void report_marker_linenr (QIntList& lines, QStringList& conditions);
    void update_breakpoints_signal (const octave_value_list& args);



public:

    void loadFile(QString fname);
    void closeFile();
    void fileChanged();
    octaveScript* get_script() {return _script;}

    void showEvent(QShowEvent *event);

    bool isClosed() {return _b_closed;}
    void updateProject(radarProject* proj);
    void updateFont();
    void update_breakpoints ();

public slots:
    void fileChangedOnDisk(QString str);
    void run_file();
    bool save_file();
    bool save_file_as();
    void update_tips();
    void open_file();
    void new_file();
    void modified();
    void closeEvent( QCloseEvent* event );
    void marginClicked(int margin, int line, Qt::KeyboardModifiers state);
    void toggle_breakpoint (int line);
    void next_breakpoint (const QWidget *ID);
    void remove_all_breakpoints (const QWidget *ID);
    void previous_breakpoint (const QWidget *ID);
    void file_has_changed (const QString& path, bool do_close = false);
    void            scriptIdle(const QString& filename);
    void            scriptRunning(const QString& filename );
    void            scriptDone(const QString& filename );
    void            scriptError(const QString& filename,const QString& strError, int line);
    void            scriptPaused(const QString& filename, int line);

    void            request_stop_engine();
    void            request_step();
    void            request_toggle_breakpoint();
    void keyPressEvent(QKeyEvent *e) {
        if(e->key() != Qt::Key_Escape)
            QDialog::keyPressEvent(e);
        else {/* minimize */}
    }

};

#endif // WNDOCTAVESCRIPT_H