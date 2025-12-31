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

#ifdef USE_NATIVE_LEXER
	octaveSyntaxHighlighter *hl;

#else
	QsciLexer				*_lexer;
	QsciAPIs				*_api;
#endif
    Ui::wndOctaveScript *ui;

    void update_wnd_title();

    void setDefaultColor(QsciLexer* lexer);
    void createShortcutActions();
    void setModified(bool bmodified);
    void updateTitle();
    void setupScintilla();
    void add_breakpoint_event (int line, const QString& cond);

    void setInterpreterConnections();
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
    void request_add_breakpoint (int line, const QString& cond);
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
    void update_breakpoints_handler (const octave_value_list& argout);
    void handle_request_remove_breakpoint(int line);
    void handle_request_add_breakpoint(int line, const QString& condition);
    void toggle_breakpoint (const QWidget *ID);
    void next_breakpoint (const QWidget *ID);
    void remove_all_breakpoints (const QWidget *ID);
    void previous_breakpoint (const QWidget *ID);
    void do_breakpoint_marker (bool insert, const QWidget *ID, int line = -1,
                              const QString& cond = "");
    void handle_remove_next (int remove_line);
    void file_has_changed (const QString& path, bool do_close = false);

    void            engineRunning();
    void            engineDone(const QString& command, int errorcode);
    void            engineBusy();
    void            enginePaused();

    void            request_pause_engine();
    void            request_stop_engine();
    void            request_step_run();



};

#endif // WNDOCTAVESCRIPT_H
