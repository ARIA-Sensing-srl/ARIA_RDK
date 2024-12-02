/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "uiElements/wndantennadesigner.h"
#include "uiElements/wndoctavescript.h"
#include "uiElements/wndradarinstanceeditor.h"
#include "uiElements/wndradarmoduleeditor.h"
#include "uiElements/wndscanmodules.h"
#include "uiElements/wndfwupload.h"
#include "uiElements/wndabout.h"
#include "uiElements/wndscheduler.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>
#include "octaveinterface.h"

#include <octave.h>
#include <interpreter.h>
#include <radarmodule.h>

class octaveInterface         *interfaceData=nullptr;   // Convenient copy

QDir             ariasdk_modules_path;
QDir             ariasdk_projects_path;
QDir             ariasdk_scripts_path;
QDir             ariasdk_antennas_path;
QDir             ariasdk_antennaff_path;

QString          ariasdk_fw_uploader;
QString          ariasdk_fw_bin;
QDir             ariasdk_uploader_path;
QDir             ariasdk_bin_path;
QString          ariasdk_flag_write;
QString          ariasdk_flag_start;
QString          ariasdk_flag_verify;
QString          ariasdk_bin_start_address;
QString          ariasdk_serial_name;



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    app_settings(QSettings::NativeFormat, QSettings::UserScope,
                 "ARIA Sensing", "ARIA UWB SDK") ,
    ui(new Ui::MainWindow),
    _plot2d_children(),
    _plot3d_children()
{
    ui->setupUi(this);
    // Radar Devices: menu signals
    connect(ui->actionScan,             &QAction::triggered, this, &MainWindow::radarModule_Scan);;
    connect(ui->actionNew_Device,       &QAction::triggered, this, &MainWindow::newDevice);
    connect(ui->actionDelete_Device,    &QAction::triggered, this, &MainWindow::deleteDevice);
    connect(ui->actionConfigure,        &QAction::triggered, this, &MainWindow::configureDevice);
    // Octave Interface: menu signals
    connect(ui->actionOctaveInterface,  &QAction::triggered, this, &MainWindow::octaveInterface_Open);
    // Antenna
    connect(ui->actionEvaluate_Antenna, &QAction::triggered,  this, &MainWindow::actionEvaluate_Antenna);
    // Project
    connect(ui->actionNew,      &QAction::triggered,  this, &MainWindow::newProject);
    connect(ui->actionOpem,     &QAction::triggered,  this, &MainWindow::loadProject);
    connect(ui->actionSave,     &QAction::triggered,  this, &MainWindow::saveProject);
    connect(ui->actionClose,    &QAction::triggered,  this, &MainWindow::closeProject);
    connect(ui->actionClone,    &QAction::triggered,  this, &MainWindow::cloneProject);
    // Radar Modules
    connect(ui->actionNew_Radar_Module,         &QAction::triggered, this, &MainWindow::newRadarModule);
    connect(ui->actionImport_Radar_Module,      &QAction::triggered, this, &MainWindow::importRadarModule);
    connect(ui->actionConfigure_RadarModule,    &QAction::triggered, this, &MainWindow::configureModule);
    connect(ui->actionDelete_Radar_Module,      &QAction::triggered, this, &MainWindow::deleteModule);
    // Project double-click
    connect(ui->treeProject, &QTreeWidget::itemDoubleClicked,  this, &MainWindow::tree_project_double_click);
    // About
    connect(ui->actionAbout, &QAction::triggered,  this, &MainWindow::showAbout);
    // Scheduler
    connect(ui->actionNew_Scheduler,        &QAction::triggered, this, &MainWindow::newScheduler);
    connect(ui->actionConfigure_Scheduler,  &QAction::triggered, this, &MainWindow::configureScheduler);
    connect(ui->actionDelete_Scheduler,     &QAction::triggered, this, &MainWindow::deleteScheduler);
    // FW Upload
    connect(ui->actionFW_Upload,            &QAction::triggered, this, &MainWindow::fw_upload);

    // Init other UI stuff
    initRadarTable();
    //
    wndOctaveInterface = nullptr;

    project = nullptr;
    read_option_file();

    this->showMaximized();
    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    octaveInterfaceCreate();

    m_qd = new ThreadLogStream(std::cout); //Redirect Console output to QTextEdit

    this->msgHandler = new MessageHandler(wndOctaveInterface==nullptr?nullptr:wndOctaveInterface->get_textoutput(), this);

    connect(m_qd, &ThreadLogStream::sendLogString, msgHandler, &MessageHandler::catchMessage);

    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

    //---------------------------------------------
    // Data/Octave section
    // Create thread for execution of octave commands

    octaveThread = nullptr;
    elabThread  = nullptr;
    createOctaveThread();

    // Connect thread error to this
#ifdef OCTAVE_THREAD
    connect(elabThread->getData(), &octaveInterface::workerError, this, &MainWindow::octaveError);
#else
    connect(interfaceData, &octaveInterface::workerError, this, &MainWindow::octaveError);
#endif

    if (wndOctaveInterface!=nullptr)
        wndOctaveInterface->update_octave_interface();
}

MainWindow::~MainWindow()
{
    if (wndOctaveInterface!=nullptr)
        delete wndOctaveInterface;
    wndOctaveInterface = nullptr;
    delete ui;
    deleteOctaveThread();
    if (project!=nullptr)
        delete project;
}

// Catch QMessage, redirect to cout
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
void MainWindow::QMessageOutput(QtMsgType , const QMessageLogContext &, const QString &msg)
{
    std::cout<<msg.toStdString().c_str()<<std::endl;
}


/*
 * Scan for all devices attached to the serial ports
 *
 */
void MainWindow::radarModule_Scan()
{
    // Get the list of available modules
    if (project==nullptr)
    {
        QMessageBox::critical(this, "Error", "A project with at least one radar module must be defined");
        return;
    }

    QVector<projectItem*> radar_modules = project->get_all_children(DT_RADARMODULE);
    if (radar_modules.isEmpty())
    {
        QMessageBox::critical(this, "Error", "No radar module is defined");
        return;
    }

    wndScanModules* scanWnd = new wndScanModules(radar_modules,project, this);
    scanWnd->exec();
    delete scanWnd;
}

void MainWindow::octaveInterfaceCreate()
{
    if (wndOctaveInterface==nullptr)
    {
        wndOctaveInterface = new mdiOctaveInterface(elabThread, this);
        // Connect closing
        // Octave Interface: menu signals
        connect(wndOctaveInterface,  &QDialog::destroyed, this, &MainWindow::octaveInterface_CloseWnd);
        // Connect also octave signals

#ifdef OCTAVE_THREAD
        if (elabThread!=nullptr)
        {
            connect(elabThread, &qDataThread::commandCompleted, wndOctaveInterface, &mdiOctaveInterface::octaveCompletedTask);
            connect(elabThread->getData(), &octaveInterface::workspaceUpdated, wndOctaveInterface, &mdiOctaveInterface::updateVarTable);
        }
#else
//        connect(interfaceData, &octaveInterface::commandCompleted, wndOctaveInterface, &mdiOctaveInterface::octaveCompletedTask);
//        connect(interfaceData, &octaveInterface::workspaceUpdated, wndOctaveInterface, &mdiOctaveInterface::updateVarTable);
        if ((interfaceData!=nullptr)&&(wndOctaveInterface!=nullptr))
        {
            connect(interfaceData,&octaveInterface::updatedVariable,    wndOctaveInterface,&mdiOctaveInterface::updatedSingleVar);
            connect(interfaceData,&octaveInterface::updatedVariables,   wndOctaveInterface,&mdiOctaveInterface::updatedVars);
        }
#endif
        connect(ui->actionOctaveScriptNew, &QAction::triggered,  this, &MainWindow::newOctaveScript);
        // Enable menus
        ui->menuOctaveScripts->setEnabled(true);

        ui->mdiArea->addSubWindow(wndOctaveInterface);
        wndOctaveInterface->showMaximized();
    }
}
/*
 * Open Octave Interface window
 *
 */
void MainWindow::octaveInterface_Open()
{
    if (wndOctaveInterface==nullptr)
        octaveInterfaceCreate();

    wndOctaveInterface->showMaximized();
}

/*
 * Open Octave Interface window
 *
 */
void MainWindow::octaveInterface_CloseWnd()
{
    ui->menuOctaveScripts->setEnabled(false);
    wndOctaveInterface = nullptr;
}

void MainWindow::deleteOctaveThread()
{
#ifdef OCTAVE_THREAD
    if (octaveThread!=nullptr)
    {
        if (interfaceData!=nullptr) interfaceData->stopThread(true);
        if (elabThread->isPaused()) elabThread->resume();

        if (octaveThread!=nullptr)
        {
            octaveThread->quit();
            octaveThread->wait();
        }
        //while (!elabThread->isReady()){};
    }
#else
    if (interfaceData!=nullptr)
        delete interfaceData;
#endif
}

void MainWindow::createOctaveThread()
{
#ifdef OCTAVE_THREAD
    if (octaveThread!=nullptr)
        deleteOctaveThread();

    octaveThread = new QThread(this);

    elabThread = new qDataThread();
    elabThread->moveToThread(octaveThread);

    connect( elabThread, &qDataThread::error, this, &MainWindow::octaveError);
    connect( octaveThread, &QThread::started, elabThread, &qDataThread::processDataThread);

    connect( elabThread, &qDataThread::finished, octaveThread, &QThread::quit);
    connect( elabThread, &qDataThread::finished, elabThread, &qDataThread::deleteLater);
    connect( octaveThread, &QThread::finished, octaveThread, &QThread::deleteLater);

    connect( elabThread, &qDataThread::commandCompleted, this, &MainWindow::octaveCompletedTask);

    octaveThread->start();

    interfaceData = elabThread->getData();
#else
    interfaceData = new octaveInterface();
#endif
    if ((interfaceData!=nullptr)&&(wndOctaveInterface!=nullptr))
    {
        connect(interfaceData,&octaveInterface::updatedVariable,    wndOctaveInterface,&mdiOctaveInterface::updatedSingleVar);
        connect(interfaceData,&octaveInterface::updatedVariables,   wndOctaveInterface,&mdiOctaveInterface::updatedVars);
    }
}


void MainWindow::octaveCompletedTask(QString task)
{
    // Do nothing, by now
}

void MainWindow::octaveError(QString error)
{
    QMessageBox::critical(this,"Error",error);
}


void MainWindow::initRadarTable()
{
    // Update the list according to the project

}

void MainWindow::newOctaveScript()
{
    if (wndOctaveInterface==nullptr) return;
    wndOctaveInterface->newScript();

}

//---------------------------------------------------------------
// Antenna Designer
void MainWindow::actionEvaluate_Antenna()
{
    wndAntennaDesigner* wndAntenna = new wndAntennaDesigner(nullptr, ui->mdiArea);
    if (wndAntenna==nullptr)
        return;
    wndAntenna->setDataEngine(interfaceData);
    ui->mdiArea->addSubWindow(wndAntenna);
    wndAntenna->showMaximized();

}

//---------------------------------------------------------------
// Project management
void MainWindow::newProject()
{
    if (project!=nullptr)
    {
        int reply;
        QMessageBox qbox;
        qbox.setWindowTitle(QString("Project")+project->get_root()->get_item_descriptor());
        qbox.setText(QString("Do you want to save project: ")+project->get_root()->get_item_descriptor());
        qbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        qbox.setDefaultButton(QMessageBox::Cancel);

        reply = qbox.exec();
        if (reply==QMessageBox::Cancel)
            return;

        if (reply==QMessageBox::Yes)
            if (!project->save_project_file())
            {
                QMessageBox::critical(this, "Error", "Error while saving project");
                return;
            }

        delete project;
        project = nullptr;
    }
    QString projectFile = QFileDialog::getSaveFileName(this,"New ARIA Radar Project",
                                                       ariasdk_projects_path.absolutePath()+QDir::separator()+QString("New project.arp"),
                                                       tr("ARIA Radar Project(*.arp);;All files (*.*)"),
                                                       nullptr,
                                                       QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));
    if (projectFile.isEmpty())
        return;


    QFileInfo fi(projectFile);
    if (fi.suffix()!="arp")
        projectFile+=".arp";

    QDir project_dir(fi.absoluteDir());
    project_dir.setPath(project_dir.cleanPath(project_dir.absoluteFilePath(fi.baseName())));

    if (project_dir.exists())
        if (!project_dir.isEmpty())
        {
            QMessageBox::critical(this, "Error", "Project dir must be empty");
            return;
        }
    ariasdk_projects_path.setPath(QFileInfo(projectFile).absolutePath());


    project = new radarProject(projectFile,interfaceData==nullptr ? nullptr : interfaceData->get_workspace(),true);

    updateProjectTree();

    connect(project,&radarProject::project_updated,     this, &MainWindow::updateProjectTree);
    connect(project,&radarProject::add_item,            this, &MainWindow::add_project_item);
    connect(project,&radarProject::remove_item,         this, &MainWindow::remove_project_item);
    connect(project,&radarProject::item_updated,        this, &MainWindow::update_project_item);

    project->save_project_file();
}
//---------------------------------------------------------------
// Tree item graphic stuff
void MainWindow::setTreeItem(QTreeWidgetItem* treeItem, projectItem* projItem)
{
    if (projItem==nullptr) return;
    if (treeItem==nullptr) return;
    QColor iconColor("lightGray");
    QString iconSource;
    if (projItem->get_type()==DT_ROOT)
        iconSource = ":/icons/folder-favorite-icon.png";
    if (projItem->get_type()==DT_FOLDER)
        iconSource = ":/icons/open-folder-outline-icon.png";
    if (projItem->get_type()==DT_RADARDEVICE)
        iconSource = ":/icons/radar-tracking-icon.png";
    if (projItem->get_type()==DT_RADARMODULE)
        iconSource = ":/icons/podcast-icon.png";
    if (projItem->get_type()==DT_SCRIPT)
        iconSource = ":/icons/accounting-icon.png";
    if (projItem->get_type()==DT_SCHEDULER)
        iconSource = ":/icons/project-management-timeline-icon.png";
    if (projItem->get_type()==DT_PROTOCOL)
        iconSource = ":/icons/speaking-bubbles-b-w-icon.png";
    if (projItem->get_type()==DT_OTHER)
        iconSource = ":/icons/file-question-icon.png";

    QPixmap px(iconSource);
    QPainter painter(&px);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.setBrush(iconColor);
    painter.setPen(iconColor);
    painter.drawRect(px.rect());
    QIcon icon(px);

    treeItem->setData(0, Qt::UserRole, QVariant::fromValue<void*>(projItem));

    treeItem->setText(0,projItem->get_item_descriptor());
    treeItem->setIcon(0, icon);

}
//---------------------------------------------------------------
// Create a tree node for the project
void MainWindow::insertProjectNode(projectItem* projItem, QTreeWidgetItem* parent)
{
    QTreeWidgetItem *treeItem = new QTreeWidgetItem();
    setTreeItem(treeItem, projItem);

    if (parent==nullptr)
        ui->treeProject->addTopLevelItem(treeItem);
    else
        parent->addChild(treeItem);

    for (int n=0; n < projItem->child_count(); n++)
        insertProjectNode(projItem->child_item(n), treeItem);
}



//---------------------------------------------------------------
// Project management
void MainWindow::updateProjectTree(projectItem* ptr)
{
    if (ptr==nullptr)
    {
        ui->treeProject->clear();
        if (project==nullptr) return;
        insertProjectNode(project->get_root(), nullptr);
    }
    else
    {
        QTreeWidgetItem* item = getWidgetItem(ptr);
        if (item==nullptr)
            return;

        foreach(auto i, item->takeChildren()) delete i;

        for (int n=0; n < ptr->child_count(); n++)
            insertProjectNode(ptr->child_item(n), item);
    }
}

QTreeWidgetItem* MainWindow::getWidgetItem(projectItem* item, QTreeWidgetItem* parent)
{
    if (parent ==nullptr)
    {
        if (ui->treeProject->topLevelItemCount() == 0) return nullptr;

        parent = ui->treeProject->topLevelItem(0);
    }

    for (int n=0; n < parent->childCount(); n++)
    {
        projectItem *currentChild = (projectItem*)(parent->child(n)->data(0, Qt::UserRole).value<void*>());
        if (currentChild==item)
            return parent->child(n);
    }

    for (int n=0; n < parent->childCount(); n++)
    {
        QTreeWidgetItem *result = getWidgetItem(item, parent->child(n));
        if (result!=nullptr) return result;
    }
    return nullptr;
}

void MainWindow::newRadarModule()
{
    if (project==nullptr)
    {
        QMessageBox::critical(this, "Error", "A project must be defined");
        return;
    }
    wndRadarModuleEditor *radarModuleEditor = new wndRadarModuleEditor(nullptr, project, this);

    ui->mdiArea->addSubWindow(radarModuleEditor);
    radarModuleEditor->showMaximized();


}

void MainWindow::add_project_item(projectItem* parent, projectItem *item)
{
    if (project==nullptr)
        return;

    if (parent==nullptr)
        parent = project->get_root();

    QTreeWidgetItem *parent_widget_item = getWidgetItem(parent);

    insertProjectNode(item, parent_widget_item);
}

void MainWindow::remove_project_item(projectItem* parent, projectItem *item)
{
    if (project==nullptr)
        return;

    if (parent==nullptr)
        parent = project->get_root();

    QTreeWidgetItem *parent_widget_item = getWidgetItem(parent);

    if (parent_widget_item==nullptr) return;

    QTreeWidgetItem *widget_item_to_delete = getWidgetItem(item,parent_widget_item);
    if (widget_item_to_delete ==nullptr) return;

    parent_widget_item->removeChild(widget_item_to_delete);
    project->save_project_file();
}

//---------------------------------------------------------------
// Import new radar module definition
void MainWindow::importRadarModule()
{
    if (project==nullptr) return;

    QString projectFile = QFileDialog::getOpenFileName(this,"Load ARIA Radar Module",
                                                       ariasdk_modules_path.absolutePath()+QDir::separator() +QString("New Radar Module.arm"),
                                                       tr("ARIA Radar Module(*.arm);;All files (*.*)"),
                                                       nullptr,
                                                       QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));
    if (projectFile.isEmpty())
        return;

    ariasdk_modules_path.setPath(QFileInfo(projectFile).absolutePath());

    project->add_radar_module(projectFile, project->get_root()->get_folder("Radar modules"));

    QString error = project->get_last_error();

    if (!error.isEmpty())
    {
        QMessageBox::critical(this,"Error",error);
    }
    project->save_project_file();
}
//---------------------------------------------------------------
void MainWindow::configureModule()
{
    QList<QTreeWidgetItem*> items = ui->treeProject->selectedItems();
    if (items.count()!=1) return;

    QTreeWidgetItem* widget = items[0];
    projectItem *currentItem = (projectItem*)(widget->data(0, Qt::UserRole).value<void*>());
    if (currentItem==nullptr) return;
    if (currentItem->get_type() != DT_RADARMODULE)
        return;

    for (auto &child: ui->mdiArea->subWindowList())
    {
        if (typeid(child).name()==typeid(wndRadarModuleEditor).name())
        {
            if (((wndRadarModuleEditor*)(child))->get_radar_module()==(radarModule*)currentItem)
            {
                ((wndRadarModuleEditor*)(child))->showMaximized();
                return;
            }
        }
    }
    wndRadarModuleEditor *radarModuleEditor = new wndRadarModuleEditor((radarModule*)currentItem, project, this);
    ui->mdiArea->addSubWindow(radarModuleEditor);
    radarModuleEditor->showMaximized();
}

//---------------------------------------------------------------
// Open an item of the project
void MainWindow::tree_project_double_click(QTreeWidgetItem* widget, int column)
{
    if (widget==nullptr)
        return;

     projectItem *currentItem = (projectItem*)(widget->data(0, Qt::UserRole).value<void*>());

    if (currentItem->get_type() == DT_RADARMODULE)
     {
        for (auto &child: ui->mdiArea->subWindowList())
        {
         if (typeid(child).name()==typeid(wndRadarModuleEditor).name())
         {
             if (((wndRadarModuleEditor*)(child))->get_radar_module()==(radarModule*)currentItem)
             {
                 ((wndRadarModuleEditor*)(child))->showMaximized();
                 return;
             }
         }
        }

        radarModule *radar_module = (radarModule*)(currentItem);
        wndRadarModuleEditor *radarModuleEditor = new wndRadarModuleEditor(radar_module,project,this);
        connect(radarModuleEditor, &wndRadarModuleEditor::radar_saved, this, &MainWindow::update_radar_module);
        ui->mdiArea->addSubWindow(radarModuleEditor);
        radarModuleEditor->showMaximized();
     }

     if (currentItem->get_type() == DT_RADARDEVICE)
     {

         for (auto &child: ui->mdiArea->subWindowList())
         {
             if (typeid(child).name()==typeid(wndRadarInstanceEditor).name())
             {
                 if (((wndRadarInstanceEditor*)(child))->getRadarInstance()==(radarInstance*)currentItem)
                 {
                     ((wndRadarInstanceEditor*)(child))->showMaximized();
                     return;
                 }
             }
         }
        QVector<radarModule*> modules  = project->get_available_modules();

        radarInstance *radar_instance = (radarInstance*)(currentItem);
        wndRadarInstanceEditor *radarInstanceEditor = new wndRadarInstanceEditor(radar_instance,modules,this);
        if (interfaceData!=nullptr)
        {
            connect(interfaceData,&octaveInterface::updatedVariable,    radarInstanceEditor,&wndRadarInstanceEditor::variable_updated);
            connect(interfaceData,&octaveInterface::updatedVariables,   radarInstanceEditor,&wndRadarInstanceEditor::variables_updated);
        }
        ui->mdiArea->addSubWindow(radarInstanceEditor);
        radarInstanceEditor->showMaximized();

     }

    if (currentItem->get_type() == DT_SCRIPT)
    {
         if (wndOctaveInterface==nullptr) return;
         wndOctaveInterface->openProjectScript((octaveScript*)(currentItem));
         wndOctaveInterface->showMaximized();

        /*
        wndOctaveScript *script_editor = new wndOctaveScript((octaveScript*)(currentItem), interfaceData, this);
        connect(script_editor, SIGNAL(update_tree(projectItem*)),  this, &MainWindow::updateProjectTree(projectItem*)));
        ui->mdiArea->addSubWindow(script_editor);
        script_editor->showMaximized();
*/
    }

    if (currentItem->get_type() == DT_ANTENNA)
    {

        wndAntennaDesigner* wndAntenna = new wndAntennaDesigner((antenna*)(currentItem),this);
        wndAntenna->setDataEngine(interfaceData);
        ui->mdiArea->addSubWindow(wndAntenna);
        wndAntenna->showMaximized();
    }

    if (currentItem->get_type() == DT_SCHEDULER)
    {
        for (auto &child: ui->mdiArea->subWindowList())
        {
            if (typeid(child).name()==typeid(wndScheduler).name())
            {
                if (((wndScheduler*)(child))->getScheduler()==(opScheduler*)currentItem)
                {
                    ((wndScheduler*)(child))->showMaximized();
                    return;
                }
            }
        }
        wndScheduler* wndSched = new wndScheduler(this, project, (opScheduler*)(currentItem));
        ui->mdiArea->addSubWindow(wndSched);
        wndSched->showMaximized();
    }
}
//---------------------------------------------------------------
// If we save the radar module being edited, we need to update the
// radar module accordingly
//---------------------------------------------------------------

void MainWindow::update_radar_module(radarModule* source)
{
    updateProjectTree(source);
}




//---------------------------------------------------------------
// Closing stuff
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (QMessageBox::question(this,"Confirm","Confirm exit?")==QMessageBox::Yes)
    {
        update_option_file();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

//---------------------------------------------------------------
// Load options
void MainWindow::read_option_file()
{
    QString def_path = QString(".") + QDir::separator();
    app_settings.beginGroup("Paths");
    ariasdk_modules_path.setPath(app_settings.value(QString("modules_path"),def_path).toString());
    ariasdk_projects_path.setPath(app_settings.value(QString("project_path"),def_path).toString());
    ariasdk_scripts_path.setPath(app_settings.value(QString("script_path"),def_path).toString());
    ariasdk_antennas_path.setPath(app_settings.value(QString("antennas_path"),def_path).toString());
    ariasdk_antennaff_path.setPath(app_settings.value(QString("farfield_path"),def_path).toString());
    app_settings.endGroup();
    app_settings.beginGroup("FW_Upload");
    ariasdk_fw_uploader = app_settings.value("fw_loader").toString();
    ariasdk_fw_bin      = app_settings.value("fw_binary_file").toString();

    ariasdk_uploader_path.setPath(app_settings.value(QString("fw_uploader_path"), def_path).toString());
    ariasdk_bin_path.setPath(app_settings.value(QString("fw_bin_path"), def_path).toString());

    ariasdk_flag_write          = app_settings.value("fw_write_flag").toString();
    ariasdk_flag_start          = app_settings.value("fw_run_flag").toString();
    ariasdk_flag_verify         = app_settings.value("fw_verify_flag").toString();
    ariasdk_bin_start_address   = app_settings.value("fw_start_address","00000000").toString();
    ariasdk_serial_name         = app_settings.value("fw_serial_port","").toString();

    app_settings.endGroup();


}
//---------------------------------------------------------------
// Update options
void MainWindow::update_option_file()
{
    app_settings.beginGroup("Paths");
    app_settings.setValue(QString("modules_path"),ariasdk_modules_path.absolutePath()+QDir::separator());
    app_settings.setValue(QString("project_path"),ariasdk_projects_path.absolutePath()+QDir::separator());
    app_settings.setValue(QString("script_path"),ariasdk_scripts_path.absolutePath()+QDir::separator());
    app_settings.setValue(QString("antennas_path"),ariasdk_antennas_path.absolutePath()+QDir::separator());
    app_settings.setValue(QString("farfield_path"),ariasdk_antennaff_path.absolutePath()+QDir::separator());
    app_settings.endGroup();

    app_settings.beginGroup("FW_Upload");
    app_settings.setValue(QString("fw_loader"),         ariasdk_fw_uploader);
    app_settings.setValue(QString("fw_binary_file"),    ariasdk_fw_bin);
    app_settings.setValue(QString("fw_uploader_path"),  ariasdk_uploader_path.absolutePath()+QDir::separator());
    app_settings.setValue(QString("fw_bin_path"),       ariasdk_bin_path.absolutePath()+QDir::separator());
    app_settings.setValue(QString("fw_write_flag"),     ariasdk_flag_write);
    app_settings.setValue(QString("fw_run_flag"),       ariasdk_flag_start);
    app_settings.setValue(QString("fw_verify_flag"),    ariasdk_flag_verify);
    app_settings.setValue(QString("fw_start_address"),  ariasdk_bin_start_address);
    app_settings.setValue(QString("fw_serial_port"),    ariasdk_serial_name);

    app_settings.endGroup();

}

//---------------------------------------------------------------
// Load a new project
void MainWindow::loadProject()
{
    QString projectFile = QFileDialog::getOpenFileName(this,"Load ARIA Radar Project",
                                                       ariasdk_projects_path.absolutePath()+QDir::separator(),
                                                       tr("ARIA Radar Project(*.arp);;All files (*.*)"),
                                                       nullptr,
                                                       QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));
    if (projectFile.isEmpty())
        return;
    ariasdk_projects_path.setPath(QFileInfo(projectFile).absolutePath());

    if (project!=nullptr)
    {
        int reply;
        QMessageBox qbox;
        qbox.setWindowTitle(QString("Project")+project->get_root()->get_item_descriptor());
        qbox.setText(QString("Do you want to save project: ")+project->get_root()->get_item_descriptor());
        qbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        qbox.setDefaultButton(QMessageBox::Cancel);

        reply = qbox.exec();
        if (reply==QMessageBox::Cancel)
            return;

        if (reply==QMessageBox::Yes)
            if (!project->save_project_file())
            {
                QMessageBox::critical(this, "Error", "Error while saving project");
                return;
            }

        delete project;
        project = nullptr;
    }

    project = new radarProject(projectFile,interfaceData==nullptr ? nullptr : interfaceData->get_workspace(), false);

    project->move_to_new_basedir(QFileInfo(projectFile).absolutePath());
    updateProjectTree();

    connect(project,&radarProject::project_updated,     this, &MainWindow::updateProjectTree);
    connect(project,&radarProject::add_item,            this, &MainWindow::add_project_item);
    connect(project,&radarProject::remove_item,         this, &MainWindow::remove_project_item);
    project->save_project_file();

}

//---------------------------------------------------------------
// Save/update project
void MainWindow::saveProject()
{

    if (project==nullptr)
        return;
    project->save_project_file();

}

//---------------------------------------------------------------
// Clone Project
void MainWindow::cloneProject()
{
    if (project==nullptr)
        return;

    if (QMessageBox::question(this,"Confirm","Do you want to close current project?")==QMessageBox::No)
        return;

    delete project;
    project = nullptr;
    ui->treeProject->clear();
}
//---------------------------------------------------------------
// Close Project
void MainWindow::closeProject()
{
    if (project==nullptr)
        return;

    if (QMessageBox::question(this,"Confirm","Do you want to close current project?")==QMessageBox::No)
        return;

    // close all window
    for (auto &child: ui->mdiArea->subWindowList())
    {
        if (typeid(child).name()!=typeid(wndOctaveScript).name())
        {
            ((QDialog*)(child))->close();
        }
    }

    delete project;
    project = nullptr;
    ui->treeProject->clear();
}
//---------------------------------------------------------------
// Remove a wndPlot2d
void    MainWindow::delete_children(class wndPlot2d* child)
{
    _plot2d_children.removeAll(child);
}
//---------------------------------------------------------------
// Delete all children of mathGL
void    MainWindow::delete_children(class wndMathGL* child)
{
    _plot3d_children.removeAll(child);
}
//---------------------------------------------------------------
// Update a project item
void MainWindow::update_project_item(projectItem* updated)
{
    QTreeWidgetItem* treeItem= getWidgetItem(updated);
    if (treeItem==nullptr) return;
    setTreeItem(treeItem,updated);
}

//---------------------------------------------------------------
// Show About
void MainWindow::showAbout()
{
    wndAbout about(this);
    about.exec();
}

//---------------------------------------------------------------
// Devices
void MainWindow::newDevice()
{
    if (project==nullptr)
    {
        QMessageBox::critical(this, "Error", "No project open");
        return;
    }
    QVector<radarModule*> modules  = project->get_available_modules();

    wndRadarInstanceEditor *radarInstanceEditor = new wndRadarInstanceEditor(nullptr,modules,this);
    if (interfaceData!=nullptr)
    {
        connect(interfaceData,&octaveInterface::updatedVariable, radarInstanceEditor,&wndRadarInstanceEditor::variable_updated);
        connect(interfaceData,&octaveInterface::updatedVariables,radarInstanceEditor,&wndRadarInstanceEditor::variables_updated);
    }
    ui->mdiArea->addSubWindow(radarInstanceEditor);
    radarInstanceEditor->showMaximized();


}
//---------------------------------------------------------------
void MainWindow::deleteDevice()
{
    if (project==nullptr) return;
    QList<QTreeWidgetItem*> items = ui->treeProject->selectedItems();
    if (items.count()!=1) return;

    QTreeWidgetItem* widget = items[0];
    projectItem *currentItem = (projectItem*)(widget->data(0, Qt::UserRole).value<void*>());
    if (currentItem==nullptr) return;
    if (currentItem->get_type() != DT_RADARDEVICE)
        return;
    radarInstance *radar = (radarInstance*)(currentItem);
    if (QMessageBox::question(this, "Confirm", tr("Do you want to delete ")+radar->get_device_name()+tr(" ?"))==QMessageBox::No)
        return;

    project->remove_radar_instance(radar);

    if (!project->get_last_error().isEmpty())
    {QMessageBox::critical(this,tr("Error"), project->get_last_error()); return;}

    updateProjectTree();

    project->save_project_file();

}
//---------------------------------------------------------------
void MainWindow::configureDevice()
{
    if (project==nullptr) return;

    QList<QTreeWidgetItem*> items = ui->treeProject->selectedItems();
    if (items.count()!=1) return;

    QTreeWidgetItem* widget = items[0];
    projectItem *currentItem = (projectItem*)(widget->data(0, Qt::UserRole).value<void*>());
    if (currentItem==nullptr) return;
    if (currentItem->get_type() != DT_RADARDEVICE)
        return;

    for (auto &child: ui->mdiArea->subWindowList())
    {
        if (typeid(child).name()==typeid(wndRadarInstanceEditor).name())
        {
            if (((wndRadarInstanceEditor*)(child))->getRadarInstance()==(radarInstance*)currentItem)
            {
                ((wndRadarInstanceEditor*)(child))->showMaximized();
                return;
            }
        }
    }
    wndRadarInstanceEditor *radarInstanceEditor = new wndRadarInstanceEditor((radarInstance*)currentItem,
                                                                            QVector<radarModule*>({((radarInstance*)(currentItem))->get_module()}),this);
    if (interfaceData!=nullptr)
    {
        connect(interfaceData,&octaveInterface::updatedVariable, radarInstanceEditor,&wndRadarInstanceEditor::variable_updated);
        connect(interfaceData,&octaveInterface::updatedVariables,radarInstanceEditor,&wndRadarInstanceEditor::variables_updated);
    }
    ui->mdiArea->addSubWindow(radarInstanceEditor);
    radarInstanceEditor->showMaximized();
}

//---------------------------------------------------------------
void MainWindow::newScheduler()
{
    if (project==nullptr)
    {
        QMessageBox::critical(this, "Error", "No project open");
        return;
    }

    wndScheduler *radarInstanceEditor = new wndScheduler(this, project, nullptr);
    ui->mdiArea->addSubWindow(radarInstanceEditor);
    radarInstanceEditor->showMaximized();
}

//---------------------------------------------------------------
void MainWindow::deleteScheduler()
{
    if (project==nullptr) return;
    QList<QTreeWidgetItem*> items = ui->treeProject->selectedItems();
    if (items.count()!=1) return;

    QTreeWidgetItem* widget = items[0];
    projectItem *currentItem = (projectItem*)(widget->data(0, Qt::UserRole).value<void*>());
    if (currentItem==nullptr) return;
    if (currentItem->get_type() != DT_SCHEDULER)
        return;
    opScheduler *scheduler= (opScheduler*)(currentItem);

    if (scheduler == nullptr) return;
    if (QMessageBox::question(this, "Confirm", tr("Do you want to delete ")+scheduler->get_name()+tr(" ?"))==QMessageBox::No)
        return;

    project->remove_scheduler(scheduler);


    if (!project->get_last_error().isEmpty())
    {QMessageBox::critical(this,tr("Error"), project->get_last_error()); return;}

    updateProjectTree();

    project->save_project_file();

}
//---------------------------------------------------------------
void MainWindow::configureScheduler()
{
    if (project==nullptr) return;

    QList<QTreeWidgetItem*> items = ui->treeProject->selectedItems();
    if (items.count()!=1) return;

    QTreeWidgetItem* widget = items[0];
    projectItem *currentItem = (projectItem*)(widget->data(0, Qt::UserRole).value<void*>());
    if (currentItem==nullptr) return;
    if (currentItem->get_type() != DT_SCHEDULER)
        return;

    for (auto &child: ui->mdiArea->subWindowList())
    {
        if (typeid(child).name()==typeid(wndScheduler).name())
        {
            if (((wndScheduler*)(child))->getScheduler()==(opScheduler*)currentItem)
            {
                ((wndScheduler*)(child))->showMaximized();
                return;
            }
        }
    }
    wndScheduler *schEditor = new wndScheduler(this,project,(opScheduler*)currentItem);
    ui->mdiArea->addSubWindow(schEditor);
    schEditor->showMaximized();
}
//---------------------------------------------------------------
void MainWindow::deleteModule()
{
    if (project==nullptr) return;
    QList<QTreeWidgetItem*> items = ui->treeProject->selectedItems();
    if (items.count()!=1) return;

    QTreeWidgetItem* widget = items[0];
    projectItem *currentItem = (projectItem*)(widget->data(0, Qt::UserRole).value<void*>());
    if (currentItem==nullptr) return;
    if (currentItem->get_type() != DT_RADARMODULE)
        return;
    radarModule *radar_mod= (radarModule*)(currentItem);
    if (QMessageBox::question(this, "Confirm", tr("Do you want to delete ")+radar_mod->get_name()+tr(" ?"))==QMessageBox::No)
        return;

    project->remove_radar_module(radar_mod);

    if (!project->get_last_error().isEmpty())
    {QMessageBox::critical(this,tr("Error"), project->get_last_error()); return;}

    updateProjectTree();

    project->save_project_file();
}

//---------------------------------------------------------------
void MainWindow::fw_upload()
{
    wndFWUpload wndFWUpload(this);
    wndFWUpload.exec();
}
