/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "wndoctavescript.h"
#include "ui_wndoctavescript.h"

#include "interpreter.h"
#include "octaveinterface.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "Qsci/qscilexeroctave.h"
#include "Qsci/qscilexercpp.h"
#include "Qsci/qsciscintilla.h"
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

    connect(ui->textScript, &QsciScintilla::textChanged, this, &wndOctaveScript::modified);
    createShortcutActions();
    _lexer->setFont(ariasdk_script_font);
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
	connect(ui->textScript, &QsciScintilla::textChanged, this, &wndOctaveScript::modified);

#endif
    this->setWindowTitle(QString("Octave Script:")+QFileInfo(_script->get_full_filepath()).fileName());

    createShortcutActions();

    _lexer->setFont(ariasdk_script_font);
}


void wndOctaveScript::setDefaultColor(QsciLexer* lexer)
{
    lexer->setColor(Qt::green, QsciLexerOctave::Operator);
    lexer->setColor(Qt::darkYellow, QsciLexerOctave::Keyword);
    lexer->setColor(Qt::white, QsciLexerOctave::Identifier);
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
    connect(ui->tbNew,     &QToolButton::clicked, this, &wndOctaveScript::new_file);

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

}

void wndOctaveScript::run_file()
{
    // Copy text into octaveScripot
    //if (_script==nullptr) return;
    //_script->set_text(ui->textScript->toPlainText());
    if (_data_interface==nullptr) return;
#ifdef USE_NATIVE_LEXER
	_data_interface->run(ui->textScript->toPlainText());
#else
	_data_interface->run(ui->textScript->text());
#endif


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
