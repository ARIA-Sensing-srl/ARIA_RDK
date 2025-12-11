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

signals:
    void update_tree(projectItem* item);
public:

    void loadFile(QString fname);
    void closeFile();
    void fileChanged();
    octaveScript* get_script() {return _script;}

	void showEvent(QShowEvent *event);

	bool isClosed() {return _b_closed;}
    void updateProject(radarProject* proj);
    void updateFont();
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

};

#endif // WNDOCTAVESCRIPT_H
