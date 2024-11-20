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

namespace Ui {
class wndOctaveScript;
}

namespace octave
{
    class interpreter;
}

class wndOctaveScript : public QDialog
{
    Q_OBJECT

public:
    explicit wndOctaveScript(QString filename="", class octaveInterface* dataEngine=nullptr,QWidget *parent = nullptr, QString basedir="");
    explicit wndOctaveScript(octaveScript* script, class octaveInterface* dataEngine=nullptr,QWidget *parent = nullptr,QString basedir="");
    ~wndOctaveScript();

private:
    static int              nNoname;

    octaveInterface*        _data_interface;
    octaveScript*           _script;
    QFileSystemWatcher      watcher;
    bool                    _bInternal;
    bool                    _b_need_save_as;
    octaveSyntaxHighlighter *hl;
    Ui::wndOctaveScript *ui;

    void update_wnd_title();

signals:
    void update_tree(projectItem* item);
public:

    void loadFile(QString fname);
    void closeFile();
    void fileChanged();
    octaveScript* get_script() {return _script;}
public slots:
    void fileChangedOnDisk(QString str);
    void run_file();
    void save_file();
    void save_file_as();
};

#endif // WNDOCTAVESCRIPT_H
