/*
 * ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 */
#include <QMessageBox>
#include <QCloseEvent>
#include <QPainter>
#include <QMenu>
#include <QFileDialog>

#include "mdioctaveinterface.h"
#include "wndoctavescript.h"
#include "ui_mdioctaveinterface.h"
#include "wndplot2d.h"
#include "dataengine.h"
#include "wndplotcolormap.h"
#include <octave.h>
#include <interpreter.h>

using namespace octave;

extern qDataEngine         *interfaceData;

mdiOctaveInterface::mdiOctaveInterface(qDataThread* worker,  QWidget *parent) :
    QDialog(parent),
    ui(new Ui::mdiOctaveInterface),
    _elabThread(worker),
    _interfaceData(interfaceData),
    _dataws(nullptr)
{
    ui->setupUi(this);

    // Connectin signals
    connect(ui->lineEdit, SIGNAL(editingFinished()),this, SLOT(newCommandLine()));

    // Workspace table
    ui->workspaceList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->workspaceList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(workspaceTableRightClick(QPoint)));

    connect(ui->btnOctaveScriptNew,    SIGNAL(clicked()), this, SLOT(newScript()));
    connect(ui->btnOctaveScriptLoad,   SIGNAL(clicked()), this, SLOT(openScript()));
    connect(ui->btnOctaveScriptSave,   SIGNAL(clicked()), this, SLOT(saveScript()));
    connect(ui->btnOctaveScriptSaveAs, SIGNAL(clicked()), this, SLOT(saveSctiptAs()));
    connect(ui->btnOctaveScriptClose,  SIGNAL(clicked()), this, SLOT(closeScript()));

    if (_interfaceData!=nullptr)
        _dataws = _interfaceData->getDataWorkspace();

    //-----------------------------------------------
    // Update workspace table
    QStringList columnHeader{"","Var","Size","Value"};
    ui->workspaceList->setHorizontalHeaderLabels(columnHeader);
    QStringList fileNames{":/icons/tech-icon.png",":/icons/shortcut-icon.png"};
    for (int n=0; n<2; n++)
    {
        QColor iconColor("lightGray");
        QPixmap px(fileNames.at(n));
        QPainter painter(&px);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.setBrush(iconColor);
        painter.setPen(iconColor);
        painter.drawRect(px.rect());
        switch (n)
        {
            case 0:
                iconInternal.addPixmap(px);
                break;
            case 1:
                iconOctave.addPixmap(px);
                break;
        }
    }

    updateVarTable();
}

mdiOctaveInterface::~mdiOctaveInterface()
{
    delete ui;
}


void mdiOctaveInterface::closeEvent( QCloseEvent* event )
{
    /*
    this->showMinimized();
    event->ignore();
*/
}


void mdiOctaveInterface::newScript()
{
    wndOctaveScript* wndScript = new wndOctaveScript("",_interfaceData,this);
    ui->mdiArea->addSubWindow(wndScript);
    wndScript->showMaximized();
}

void mdiOctaveInterface::openScript()
{

    QStringList filesToRead = QFileDialog::getOpenFileNames(this,"Open scrtip file",
                                                      "",
                                                      tr("Octave files (*.m);;Text files (*.txt);; C/C++ files (*.c *.cpp);; All files (*.*)"),
                                                      nullptr,
                                                      QFileDialog::Options(QFileDialog::Detail));

    for (int n=0; n < filesToRead.count(); n++)
    {
        QString fname = filesToRead.at(n);
        wndOctaveScript* wndScript = new wndOctaveScript(fname,_interfaceData,this);
        ui->mdiArea->addSubWindow(wndScript);
        wndScript->showMaximized();
    }
}

void mdiOctaveInterface::saveScript()
{
    wndOctaveScript* wnd = (wndOctaveScript*)(ui->mdiArea->activeSubWindow());
    if (wnd==nullptr) return;

    if (_scripts_children.contains(wnd))
        wnd->saveFile();
}

void mdiOctaveInterface::closeScript()
{
    wndOctaveScript* wnd = (wndOctaveScript*)(ui->mdiArea->activeSubWindow());
    if (wnd==nullptr) return;

    if (_scripts_children.contains(wnd))
    {
        wnd->close();
        delete wnd;
    }
}

void mdiOctaveInterface::saveSctiptAs()
{

}

void mdiOctaveInterface::error(QString errorString)
{
    QMessageBox::critical(this,"Error",errorString);    
}


/*
 * Send the new line to the command interpreter
 */

void mdiOctaveInterface::newCommandLine()
{
    QString strCommand = ui->lineEdit->text();
    if (strCommand.isEmpty())
        return;

    bool bRes = interfaceData->appendCommand(strCommand);
#ifdef OCTAVE_THREAD
    if (_elabThread->isPaused()) _elabThread->resume();
#else
    if (!bRes) error(QString("Error while parsing \n")+strCommand);
#endif
    ui->lineEdit->setText("");

}


void mdiOctaveInterface::octaveCompletedTask(QString task, int errorcode)
{
    // Do we need to update the table?
    if (_dataws==nullptr)
        return;
    ui->historyCommands->setTextColor( errorcode==0? QColor( "white" ):QColor("red") );
    ui->historyCommands->append(task);
}


void  mdiOctaveInterface::updateVarTable()
{
    if (_dataws==nullptr)
        return;

    ui->workspaceList->clearContents();
    QList<variable*> varList = _dataws->getVars();
    size_t varCount = varList.count();
    ui->workspaceList->setRowCount(varCount);
    //ui->workspaceList->setColumnCount(4);
    for (size_t varid = 0; varid<varCount; varid++)
    {
        variable* var = varList[varid];
        if (var==nullptr)
            continue;
        // Column 0: icon

        QTableWidgetItem *iconItem = new QTableWidgetItem();
        iconItem->setIcon(var->isInternal() ? iconInternal : iconOctave);

        ui->workspaceList->setItem(varid,0,iconItem);

        // Column 1: name
        ui->workspaceList->setItem(varid,1,new QTableWidgetItem(var->getName()));

        // Column 2: size
        QString strSize("");
        QVector<size_t> vSize = var->getSize();
        for (int sid = 0; sid < vSize.count(); sid++)
        {
            strSize += QString::number(vSize[sid]);
            if (sid < vSize.count()-1)
                strSize += QString(" x ");
        }

        ui->workspaceList->setItem(varid,2,new QTableWidgetItem(strSize));

        // Column 3: value type

        QString strVal = var->getClassName();
        if ((var->isNumber())&&(var->isComplex()))
            strVal = "complex";
        ui->workspaceList->setItem(varid,3,new QTableWidgetItem(strVal));
    }
}


void mdiOctaveInterface::workspaceTableRightClick(QPoint pos)
{
    QList<QTableWidgetItem *> selected = ui->workspaceList->selectedItems();
    if (selected.empty())
        return;

    QMenu *menu = new QMenu(this);
    QAction *plotAction = new QAction("Plot");
    QAction *plotColormap = new QAction("Colormap");
    connect(plotAction, SIGNAL(triggered()), this, SLOT(variablePlot()));
    connect(plotColormap, SIGNAL(triggered()), this,SLOT(variableImage()));
    menu->addAction(plotAction);
    menu->addAction(plotColormap);
    menu->popup(ui->workspaceList->viewport()->mapToGlobal(pos));
}

void mdiOctaveInterface::variablePlot()
{
    // Get the list of selected variables
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.isEmpty())
        return;

    if (_dataws==nullptr)
        return;
    wndPlot2d *plotwnd = new wndPlot2d(this);
    QList<variable*> vars;
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        //
        variable* var = _dataws->getVarByName(varname);
        vars.append(var);
        connect(_dataws, &dataWorkspace::variableChanged,plotwnd,&wndPlot2d::udpatedVariable);
        connect(_dataws, &dataWorkspace::variableDeleted,plotwnd,&wndPlot2d::deletedVariable);
    }

    ui->mdiArea->addSubWindow(plotwnd);


    plotwnd->showMaximized();
    for (int n=0; n<vars.count();n++)
        plotwnd->setVariable(vars.at(n));
}

void mdiOctaveInterface::variableImage()
{
    // Get the list of selected variables
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.isEmpty())
        return;

    if (_dataws==nullptr)
        return;


    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        //
        variable* var = _dataws->getVarByName(varname);

        if (var==nullptr) continue;
        dim_vector dims = var->getOctValue().dims();
        int relevant_size = 0;
        for (int n=0; n < dims.numel(); n++)
        {
            if (dims(n)>1) relevant_size++;
            if (dims(n)==0) {relevant_size=0; continue;}
        }
        if (relevant_size!=2)
            continue;

        wndPlotColormap *plotwnd = new wndPlotColormap(this);
        if (plotwnd==nullptr) return;
        plotwnd->set_variable(var);
        ui->mdiArea->addSubWindow(plotwnd);

        connect(_dataws, &dataWorkspace::variableChanged,plotwnd,&wndPlotColormap::udpatedVariable);
        connect(_dataws, &dataWorkspace::variableDeleted,plotwnd,&wndPlotColormap::deletedVariable);
        plotwnd->showMaximized();
    }
//    for (int n=0; n<vars.count();n++)
//        plotwnd->setVariable(vars.at(n));

}


void    mdiOctaveInterface::delete_children(class wndOctaveScript* child)
{
    _scripts_children.removeAll(child);
}
void    mdiOctaveInterface::delete_children(class wndPlot2d* child)
{
    _plot2d_children.removeAll(child);
}
