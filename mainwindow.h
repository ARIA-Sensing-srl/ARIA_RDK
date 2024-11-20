/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>

#include "uiElements/mdioctaveinterface.h"
#include "radarproject.h"
#include "q_debugstream.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#undef OCTAVE_THREAD

extern class octaveInterface         *interfaceData;   // Convenient copy

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void        delete_children(class wndPlot2d* child);
    void        delete_children(class wndMathGL* child);
    // QMessage
    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    static  void QMessageOutput(QtMsgType , const QMessageLogContext &, const QString &msg);
    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

private:
    QThread                   *octaveThread;
    class qDataThread         *elabThread;

    mdiOctaveInterface *wndOctaveInterface;
    radarProject        *project;
    QSettings           app_settings;

private:
    void deleteOctaveThread();
    void createOctaveThread();
    void setTreeItem(QTreeWidgetItem* treeItem, projectItem* projItem);
    void insertProjectNode(projectItem* projItem, QTreeWidgetItem* parent);

    QTreeWidgetItem* getWidgetItem(projectItem* item, QTreeWidgetItem* parent=nullptr);

    QFile            option_file;

    void            read_option_file();
    void            update_option_file();

    std::streambuf  *octavepbuf, *coutbackup;

    void            octaveInterfaceCreate();

public slots:

// Menù slots
    void octaveInterface_Open();
    void octaveInterface_CloseWnd();
// Radar Devices
    void newDevice();
    void deleteDevice();
    void configureDevice();

// Octave interface
    void octaveCompletedTask(QString task);
    void octaveError(QString error);
    void newOctaveScript();
// Radar Table
    void initRadarTable();

// Antenna Designer
    void actionEvaluate_Antenna();

// Project Handler
    void newProject();
    void updateProjectTree(projectItem* ptr=nullptr);
    void add_project_item(projectItem* parent, projectItem *item);
    void remove_project_item(projectItem* parent, projectItem *item);
    void update_project_item(projectItem* updated);
    void loadProject();
    void saveProject();
    void closeProject();
    void cloneProject();
    void tree_project_double_click(QTreeWidgetItem* widget, int column);
// Radar Modules
    void radarModule_Scan();
    void newRadarModule();
    void importRadarModule();
    void update_radar_module(radarModule* modified);
    void configureModule();
    void deleteModule();
// Scheduler
    void newScheduler();
    void configureScheduler();
    void deleteScheduler();
// FW Upload
    void fw_upload();

// About
    void showAbout();
protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    QVector<class wndPlot2d*>           _plot2d_children;
    QVector<class wndMathGL*>           _plot3d_children;

    // MessageHandler for display and ThreadLogStream for redirecting cout
    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    MessageHandler *msgHandler = Q_NULLPTR;
    ThreadLogStream* m_qd;
};
#endif // MAINWINDOW_H
