/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "wndoctavescript.h"
#include "ui_wndoctavescript.h"

#include "octaveinterface.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

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
    hl(nullptr),
    ui(new Ui::wndOctaveScript)
{
    ui->setupUi(this);

    emit ui->textScript->undoAvailable(true);

    hl = new octaveSyntaxHighlighter(_data_interface,ui->textScript->document());

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

    int fontWidth = QFontMetrics(ui->textScript->currentCharFormat().font()).averageCharWidth();
    ui->textScript->setTabStopDistance( 4 * fontWidth );
}

wndOctaveScript::wndOctaveScript(octaveScript* script, class octaveInterface* dataEngine,QWidget *parent, QString basedir) : QDialog(parent),
    _data_interface(dataEngine),
    _script(script),
    _bInternal(true),
    _b_need_save_as(false),
    hl(nullptr),    
    ui(new Ui::wndOctaveScript)
{
    ui->setupUi(this);
    hl = new octaveSyntaxHighlighter(_data_interface,ui->textScript->document());

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
    connect(ui->btnOctaveScriptRun, &QPushButton::clicked, this, &wndOctaveScript::run_file);
    connect(ui->btnScriptSave,      &QPushButton::clicked, this, &wndOctaveScript::save_file);
    connect(ui->btnScriptSaveAs,    &QPushButton::clicked, this, &wndOctaveScript::save_file_as);
    this->setWindowTitle(QString("Octave Script:")+QFileInfo(_script->get_full_filepath()).fileName());
}

wndOctaveScript::~wndOctaveScript()
{
    if (hl!=nullptr) delete hl;
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

    _data_interface->run(ui->textScript->toPlainText());
}



void wndOctaveScript::fileChangedOnDisk(QString str)
{
//    if (QMessageBox()
}


void wndOctaveScript::  save_file()
{
    if (_b_need_save_as) { save_file_as(); return;}

    _script->set_text(ui->textScript->toPlainText());
    _script->save();
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

    _script->set_text(ui->textScript->toPlainText());
    _script->set_filename(projectFile);
    _script->save();

    if (!_bInternal)
        if (root != nullptr)
            root->add_script(_script);

    _b_need_save_as = false;
    this->setWindowTitle(QString("Octave Script:")+QFileInfo(_script->get_full_filepath()).fileName());
}
