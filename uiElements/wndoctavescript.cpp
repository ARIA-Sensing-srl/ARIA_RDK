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
#include <parse.h>
#include "builtin-defun-decls.h"
#include "file-ops.h"
int wndOctaveScript::nNoname = 0;

extern QDir             ariasdk_modules_path;
extern QDir             ariasdk_projects_path;
extern QDir             ariasdk_scripts_path;
extern QDir             ariasdk_antennas_path;
extern QDir             ariasdk_antennaff_path;
extern QFont            ariasdk_script_font;
extern QString          m_current_directory;
//----------------------------------------------------
/**
 * @brief wndOctaveScript::wndOctaveScript
 * @param proj
 * @param filename
 * @param dataEngine
 * @param parent
 * @param basedir
 */
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
    _n_rows(0),
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
        create_new_script();
        _bInternal = true;

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
            create_new_script();
            _script->set_filename(filename,false);
            _bInternal = true;
        }
        else
            link_script_signals(_script);
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
    updateTitle();
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::wndOctaveScript
 * @param script
 * @param dataEngine
 * @param parent
 * @param basedir
 */
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
    _n_rows(0),
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
        link_script_signals(_script);
    }
    else
    {
        _project = _script->get_root();
        _b_need_save_as = false;
        _bInternal = false;
        _script->load();
        ui->textScript->setText(_script->get_text());
        link_script_signals(_script);
    }

    ui->textScript->update();
#ifndef USE_NATIVE_LEXER
    setupScintilla();
#endif
    this->setWindowTitle(QString("Octave Script:")+QFileInfo(_script->get_full_filepath()).fileName());

    createShortcutActions();

    _lexer->setFont(ariasdk_script_font);

    setInterpreterConnections();

    updateTitle();
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::~wndOctaveScript
 */

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
//----------------------------------------------------
/**
 * @brief wndOctaveScript::setInterpreterConnections
 * Create connection between the workspace update event
 * and this editor
 */

void wndOctaveScript::setInterpreterConnections()
{
    if (_data_interface==nullptr) return;
    if (_data_interface->engine_get_octave_engine()==nullptr) return;
    // This connections are used to enable / disable actions
    connect(_data_interface, &octaveInterface::signal_workspace_updated,    this, &wndOctaveScript::update_tips);


}


//----------------------------------------------------
/**
 * @brief wndOctaveScript::scriptRunning
 * Update the actions according to a script that is running
 */
void wndOctaveScript::scriptIdle(const QString& filename)
{
    // The engine is idle after filename.
    ui->textScript->markerDeleteAll(marker::debugger_position);
    ui->tbRun->setEnabled(true);
    ui->tbStep->setEnabled(true);
    ui->tbStop->setEnabled(true);
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::scriptRunning
 * Some script is running. We need to disable run/stop/step
 * Update the actions according to a script that is running
 */
void wndOctaveScript::scriptRunning(const QString& filename)
{
    ui->textScript->markerDeleteAll(marker::debugger_position);

    ui->tbRun->setEnabled(false);
    ui->tbStep->setEnabled(false);
    ui->tbStop->setEnabled(false);

}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::scriptDone
 */
void    wndOctaveScript::scriptDone(const QString& , bool)
{
    ui->textScript->markerDeleteAll(marker::debugger_position);

    ui->tbRun->setEnabled(true);
    ui->tbStep->setEnabled(true);
    ui->tbStop->setEnabled(true);
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::scriptError
 * @param error
 * @param line
 */

void    wndOctaveScript::scriptError(const QString& filename, const QString& error,int line)
{
    ui->tbRun->setEnabled(true);
    ui->tbStep->setEnabled(true);
    ui->tbStop->setEnabled(true);
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::scriptPaused
 * @param line
 */

void    wndOctaveScript::scriptPaused(const QString& filename,int line)
{
    QString fthis = _script == nullptr ? "" : _script->get_fullfilename();

    ui->textScript->markerDeleteAll(marker::debugger_position);
    bool beq = fthis==filename;
    // in Win32 (MSYS), the filename is, e.g. "c:/"...
    // but the file name is
    if (!beq)
    {
    }

    if (beq)
    {
        ui->tbRun->setEnabled(true);
        ui->tbStep->setEnabled(true);
        ui->tbStop->setEnabled(line>=0);


        if (line >= 0)
            ui->textScript->markerAdd(line, marker::debugger_position);
    }
    else
    {
        ui->tbRun->setEnabled(false);
        ui->tbStep->setEnabled(false);
        // Allows to stop the Engine from any other window
        ui->tbStop->setEnabled(line>=0);
    }
}

//----------------------------------------------------
/**
 * @brief wndOctaveScript::setupScintilla
 */
void wndOctaveScript::setupScintilla()
{
    QIcon bpIcon(":/icons/stop-blocked-icon.png");

    ui->textScript->setMargins(2); // 0 is for breakpoints position, 1 is for execution position, 2 is line number
    ui->textScript->setMarginType (1, QsciScintilla::SymbolMargin);
    ui->textScript->setMarginType (0, QsciScintilla::SymbolMargin);
    ui->textScript->setMarginSensitivity (1, true);
    ui->textScript->setMarginSensitivity (0, true);


    ui->textScript->markerDefine (QsciScintilla::RightTriangle, marker::bookmark);
    ui->textScript->setMarkerBackgroundColor (QColor (0, 0, 232), marker::bookmark);
    ui->textScript->markerDefine (bpIcon.pixmap(16,16), marker::breakpoint);
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
#ifndef _WIN32
    connect(ui->textScript, &QsciScintilla::marginClicked, this, &wndOctaveScript::marginClicked);
#endif
    // line numbers
    //ui->textScript->setMarginsForegroundColor (QColor (96, 96, 96));
    //ui->textScript->setMarginsBackgroundColor (QColor (232, 232, 220));
    ui->textScript->setMarginType (0, QsciScintilla::SymbolMargin);
    ui->textScript->setMarginType (1, QsciScintilla::SymbolMargin);
    ui->textScript->setMarginType (2, QsciScintilla::TextMargin);

    // other features
    ui->textScript->setBraceMatching (QsciScintilla::StrictBraceMatch);
    ui->textScript->setAutoIndent (true);
    ui->textScript->setIndentationWidth (2);
    ui->textScript->setIndentationsUseTabs (false);

    ui->textScript->setUtf8 (true);
#ifdef _WIN32
    ui->textScript->setEolMode(QsciScintilla::EolUnix);
#else
    ui->textScript->setEolMode(QsciScintilla::EolUnix);
#endif
    // auto completion
    ui->textScript->SendScintilla (QsciScintillaBase::SCI_AUTOCSETCANCELATSTART, false);
    // Margin lines
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(8);
    ui->textScript->setMarginsFont(font);
    QFontMetrics fmetric(font);
    ui->textScript->setMarginWidth(0, fmetric.maxWidth()+ 18);
    ui->textScript->setMarginLineNumbers(0, true);
    ui->textScript->setMarginsBackgroundColor(Qt::lightGray);
    ui->textScript->setCaretLineVisible(true);
    ui->textScript->setCaretLineBackgroundColor(QColor("#3c3c3c"));

}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::setDefaultColor
 * @param lexer
 */

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



//----------------------------------------------------
/**
 * @brief wndOctaveScript::createShortcutActions
 */
void wndOctaveScript::createShortcutActions()
{
    connect(ui->tbRun,      &QToolButton::clicked, this, &wndOctaveScript::run_file);
    connect(ui->tbSave,     &QToolButton::clicked, this, &wndOctaveScript::save_file);
    connect(ui->tbSaveAs,   &QToolButton::clicked, this, &wndOctaveScript::save_file_as);
    connect(ui->tbOpen,     &QToolButton::clicked, this, &wndOctaveScript::open_file);
    connect(ui->tbNew,      &QToolButton::clicked, this, &wndOctaveScript::new_file);
    connect(ui->tbStop,     &QToolButton::clicked, this, &wndOctaveScript::request_stop_engine);
    connect(ui->tbStep,     &QToolButton::clicked, this, &wndOctaveScript::request_step);
    connect(ui->tbToggleBreakpoint,
            &QToolButton::clicked, this, &wndOctaveScript::request_toggle_breakpoint);

    // At start-up, we need to check if the engine was running. So let's update the "enable" status of each action
    // according to the status of the _octave_engine
    const QString& fname = _script==nullptr? "" : _script->get_fullfilename();

    if (_data_interface==nullptr) return;
    switch (_data_interface->engine_get_status())
    {
    case (octaveThreadHandler::OTH_IDLE):
        scriptIdle(fname);
        break;
    case (octaveThreadHandler::OTH_DEBUG):
        if (_data_interface->engine_get_debug_script()!="")
            scriptPaused(_data_interface->engine_get_debug_script(), _data_interface->engine_get_debug_line());
        else
            scriptRunning("");
        break;
    case (octaveThreadHandler::OTH_NULL):
        scriptIdle(fname);
        break;
    case (octaveThreadHandler::OTH_RUNNING):
        if (_data_interface->engine_get_running_script()!=nullptr)
        {
            scriptRunning(_data_interface->engine_get_running_script()->get_fullfilename());
        }
        else
            scriptRunning("");
    }
}



//----------------------------------------------------
/**
 * @brief wndOctaveScript::loadFile
 * @param fname
 */

void wndOctaveScript::loadFile(QString fname)
{
    updateTitle();
    connect(&watcher,SIGNAL(fileChanged(QString)), this, SLOT(fileChangedOnDisk(QString)));

    _script->set_filename(fname,false);
    ui->textScript->setText(_script->get_text());
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::closeFile
 */

void wndOctaveScript::closeFile()
{
    if (_script!=nullptr)
        _script->breakpoint_remove_all();
}

//----------------------------------------------------
/**
 * @brief wndOctaveScript::request_stop_engine
 */

void wndOctaveScript::request_stop_engine()
{
    _script->do_request_stop();
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::request_step
 */

void wndOctaveScript::request_step()
{
    _script->do_request_step_in();
}

//----------------------------------------------------
/**
 * @brief wndOctaveScript::run_file
 */

void wndOctaveScript::run_file()
{
    update_path();
    int db_line = _script->getCurrentDebugPosition();
    if (db_line>=0)
    {
        _script->do_request_continue();
        return;
    }

    if (_b_modified)
        if (!save_file()) return;

    _script->skip_workspace_update(false);
    if (_data_interface==nullptr) return;
    breakpoint_update_all();
    _script->do_request_run();
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::update_tips
 */

void wndOctaveScript::update_tips()
{
    if (_data_interface->engine_get_status()!=octaveThreadHandler::OTH_IDLE) return;

#ifndef USE_NATIVE_LEXER
    _api->clear();

    if ((_data_interface!=nullptr)&&(_data_interface->engine_get_octave_engine()!=nullptr))
    {
        _data_interface->operation_wait_and_lock("update_tips");
        // Add variables
        std::list<std::string> varnames = _data_interface->engine_get_octave_engine()->variable_names();

        for (std::list<std::string>::iterator iter = varnames.begin(); iter!=varnames.end(); iter++)
            _api->add(QString::fromStdString(*iter));

        varnames = _data_interface->engine_get_octave_engine()->global_variable_names();

        for (std::list<std::string>::iterator iter = varnames.begin(); iter!=varnames.end(); iter++)
            _api->add(QString::fromStdString(*iter));

        varnames = _data_interface->engine_get_octave_engine()->top_level_variable_names();

        for (std::list<std::string>::iterator iter = varnames.begin(); iter!=varnames.end(); iter++)
            _api->add(QString::fromStdString(*iter));
        // Add functions


        std::list<std::string> funcnames= _data_interface->engine_get_octave_engine()->user_function_names();

        for (std::list<std::string>::iterator iter = funcnames.begin(); iter!=funcnames.end(); iter++)
        {
            _api->add(QString::fromStdString(*iter)+"( args )");

        }
        _data_interface->operation_unlock("update_tips");
    }
    _api->prepare();
    _lexer->setAPIs(_api);
#endif
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::showEvent
 * @param event
 */

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
//----------------------------------------------------
/**
 * @brief wndOctaveScript::fileChangedOnDisk
 * @param str
 */

void wndOctaveScript::fileChangedOnDisk(QString str)
{
    //    if (QMessageBox()
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::save_file
 * @return
 */


bool wndOctaveScript::save_file()
{
    if (_b_need_save_as) { return save_file_as(); }
    QStringList lines;

#ifdef USE_NATIVE_LEXER
    _script->set_text(ui->textScript->toPlainText().split("\n"));
#else
    _script->set_text(ui->textScript->text().toLatin1());
#endif

    _script->save();
    breakpoint_update_all();

    _b_modified = false;
    if (windowTitle().endsWith("*"))
        setWindowTitle(windowTitle().removeLast());

    return true;
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::create_new_script
 */

void wndOctaveScript::create_new_script()
{
    if (_script!=nullptr)
        link_script_signals(nullptr);
    // Should we delete it?
    _script = new octaveScript();
    link_script_signals(_script);
    _script->attach_to_dataengine(_data_interface);


}

//----------------------------------------------------
/**
 * @brief wndOctaveScript::link_script_signals
 * Link the underlying script object
 * @param script
 */

void wndOctaveScript::link_script_signals(octaveScript* script)
{
    connect(script, &octaveScript::script_run,          this, &wndOctaveScript::scriptRunning);
    connect(script, &octaveScript::script_stop,         this, &wndOctaveScript::scriptPaused);
    connect(script, &octaveScript::script_error,        this, &wndOctaveScript::scriptError);
    connect(script, &octaveScript::script_run_complete, this, &wndOctaveScript::scriptDone);

}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::new_file
 * Create a new script. If we are inside a project, the script is anyway independent.
 */
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
        create_new_script();
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
//----------------------------------------------------
/**
 * @brief wndOctaveScript::open_file
 */
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
        link_script_signals(_script);
        _script->set_filename(scriptFile,false);
        _bInternal = false;
    }

    if ((_bInternal)&&(!bNewInternal))
    {
        // The new one is a project script. Update the script pointer to the project's one
        _script = possibleOldScript;
        link_script_signals(_script);
        _script->set_filename(scriptFile,false);
        _bInternal = false;
    }

    if ((!_bInternal)&&(bNewInternal))
    {
        // The old one is a project script and the new one is not. We need to create a temporary octaveScript object
        create_new_script();
        _script->set_filename(scriptFile,false);
        _bInternal = true;
    }

    if ((_bInternal)&&(bNewInternal))
    {
        // The new one is not a project script, we need to create a temporary octaveScript object iff prev was null
        if (_script==nullptr)
        {
            create_new_script();
        }

        _bInternal = true;
        _script->set_filename(scriptFile,false);
    }
    ui->textScript->setText(_script->get_text());
    setModified(false);


    _b_need_save_as = false;
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::save_file_as
 * @return
 */
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
        create_new_script();

#ifdef USE_NATIVE_LEXER
    _script->set_text(ui->textScript->toPlainText());
#else
    _script->set_text(ui->textScript->text());
#endif
    _script->set_filename(projectFile);
    _script->save();
    breakpoint_update_all();

    if (!_bInternal)
        if (root != nullptr)
            root->add_script(_script);

    _b_need_save_as = false;
    setModified(false);
    return true;
}
//---------------------------------------------
/**
 * @brief wndOctaveScript::setModified
 * @param bmodified
 */
void wndOctaveScript::setModified(bool bmodified)
{
    _b_modified = bmodified;
    updateTitle();
}
//---------------------------------------------
/**
 * @brief wndOctaveScript::updateTitle
 */
void wndOctaveScript::updateTitle()
{
    this->setWindowTitle(QString("Octave Script:")+_script->get_fullfilename()+QString(_b_modified?"*":""));
}

//---------------------------------------------
/**
 * @brief wndOctaveScript::modified
 */
void wndOctaveScript::modified()
{

    setModified(true);
}

/**
 * @brief wndOctaveScript::closeEvent
 * @param event
 */
void wndOctaveScript::closeEvent( QCloseEvent* event )
{
    if (_script!=nullptr)
    {
        if (_data_interface!=nullptr)
            if ((_data_interface->engine_get_running_script()!=nullptr)&&(_data_interface->engine_get_running_script()==_script))
            {
                QMessageBox::warning(this,"Warning","Cannot exit while Octave engine is in debug/running mode. Wait for script completion or terminate the debug");
                event->ignore();
                _b_closed = false;
                return;
            }
    }
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
    update_path();
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
            link_script_signals(nullptr);
            octaveScript* newScript = new octaveScript();
            newScript->set_filename(_script->get_full_filepath(),false);
            newScript->set_text(ui->textScript->text());
            newScript->attach_to_dataengine(_data_interface);
            _script = newScript;
            link_script_signals(_script);
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
            link_script_signals(nullptr);

            octaveScript* newScript = new octaveScript();
            newScript->set_filename(_script->get_full_filepath(),false);
            newScript->set_text(ui->textScript->text());
            newScript->attach_to_dataengine(_data_interface);
            _script = newScript;
            link_script_signals(_script);

            _bInternal= true;
        }
        _project = proj;
    }
}

//----------------------------------------------------
/**
 * @brief wndOctaveScript::breakpoint_update_all
 * Delete all breakpoints in the Octave database and
 * then re-insert according to the list of bp markers
 */
void wndOctaveScript::breakpoint_update_all()
{
    update_path();
    if (_script==nullptr) return;
    _script->breakpoint_remove_all();
    for (int line = 0; line < ui->textScript->lines(); line++)
    {
        unsigned int markers_mask = ui->textScript->markersAtLine (line);

        if (markers_mask & (1 << marker::breakpoint))
        {
            _script->breakpoint_add(line);
        }
    }

}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::request_toggle_breakpoint
 * This handles the "set/unset breakpoint" action. It toggles a breakpoint at current cursor position
 */
void wndOctaveScript::request_toggle_breakpoint()
{
    int editor_linenr, cur;
    ui->textScript->getCursorPosition (&editor_linenr, &cur);
    marginClicked(1, editor_linenr, Qt::NoModifier);
}

//----------------------------------------------------
/**
 * @brief wndOctaveScript::marginClicked
 * React to the margin clicked. As of now, it toggles the breakpoint
 * @param margin
 * @param line
 * @param state
 */

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
            toggle_breakpoint(line);
        }
    }

}

void wndOctaveScript::update_path()
{

    QFileInfo fi(_script->get_fullfilename());

    QString cname = fi.absolutePath();

    if (cname != m_current_directory)
    {
        _data_interface->engine_get_octave_engine()->chdir(cname.toStdString());
        m_current_directory = cname;
    }
}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::toggle_breakpoint
 * Toggle a breakpoint at the requested line. If the line is not an active line (e.g. it is empty or it is a comment)
 * the marker is moved to the first available statement
 * @param line
 */
void wndOctaveScript::toggle_breakpoint (int line)
{
    update_path();
    // To get the actual line number, we need to have a file-saved script.
    if (_b_modified)
    {
        if (QMessageBox::question(this,"Confirm save?","To enable breakpoint, the file must be saved. \n Do you want to save and continue?")==QMessageBox::No)
            return;

        if (!save_file()) return;
    }
    // Get the actual line
    line = _script->breakpoint_get_line(line);

    unsigned int markers_mask = ui->textScript->markersAtLine (line);

    if (markers_mask & (1 << marker::breakpoint))
    {
        ui->textScript->markerDelete (line, marker::breakpoint);
        _script->breakpoint_remove(line);
    }
    else
    {
        ui->textScript->markerAdd (line, marker::breakpoint);

        _script->breakpoint_add(line);
    }

}
//----------------------------------------------------
/**
 * @brief wndOctaveScript::next_breakpoint
 * Move the cursor to the next marker
 * @param ID - unused as of now
 *
 */

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
//----------------------------------------------------
/**
 * @brief wndOctaveScript::remove_all_breakpoints
 * It handle the action of
 * @param ID
 */
void wndOctaveScript::remove_all_breakpoints (const QWidget *ID)
{
    if (ID != this)
        return;

    if (_script==nullptr)
        return;

    _script->breakpoint_remove_all();

}

//----------------------------------------------------
/**
 * @brief wndOctaveScript::previous_breakpoint
 * Move the cursor the previous breakpoint
 * @param ID
 */

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

//----------------------------------------------------
/**
 * @brief wndOctaveScript::file_has_changed
 * @param do_close
 */


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

