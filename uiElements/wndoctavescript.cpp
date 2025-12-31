/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "wndoctavescript.h"
#include "ui_wndoctavescript.h"
#include "marker.h"
#include "interpreter.h"
#include "octaveinterface.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "Qsci/qscilexeroctave.h"
#include "Qsci/qscilexercpp.h"
#include "Qsci/qsciscintilla.h"
#include <ovl.h>
int wndOctaveScript::nNoname = 0;

extern QDir             ariasdk_modules_path;
extern QDir             ariasdk_projects_path;
extern QDir             ariasdk_scripts_path;
extern QDir             ariasdk_antennas_path;
extern QDir             ariasdk_antennaff_path;
extern QFont            ariasdk_script_font;

wndOctaveScript::wndOctaveScript(radarProject* proj, QString filename, octaveInterface* dataEngine,QWidget *parent, QString basedir) :
    QDialog(parent),
    _data_interface(dataEngine),
    _bInternal(true),
    _b_need_save_as(false),
	_b_modified(false),
	_b_closed(false),
    _project(proj),
    _script(nullptr),

#ifdef USE_NATIVE_LEXER
    hl(nullptr),
#else

	_lexer(nullptr),
	_api(nullptr),
#endif
    ui(new Ui::wndOctaveScript)
{
    ui->setupUi(this);


#ifdef USE_NATIVE_LEXER
	emit ui->textScript->undoAvailable(true);
    hl = new octaveSyntaxHighlighter(_data_interface,ui->textScript->document());
#else
	if ((filename.endsWith(".cpp"))||(filename.endsWith(".cc"))||(filename.endsWith(".c")))
		_lexer = new QsciLexerCPP(ui->textScript);
	else
		_lexer = new QsciLexerOctave(ui->textScript);

    setDefaultColor(_lexer);

    ui->textScript->setLexer(_lexer);
	_api = new QsciAPIs(_lexer);

#endif

    // Create a new script object in case of a new file
    if (filename.isEmpty())
    {
        _script = new octaveScript(filename);
        _bInternal = true;
        _script->attach_to_dataengine(dataEngine);

        filename = "newscript";
        QFile fi(filename+QString(".m"));
        QString newFilename;

        int n=0;
        while (fi.exists())
        {
            newFilename = filename + QString::number(n) + QString(".m");
            fi.setFileName(newFilename);
        }

        filename = newFilename;
        _script->set_filename(filename,false);
        _script->save();
        _b_need_save_as = true;
    }
    else
    {
        // If we have a valid filename AND a project, let's check if the script
        // that we are opening does not already belong to the project
        if (_project!=nullptr)
        {
            octaveScript* script = nullptr;
            QVector<octaveScript*> scripts = _project->get_available_scripts();
            for (auto s : scripts)
                if (s!=nullptr)
                    if (s->get_fullfilename() == filename)
                    {
                        _script = s;
                        _bInternal = false;
                        break;
                    }
        }
        // if the file does not belong to the script, this is a file not linked
        // to the project
        if (_script==nullptr)
        {
            _script = new octaveScript();
            _script->attach_to_dataengine(_data_interface);
            _script->set_filename(filename,false);
            _bInternal = true;
        }
    }
    ui->textScript->setText(_script->get_text());


#ifdef USE_NATIVE_LEXER
    int fontWidth = QFontMetrics(ui->textScript->currentCharFormat().font()).averageCharWidth();
    ui->textScript->setTabStopDistance( 4 * fontWidth );
#endif
	ui->textScript->update();

#ifndef USE_NATIVE_LEXER
    setupScintilla();
#endif

    createShortcutActions();
    _lexer->setFont(ariasdk_script_font);

    setInterpreterConnections();
}

wndOctaveScript::wndOctaveScript(octaveScript* script, class octaveInterface* dataEngine,QWidget *parent, QString basedir) : QDialog(parent),
    _data_interface(dataEngine),
    _script(script),
    _bInternal(true),
    _b_need_save_as(false),
	_b_modified(false),
	_b_closed(false),

#ifdef USE_NATIVE_LEXER
	hl(nullptr),
#else
	_lexer(nullptr),
	_api(nullptr),
#endif
    ui(new Ui::wndOctaveScript)
{
    ui->setupUi(this);
    _lexer = new QsciLexerOctave(ui->textScript);
#ifdef USE_NATIVE_LEXER
	hl = new octaveSyntaxHighlighter(_data_interface,ui->textScript->document());
#else
    setDefaultColor(_lexer);
    ui->textScript->setLexer(_lexer);
	_api = new QsciAPIs(_lexer);
	_lexer->setAPIs(_api);
#endif

    if (script == nullptr)
    {
        QString filename = "newscript";
        _script = new octaveScript();
        _bInternal = true;
        _script->attach_to_dataengine(dataEngine);

        QFile fi(filename+QString(".m"));
        QString newFilename;

        int n=0;
        while (fi.exists())
        {
            newFilename = filename + QString::number(n) + QString(".m");
            fi.setFileName(newFilename);
        }

        filename = newFilename;
        _script->set_filename(filename,true);
        _b_need_save_as = true;
        _bInternal = true;
    }
    else
    {
        _project = _script->get_root();
        _b_need_save_as = false;
        _bInternal = false;
        _script->load();
        ui->textScript->setText(_script->get_text());
    }

	ui->textScript->update();
#ifndef USE_NATIVE_LEXER
    setupScintilla();
#endif
    this->setWindowTitle(QString("Octave Script:")+QFileInfo(_script->get_full_filepath()).fileName());

    createShortcutActions();

    _lexer->setFont(ariasdk_script_font);

    setInterpreterConnections();
}


void wndOctaveScript::setInterpreterConnections()
{
    if (_data_interface==nullptr) return;
    if (_data_interface->get_octave_engine()==nullptr) return;
    // This connections are used to enable / disable actions
    connect(_data_interface, &octaveInterface::engineRunning, this, &wndOctaveScript::engineRunning);
    connect(_data_interface, &octaveInterface::engineDone, this, &wndOctaveScript::engineDone);

}

void wndOctaveScript::engineRunning()
{
    ui->tbRun->setEnabled(false);
    ui->tbStep->setEnabled(false);
    ui->tbPause->setEnabled(true);
    ui->tbStop->setEnabled(true);


}

void    wndOctaveScript::engineDone(const QString& command, int errorcode)
{
    enginePaused();
}


void    wndOctaveScript::engineBusy()
{
    engineRunning();
}

void    wndOctaveScript::enginePaused()
{
    ui->tbRun->setEnabled(true);
    ui->tbStep->setEnabled(true);
    ui->tbPause->setEnabled(false);
    ui->tbStop->setEnabled(false);
}


void wndOctaveScript::setupScintilla()
{
    ui->textScript->setMargins(2); // 0 is for breakpoints position, 1 is for execution position
    ui->textScript->setMarginType (1, QsciScintilla::SymbolMargin);
    ui->textScript->setMarginSensitivity (1, true);
    ui->textScript->markerDefine (QsciScintilla::RightTriangle, marker::bookmark);
    ui->textScript->setMarkerBackgroundColor (QColor (0, 0, 232), marker::bookmark);
    ui->textScript->markerDefine (QsciScintilla::Circle, marker::breakpoint);
    ui->textScript->setMarkerBackgroundColor (QColor (192, 0, 0), marker::breakpoint);
    ui->textScript->markerDefine (QsciScintilla::Circle, marker::cond_break);
    ui->textScript->setMarkerBackgroundColor (QColor (255, 127, 0), marker::cond_break);
    ui->textScript->markerDefine (QsciScintilla::RightArrow,
                              marker::debugger_position);
    ui->textScript->setMarkerBackgroundColor (QColor (255, 255, 0),
                                          marker::debugger_position);
    ui->textScript->markerDefine (QsciScintilla::RightArrow,
                              marker::unsure_debugger_position);
    ui->textScript->setMarkerBackgroundColor (QColor (192, 192, 192),
                                          marker::unsure_debugger_position);



    connect(ui->textScript, &QsciScintilla::textChanged, this, &wndOctaveScript::modified);
    connect(ui->textScript, &QsciScintilla::marginClicked, this, &wndOctaveScript::marginClicked);

    // line numbers
    ui->textScript->setMarginsForegroundColor (QColor (96, 96, 96));
    ui->textScript->setMarginsBackgroundColor (QColor (232, 232, 220));
    ui->textScript->setMarginType (2, QsciScintilla::TextMargin);

    // other features
    ui->textScript->setBraceMatching (QsciScintilla::StrictBraceMatch);
    ui->textScript->setAutoIndent (true);
    ui->textScript->setIndentationWidth (2);
    ui->textScript->setIndentationsUseTabs (false);

    ui->textScript->setUtf8 (true);
    // auto completion
    ui->textScript->SendScintilla (QsciScintillaBase::SCI_AUTOCSETCANCELATSTART, false);

    connect (this, &wndOctaveScript::maybe_remove_next,
            this, &wndOctaveScript::handle_remove_next);

    connect (this, &wndOctaveScript::request_add_breakpoint,
            this, &wndOctaveScript::handle_request_add_breakpoint);
}


void wndOctaveScript::setDefaultColor(QsciLexer* lexer)
{
    lexer->setColor(Qt::white, QsciLexerOctave::Operator);
    lexer->setColor(Qt::darkYellow, QsciLexerOctave::Keyword);
    lexer->setColor(Qt::cyan, QsciLexerOctave::Identifier);
    lexer->setColor(Qt::darkGreen, QsciLexerOctave::Command);
    lexer->setColor(Qt::darkCyan, QsciLexerOctave::Number);
    lexer->setColor(Qt::yellow, QsciLexerOctave::DoubleQuotedString);
    lexer->setColor(Qt::darkYellow, QsciLexerOctave::SingleQuotedString);
}


void wndOctaveScript::createShortcutActions()
{
    connect(ui->tbRun,      &QToolButton::clicked, this, &wndOctaveScript::run_file);
    connect(ui->tbSave,     &QToolButton::clicked, this, &wndOctaveScript::save_file);
    connect(ui->tbSaveAs,   &QToolButton::clicked, this, &wndOctaveScript::save_file_as);
    connect(ui->tbOpen,     &QToolButton::clicked, this, &wndOctaveScript::open_file);
    connect(ui->tbNew,      &QToolButton::clicked, this, &wndOctaveScript::new_file);
    connect(ui->tbPause,   &QToolButton::clicked, this, &wndOctaveScript::request_pause_engine);
    connect(ui->tbStop,   &QToolButton::clicked, this, &wndOctaveScript::request_stop_engine);
    connect(ui->tbStep,   &QToolButton::clicked, this, &wndOctaveScript::request_step_run);
}


void  wndOctaveScript::request_pause_engine()
{

}

void wndOctaveScript::request_stop_engine()
{

}

void wndOctaveScript::request_step_run()
{

}


wndOctaveScript::~wndOctaveScript()
{

#ifdef USE_NATIVE_LEXER
    if (hl!=nullptr) delete hl;
#else
	if (_lexer!=nullptr) delete _lexer;

#endif
    if (_bInternal)
        delete(_script);
    _script = nullptr;
    delete ui;
}



void wndOctaveScript::update_wnd_title()
{
    this->setWindowTitle(_script->get_full_filepath());
}


void wndOctaveScript::loadFile(QString fname)
{
    update_wnd_title();
    connect(&watcher,SIGNAL(fileChanged(QString)), this, SLOT(fileChangedOnDisk(QString)));

    _script->set_filename(fname,false);
    ui->textScript->setText(_script->get_text());
}


void wndOctaveScript::closeFile()
{
    if (_script!=nullptr)
        _script->remove_all_breakpoints();

}

void wndOctaveScript::run_file()
{
    if (_b_modified)
        if (!save_file()) return;

    if (_data_interface==nullptr) return;
    _script->request_run();
}


void wndOctaveScript::update_tips()
{

#ifndef USE_NATIVE_LEXER
	_api->clear();
	if ((_data_interface!=nullptr)&&(_data_interface->get_octave_engine()!=nullptr))
	{
		// Add variables
		std::list<std::string> varnames = _data_interface->get_octave_engine()->variable_names();

		for (std::list<std::string>::iterator iter = varnames.begin(); iter!=varnames.end(); iter++)
			_api->add(QString::fromStdString(*iter));

		varnames = _data_interface->get_octave_engine()->global_variable_names();

		for (std::list<std::string>::iterator iter = varnames.begin(); iter!=varnames.end(); iter++)
			_api->add(QString::fromStdString(*iter));

		varnames = _data_interface->get_octave_engine()->top_level_variable_names();

		for (std::list<std::string>::iterator iter = varnames.begin(); iter!=varnames.end(); iter++)
			_api->add(QString::fromStdString(*iter));
		// Add functions


		std::list<std::string> funcnames= _data_interface->get_octave_engine()->user_function_names();

		for (std::list<std::string>::iterator iter = funcnames.begin(); iter!=funcnames.end(); iter++)
		{
			_api->add(QString::fromStdString(*iter)+"( args )");

		}
	}
	_api->prepare();
	_lexer->setAPIs(_api);
#endif
}

void wndOctaveScript::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
#ifndef  USE_NATIVE_LEXER
	ui->textScript->setCaretForegroundColor(Qt::lightGray);
	ui->textScript->setCaretWidth(5);
//# Set the text wrapping mode to word wrap
	ui->textScript->setWrapMode(QsciScintilla::WrapWord);
//# Set the text wrapping mode visual indication
	ui->textScript->setWrapVisualFlags(QsciScintilla::WrapFlagByText);
//# Set the text wrapping to indent the wrapped lines
	ui->textScript->setWrapIndentMode(QsciScintilla::WrapIndentSame);
	ui->textScript->setAutoIndent(true);
	ui->textScript->setAutoCompletionFillupsEnabled(true);
	ui->textScript->setAutoCompletionCaseSensitivity(true);
	ui->textScript->setAutoCompletionSource(QsciScintilla::AcsAll);
	ui->textScript->setAutoCompletionThreshold(2);
	ui->textScript->setAutoCompletionReplaceWord(true);
	ui->textScript->setCallTipsVisible(0);
	ui->textScript->setCallTipsPosition(QsciScintilla::CallTipsBelowText);

	update_tips();

#endif

}

void wndOctaveScript::fileChangedOnDisk(QString str)
{
//    if (QMessageBox()
}


bool wndOctaveScript::save_file()
{
    if (_b_need_save_as) { return save_file_as(); }

#ifdef USE_NATIVE_LEXER
	_script->set_text(ui->textScript->toPlainText());
#else
    _script->set_text(ui->textScript->text());
#endif

    _script->save();

	_b_modified = false;
	if (windowTitle().endsWith("*"))
		setWindowTitle(windowTitle().removeLast());

    return true;
}

void wndOctaveScript::new_file()
{
    if (_b_modified)
    {
        auto res = QMessageBox::question(this,"Confirm","The current file has been modified. Do you want to save it?",
                                         QMessageBox::Yes | QMessageBox::Cancel | QMessageBox::No, QMessageBox::Cancel);

        if (res ==  QMessageBox::Yes)
            if (!save_file()) return;

        if (res == QMessageBox::Cancel)
            return;
    }

    if (!_bInternal)
    {
        // if if was a project script, we need to create a new one detached from the project
        _script = new octaveScript();
        _script->attach_to_dataengine(_data_interface);
        _bInternal = true;
    }

    _script->set_text("");
    QString filename = "newscript";
    QFile fi(filename+QString(".m"));
    QString newFilename;

    int n=0;
    while (fi.exists())
    {
        newFilename = filename + QString::number(n) + QString(".m");
        fi.setFileName(newFilename);
    }

    filename = newFilename;
    _script->set_filename(filename,true);
    _b_need_save_as = true;
    ui->textScript->setText("");
    setModified(false);
}

void wndOctaveScript::open_file()
{
    if (_b_modified)
    {
        auto res = QMessageBox::question(this,"Confirm","The current file has been modified. Do you want to save it?",
                                         QMessageBox::Yes | QMessageBox::Cancel | QMessageBox::No, QMessageBox::Cancel);

        if (res ==  QMessageBox::Yes)
            if (!save_file()) return;

        if (res == QMessageBox::Cancel)
            return;

   }

    QString scriptFile = QFileDialog::getOpenFileName(this,"Import script file",
                                                      ariasdk_scripts_path.absolutePath()+QDir::separator(),
                                                      tr("Script files(*.m);;All files (*.*)"),
                                                      nullptr,
                                                      QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));

   if (scriptFile.isEmpty())
        return;

   // Check if we are not opening a project script
   bool bNewInternal = true;

   octaveScript* possibleOldScript =  nullptr;
   QVector<octaveScript*> scripts;

   if (_project!=nullptr) scripts = _project->get_available_scripts();

   for (auto s : scripts)
       if (s!=nullptr)
       {
           if (s->get_fullfilename() == scriptFile)
           {
               possibleOldScript = s;
               bNewInternal = false;
               break;
           }
       }

    if ((!_bInternal)&&(!bNewInternal))
   {
       // The new one is a project script. Update the script to the project's one
       _script = possibleOldScript;
       _script->set_filename(scriptFile,false);
       _bInternal = false;
   }

   if ((_bInternal)&&(!bNewInternal))
   {
       // The new one is a project script. Update the script pointer to the project's one
       _script = possibleOldScript;
       _script->set_filename(scriptFile,false);
       _bInternal = false;
   }

   if ((!_bInternal)&&(bNewInternal))
   {
       // The old one is a project script and the new one is not. We need to create a temporary octaveScript object
       _script = new octaveScript();
        _script->attach_to_dataengine(_data_interface);
       _script->set_filename(scriptFile,false);
        _bInternal = true;
   }

   if ((_bInternal)&&(bNewInternal))
   {
       // The new one is not a project script, we need to create a temporary octaveScript object iff prev was null
       if (_script==nullptr)
       {
            _script = new octaveScript();
            _script->attach_to_dataengine(_data_interface);
       }

        _bInternal = true;
       _script->set_filename(scriptFile,false);
   }

   ui->textScript->setText(_script->get_text());
   setModified(false);

   _b_need_save_as = false;


}

bool wndOctaveScript::save_file_as()
{
    QString fname = _script->get_filename();
    QFileInfo fi(fname);
    if (fi.suffix()!=".m")
        fname+=".m";

    QString projectFile = QFileDialog::getSaveFileName(this,"Save script as",
                                                      ariasdk_scripts_path.absolutePath()+QDir::separator()+fname,
                                                       tr("Script files(*.m);;All files (*.*)"),
                                                       nullptr,
                                                       QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));

    if (projectFile.isEmpty()) return false;

    fi.setFile(projectFile);
    ariasdk_scripts_path.setPath(fi.absolutePath());

    _bInternal = true;

    radarProject* root = _script->get_root();
    // Create a new script
    if (!_bInternal)
    _script = new octaveScript(*_script);
#ifdef USE_NATIVE_LEXER
    _script->set_text(ui->textScript->toPlainText());
#else
	_script->set_text(ui->textScript->text());
#endif
    _script->set_filename(projectFile);
    _script->save();

    if (!_bInternal)
        if (root != nullptr)
            root->add_script(_script);

    _b_need_save_as = false;
    setModified(false);
    return true;
}

void wndOctaveScript::setModified(bool bmodified)
{
    _b_modified = bmodified;
    updateTitle();
}
void wndOctaveScript::updateTitle()
{
    this->setWindowTitle(QString("Octave Script:")+_script->get_fullfilename()+QString(_b_modified?"*":""));
}


//---------------------------------------------
void wndOctaveScript::modified()
{
    setModified(true);
}


void wndOctaveScript::closeEvent( QCloseEvent* event )
{
	if (_b_modified)
	{
		QMessageBox::StandardButton result = QMessageBox::question(this, "Confirm","The current content has changed. Do you want to save?",QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
		if (result == QMessageBox::No)
		{event->accept(); _b_closed = true; return;}
		if (result == QMessageBox::Cancel)
		{event->ignore(); return;}
		if (result == QMessageBox::Yes)
		{
			if (_b_need_save_as)
				save_file_as();
			else
				save_file();

			if (_b_modified)
			{event->ignore(); return;}

			event->accept(); _b_closed = true; return;
		}

		event->ignore();
		return;
	}
	_b_closed = true;
	event->accept();
	return;
}


void wndOctaveScript::updateFont()
{
    if (_lexer==nullptr)
        ui->textScript->setFont(ariasdk_script_font);
    else
    {
        _lexer->setFont(ariasdk_script_font);
    }
}


void wndOctaveScript::updateProject(radarProject* proj)
{

    if ((_project==nullptr)&&(proj==nullptr))
        return;
    // We are moving onto a new project
    if ((proj!=nullptr)&&(proj!=_project))
    {
        _project = proj;
        // 1. Check if the current script is a script of the new project
        QVector<octaveScript*> scripts = proj->get_available_scripts();

        octaveScript* projScript = nullptr;
        for (auto s : scripts)
            if ((s!=nullptr)&&(s->get_fullfilename()==_script->get_fullfilename()))
            { projScript = s; break;}
        // We have found a script item pointing to same file as current text under editing
        if (projScript!=nullptr)
        {
            if ((_bInternal)&&(_script!=nullptr))
            {
                delete _script;
            }
            projScript->set_text(ui->textScript->text());
            _script = projScript;
            _bInternal = false;
            return;
        }

        // Previous script was linked to the previous project and not on the new one.
        if (!_bInternal)
        {
            octaveScript* newScript = new octaveScript();
            newScript->set_filename(_script->get_full_filepath(),false);
            newScript->set_text(ui->textScript->text());
            _bInternal= true;
            return;
       }

       // If script was internal (i.e. detached from any project) and detached from this one too,
       // we don't need to do anything
        return;
    }

    if (proj==nullptr)
    {
        if (!_bInternal)
        {
            // In this case, current script was linked to a project but we need to detach.
            octaveScript* newScript = new octaveScript();
            newScript->set_filename(_script->get_full_filepath(),false);
            newScript->set_text(ui->textScript->text());
            _script = newScript;
            _bInternal= true;
        }
        _project = proj;
    }
}

// Subsequent section is taken from Octave source code. (Credits go where the credits are due).

void wndOctaveScript::marginClicked(int margin, int line, Qt::KeyboardModifiers state)
{
    if (_script == nullptr) return;

    if ((line < 0)||(line > ui->textScript->lines()))
        return;

    if (margin!=1) return;

    if (_data_interface==nullptr) return;

    if (margin == 1)
    {
        unsigned int markers_mask = ui->textScript->markersAtLine (line);

        if (state & Qt::ControlModifier)
        {
            //Add a simple bookmark
            if (markers_mask & (1 << marker::bookmark))
                ui->textScript->markerDelete (line, marker::bookmark);
            else
                ui->textScript->markerAdd (line, marker::bookmark);
        }
        else
        {
            if (_script->has_breakpoint_at_line(line))
                handle_request_remove_breakpoint (line + 1);
            else
            {
                if (_b_modified)
                    if (!save_file()) return;

                handle_request_add_breakpoint (line + 1, "");
            }
        }
    }

}

void wndOctaveScript::handle_request_remove_breakpoint(int line)
{


}
void wndOctaveScript::handle_request_add_breakpoint(int line, const QString& condition)
{
    add_breakpoint_event (line, condition);
}

void wndOctaveScript::toggle_breakpoint (const QWidget *ID)
{
    if (ID != this)
        return;

    int editor_linenr, cur;
    ui->textScript->getCursorPosition (&editor_linenr, &cur);

    if (ui->textScript->markersAtLine (editor_linenr) & (1 << marker::breakpoint))
        request_remove_breakpoint_via_editor_linenr (editor_linenr);
    else
    {
        if (!_b_modified)
            handle_request_add_breakpoint (editor_linenr + 1, "");
    }
}


void wndOctaveScript::next_breakpoint (const QWidget *ID)
{
    if (ID != this)
        return;

    int line, cur;
    ui->textScript->getCursorPosition (&line, &cur);

    line++; // Find breakpoint strictly after the current line.

    int nextline = ui->textScript->markerFindNext (line, (1 << marker::breakpoint));
    int nextcond = ui->textScript->markerFindNext (line, (1 << marker::cond_break));

    // Check if the next conditional breakpoint is before next unconditional one.
    if (nextcond != -1 && (nextcond < nextline || nextline == -1))
        nextline = nextcond;

    ui->textScript->setCursorPosition (nextline, 0);

}
void wndOctaveScript::remove_all_breakpoints (const QWidget *ID)
{
    if (ID != this)
        return;

    if (_script==nullptr)
        return;

    _script->remove_all_breakpoints();

/*    Q_EMIT interpreter_event
        ([this] (octave::interpreter& interp)
         {
             // INTERPRETER THREAD

             octave::tree_evaluator& tw = interp.get_evaluator ();
             octave::bp_table& bptab = tw.get_bp_table ();

             bptab.remove_all_breakpoints_from_file (_script->get_fullfilename().toStdString (),
                                                    true);
         });*/

}
void wndOctaveScript::previous_breakpoint (const QWidget *ID)
{
    if (ID != this)
        return;

    int line, cur, prevline, prevcond;
    ui->textScript->getCursorPosition (&line, &cur);

    line--; // Find breakpoint strictly before the current line.

    prevline = ui->textScript->markerFindPrevious (line, (1 << marker::breakpoint));
    prevcond = ui->textScript->markerFindPrevious (line, (1 << marker::cond_break));

    // Check if the prev conditional breakpoint is closer than the unconditional.
    if (prevcond != -1 && prevcond > prevline)
        prevline = prevcond;

    ui->textScript->setCursorPosition (prevline, 0);

}

void wndOctaveScript::add_breakpoint_event (int line, const QString& cond)
{

    if (_script==nullptr) return;

    _script->add_breakpoint(line,cond);

}

void wndOctaveScript::do_breakpoint_marker (bool insert, const QWidget *ID, int line,
                                           const QString& cond)
{
    if (ID != this || ID == nullptr)
        return;

    if (line > 0)
    {
        if (insert)
        {
            int editor_linenr = -1;
            marker *bp = nullptr;

            // If comes back indicating a nonzero breakpoint marker,
            // reuse it if possible
            Q_EMIT find_translated_line_number (line, editor_linenr, bp);
            if (bp != nullptr)
            {
                if ((cond == "") != (bp->get_cond () == ""))
                {
                    // can only reuse conditional bp as conditional
                    Q_EMIT remove_breakpoint_via_debugger_linenr (line);
                    bp = nullptr;
                }
                else
                    bp->set_cond (cond);
            }

            if (bp == nullptr)
            {
                bp = new marker (ui->textScript, line,
                                cond == "" ? marker::breakpoint
                                           : marker::cond_break, cond);

                connect (this, &wndOctaveScript::remove_breakpoint_via_debugger_linenr,
                        bp, &marker::handle_remove_via_original_linenr);
                connect (this, &wndOctaveScript::request_remove_breakpoint_via_editor_linenr,
                        bp, &marker::handle_request_remove_via_editor_linenr);
                connect (this, &wndOctaveScript::remove_all_breakpoints_signal,
                        bp, &marker::handle_remove);
                connect (this, &wndOctaveScript::find_translated_line_number,
                        bp, &marker::handle_find_translation);
                connect (this, &wndOctaveScript::find_linenr_just_before,
                        bp, &marker::handle_find_just_before);
                connect (this, &wndOctaveScript::report_marker_linenr,
                        bp, &marker::handle_report_editor_linenr);
                connect (bp, &marker::request_remove,
                        this, &wndOctaveScript::handle_request_remove_breakpoint);
            }
        }
        else
            Q_EMIT remove_breakpoint_via_debugger_linenr (line);
    }

}



void
wndOctaveScript::update_breakpoints ()
{
    if (_script->get_fullfilename().isEmpty ())
        return;


}

void
wndOctaveScript::update_breakpoints_handler (const octave_value_list& argout)
{
    octave_map dbg = argout(0).map_value ();
    octave_idx_type n_dbg = dbg.numel ();

    Cell file = dbg.contents ("file");
    Cell line = dbg.contents ("line");
    Cell cond = dbg.contents ("cond");

    for (octave_idx_type i = 0; i < n_dbg; i++)
    {
        if (file (i).string_value () == _script->get_fullfilename().toStdString ())
            do_breakpoint_marker (true, this, line (i).int_value (),
                                 QString::fromStdString (cond (i).string_value ()));
    }
}

void
wndOctaveScript::handle_remove_next (int remove_line)
{
    // Store some info breakpoint
    if (_script == nullptr) return;
    _script->remove_breakpoint(remove_line);
}



void
wndOctaveScript::file_has_changed (const QString&, bool do_close)
{
    bool file_exists = QFile::exists (_script->get_fullfilename());

    if (!file_exists)
    {
        if (QMessageBox::warning(this, "Warning", QString("The file ")+_script->get_fullfilename()+QString(" has been deleted outside the editor. Do you want to save the current text?"),
                                 QMessageBox::Yes|QMessageBox::No)==QMessageBox::No)
        {
            setModified(true);
            return;
        }

        save_file();


    }

    if (QMessageBox::warning(this, "Warning", QString("The file ")+_script->get_fullfilename()+QString(" has been changed outside the editor.\n Do you want to reload?"),
                             QMessageBox::Yes|QMessageBox::No)==QMessageBox::No)
    {
        setModified(true);
    }
    else
    {
        _script->load();
        setModified(false);
    }
}
