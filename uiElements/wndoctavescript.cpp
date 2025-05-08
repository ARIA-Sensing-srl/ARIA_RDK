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

wndOctaveScript::wndOctaveScript(QString filename, octaveInterface* dataEngine,QWidget *parent, QString basedir) :
    QDialog(parent),
    _data_interface(dataEngine),
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


#ifdef USE_NATIVE_LEXER
	emit ui->textScript->undoAvailable(true);
    hl = new octaveSyntaxHighlighter(_data_interface,ui->textScript->document());
#else
	if ((filename.endsWith(".cpp"))||(filename.endsWith(".cc"))||(filename.endsWith(".c")))
		_lexer = new QsciLexerCPP(ui->textScript);
	else
		_lexer = new QsciLexerOctave(ui->textScript);

	_lexer->setColor(Qt::green, QsciLexerOctave::Operator);
	_lexer->setColor(Qt::darkYellow, QsciLexerOctave::Keyword);
	_lexer->setColor(Qt::gray, QsciLexerOctave::Identifier);
	_lexer->setColor(Qt::darkGreen, QsciLexerOctave::Command);
	_lexer->setColor(Qt::darkCyan, QsciLexerOctave::Number);

	ui->textScript->setLexer(_lexer);
	_api = new QsciAPIs(_lexer);

#endif

    _script = new octaveScript(filename);
    _bInternal = true;
    _script->attach_to_dataengine(dataEngine);

    if (filename.isEmpty())
    {
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
        _script->set_scriptfile(filename);
        _script->save();
        _b_need_save_as = true;
    }
    ui->textScript->setText(_script->get_text());

    connect(ui->btnOctaveScriptRun, &QPushButton::clicked, this, &wndOctaveScript::run_file);
    connect(ui->btnScriptSave,      &QPushButton::clicked, this, &wndOctaveScript::save_file);
    connect(ui->btnScriptSaveAs,    &QPushButton::clicked, this, &wndOctaveScript::save_file_as);
#ifdef USE_NATIVE_LEXER
    int fontWidth = QFontMetrics(ui->textScript->currentCharFormat().font()).averageCharWidth();
    ui->textScript->setTabStopDistance( 4 * fontWidth );
#endif
	ui->textScript->update();

	connect(ui->textScript, &QsciScintilla::textChanged, this, &wndOctaveScript::modified);
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

#ifdef USE_NATIVE_LEXER
	hl = new octaveSyntaxHighlighter(_data_interface,ui->textScript->document());
#else
	_lexer = new QsciLexerOctave(ui->textScript);
	_lexer->setColor(Qt::green, QsciLexerOctave::Operator);
	_lexer->setColor(Qt::darkYellow, QsciLexerOctave::Keyword);
	_lexer->setColor(Qt::gray, QsciLexerOctave::Identifier);
	_lexer->setColor(Qt::darkGreen, QsciLexerOctave::Command);
	_lexer->setColor(Qt::darkCyan, QsciLexerOctave::Number);
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
        _script->set_scriptfile(filename);
        _script->save();
        _b_need_save_as = true;
        _bInternal = true;
    }
    else
    {
        _b_need_save_as = false;
        _bInternal = false;
        _script->load();
        ui->textScript->setText(_script->get_text());


    }

	ui->textScript->update();
    connect(ui->btnOctaveScriptRun, &QPushButton::clicked, this, &wndOctaveScript::run_file);
    connect(ui->btnScriptSave,      &QPushButton::clicked, this, &wndOctaveScript::save_file);
	connect(ui->btnScriptSaveAs,    &QPushButton::clicked, this, &wndOctaveScript::save_file_as);
#ifndef USE_NATIVE_LEXER
	connect(ui->textScript, &QsciScintilla::textChanged, this, &wndOctaveScript::modified);

#endif
    this->setWindowTitle(QString("Octave Script:")+QFileInfo(_script->get_full_filepath()).fileName());
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
    this->setWindowTitle(_script->get_name());
}


void wndOctaveScript::loadFile(QString fname)
{
    update_wnd_title();
    connect(&watcher,SIGNAL(fileChanged(QString)), this, SLOT(fileChangedOnDisk(QString)));

    _script->set_scriptfile(fname);
    _script->load();
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


void wndOctaveScript::  save_file()
{
    if (_b_need_save_as) { save_file_as(); return;}

#ifdef USE_NATIVE_LEXER
	_script->set_text(ui->textScript->toPlainText());
#else
	_script->set_text(ui->textScript->text());
#endif

    _script->save();

	_b_modified = false;
	if (windowTitle().endsWith("*"))
		setWindowTitle(windowTitle().removeLast());

}

void wndOctaveScript::save_file_as()
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

    if (projectFile.isEmpty()) return;

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
    this->setWindowTitle(QString("Octave Script:")+QFileInfo(_script->get_full_filepath()).fileName());
	_b_modified = false;
	if (windowTitle().endsWith("*"))
		setWindowTitle(windowTitle().removeLast());

}



//---------------------------------------------
void wndOctaveScript::modified()
{
	if (!_b_modified)
	{
		setWindowTitle(this->windowTitle()+"*");
		_b_modified = true;
	}
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
