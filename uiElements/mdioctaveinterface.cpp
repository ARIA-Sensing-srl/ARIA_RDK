/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include <QMessageBox>
#include <QCloseEvent>
#include <QPainter>
#include <QMenu>
#include <QFileDialog>
#include <QMdiSubWindow>
#include "mdioctaveinterface.h"
#include "wndoctavescript.h"
#include "ui_mdioctaveinterface.h"
#include "octaveinterface.h"
#include "wndplot2d.h"
#include "wnddatatable.h"
#include <octave.h>
#include <interpreter.h>


extern QDir             ariasdk_modules_path;
extern QDir             ariasdk_projects_path;
extern QDir             ariasdk_scripts_path;
extern QDir             ariasdk_antennas_path;
extern QDir             ariasdk_antennaff_path;
using namespace octave;

extern octaveInterface         *interfaceData;

QTextEdit*  mdiOctaveInterface::get_textoutput()
{
    return ui->outputCommands;
}

mdiOctaveInterface::mdiOctaveInterface(qDataThread* worker,  QWidget *parent) :
    QDialog(parent),
    ui(new Ui::mdiOctaveInterface),
    _elabThread(worker),
    _interfaceData(interfaceData),
    _workspace(nullptr),
    _scripts_children(),
    _plot2d_children(),
    _b_deleting(false),
    _project(nullptr)
{
    ui->setupUi(this);

    // Connectin signals
    connect(ui->lineEdit,  &QLineEdit::returnPressed,this, &mdiOctaveInterface::newCommandLine);
    connect(ui->pbExecute, &QPushButton::clicked, this, &mdiOctaveInterface::newCommandLine);
    ui->historyCommands->viewport()->installEventFilter(this);

    // Workspace table
    ui->workspaceList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->workspaceList, &QTableWidget::customContextMenuRequested, this, &mdiOctaveInterface::workspaceTableRightClick);

    connect(ui->btnOctaveScriptNew,    &QPushButton::clicked, this, &mdiOctaveInterface::newScript);
    connect(ui->btnOctaveScriptLoad,   &QPushButton::clicked, this, &mdiOctaveInterface::openScript);
    connect(ui->btnExport,             &QPushButton::clicked, this, &mdiOctaveInterface::saveWorkspaceData);

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

    connect(ui->btnCleanOutput, &QPushButton::clicked, this, &mdiOctaveInterface::cleanOutput);
    connect(ui->pbCleanHistory, &QPushButton::clicked, this, &mdiOctaveInterface::cleanHistory);
    connect(ui->workspaceList, &QTableWidget::doubleClicked, this, &mdiOctaveInterface::workspaceTableDblClick);

}

void mdiOctaveInterface::cleanOutput()
{
    ui->outputCommands->clear();
}

void mdiOctaveInterface::cleanHistory()
{
    ui->historyCommands->clear();
}


void mdiOctaveInterface::update_octave_interface()
{
    connect(interfaceData, &octaveInterface::commandCompleted, this, &mdiOctaveInterface::octaveCompletedTask);
    connect(interfaceData, &octaveInterface::workspaceUpdated, this, &mdiOctaveInterface::updateVarTable);
    _interfaceData = interfaceData;
    if (_interfaceData!=nullptr)
        _workspace = _interfaceData->get_workspace();

    updateVarTable();
}

mdiOctaveInterface::~mdiOctaveInterface()
{
    _b_deleting = true;
    _plot2d_children.clear();
    _scripts_children.clear();
    delete ui;
}


void mdiOctaveInterface::closeEvent( QCloseEvent* event )
{
    this->showMinimized();
    this->hide();
    event->ignore();

}


void mdiOctaveInterface::newScript()
{
    wndOctaveScript* wndScript = new wndOctaveScript(_project,"",_interfaceData,this);
    ui->mdiArea->addSubWindow(wndScript);
    wndScript->showMaximized();

	connect(_interfaceData, &octaveInterface::workspaceUpdated, wndScript, &wndOctaveScript::update_tips);
}


void mdiOctaveInterface::openProjectScript(octaveScript* script)
{
	QList<QMdiSubWindow*> subwnds = ui->mdiArea->subWindowList();
    for (auto &child: subwnds)
    {
		wndOctaveScript* wnd = qobject_cast<wndOctaveScript*>(child->widget());

		if (wnd==nullptr) continue;
		octaveScript* wnd_script = wnd->get_script();

		if ((wnd_script!=nullptr)&&(script!=nullptr))
            if (wnd_script->get_fullfilename() == script->get_fullfilename())
			{
				((wndOctaveScript*)(child))->showMaximized();
				return;
			}
    }

    wndOctaveScript* wndScript = new wndOctaveScript(script,_interfaceData,this);
    ui->mdiArea->addSubWindow(wndScript);
    wndScript->showMaximized();

	connect(_interfaceData, &octaveInterface::workspaceUpdated, wndScript, &wndOctaveScript::update_tips);
}



QTextLine mdiOctaveInterface::currentTextLine(const QTextCursor &cursor)
{
    const QTextBlock block = cursor.block();
    if (!block.isValid())
        return QTextLine();

    const QTextLayout *layout = block.layout();
    if (!layout)
        return QTextLine();

    const int relativePos = cursor.position() - block.position();
    return layout->lineForTextPosition(relativePos);
}


bool mdiOctaveInterface::eventFilter(QObject *obj, QEvent *event) {

    if (event->type() == QMouseEvent::MouseButtonDblClick)
    {
        if (obj == ui->historyCommands->viewport())
        {
            //QTextLine tl = currentTextLine(ui->historyCommands->textCursor());
            int pos = ui->historyCommands->textCursor().position();
            QString strCopy = ui->historyCommands->toPlainText();
            qsizetype send = strCopy.indexOf("\n",pos);
            if (send==-1) send = strCopy.length();
            qsizetype start= strCopy.lastIndexOf("\n",pos);
            strCopy = strCopy.mid(start,send-start);
            ui->lineEdit->setText(strCopy);
        }

    }
    return QWidget::eventFilter(obj, event);
}


void mdiOctaveInterface::workspaceTableDblClick(const QModelIndex &index)
{
    viewDataInTable();
}


void mdiOctaveInterface::openScript()
{

    QStringList filesToRead = QFileDialog::getOpenFileNames(this,"Open script file",
                                                      ariasdk_scripts_path.absolutePath()+QDir::separator(),
                                                      tr("Octave files (*.m);;Text files (*.txt);; C/C++ files (*.c *.cpp);; All files (*.*)"),
                                                      nullptr,
                                                      QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));

    for (int n=0; n < filesToRead.count(); n++)
    {
        QString fname = filesToRead.at(n);

        QList<QMdiSubWindow*> subwnds = ui->mdiArea->subWindowList();

        bool bfound = false;
        for (auto &child: subwnds)
        {
            wndOctaveScript* wnd = qobject_cast<wndOctaveScript*>(child->widget());

            if ((wnd!=nullptr)&&(wnd->get_script()->get_fullfilename()==fname))
            {
                bfound = true;
                break;
            }

            if (!bfound)
            {
                wndOctaveScript* wndScript = new wndOctaveScript(_project, fname,_interfaceData,this);
                ui->mdiArea->addSubWindow(wndScript);
                wndScript->showMaximized();
                connect(_interfaceData, &octaveInterface::workspaceUpdated, wndScript, &wndOctaveScript::update_tips);
            }
            else
                wnd->showMaximized();
        }
    }

    if (filesToRead.count()>0)
        ariasdk_scripts_path.setPath(QFileInfo(filesToRead[0]).absolutePath());
}

void mdiOctaveInterface::saveScript()
{
    wndOctaveScript* wnd = (wndOctaveScript*)(ui->mdiArea->activeSubWindow());
    if (wnd==nullptr) return;

    if (_scripts_children.contains(wnd))
        wnd->save_file();
}

void mdiOctaveInterface::closeScript()
{
    wndOctaveScript* wnd = (wndOctaveScript*)(ui->mdiArea->activeSubWindow());
    if (wnd==nullptr) return;

	wnd->close();
	if (wnd->isClosed())
		delete wnd;

}


void mdiOctaveInterface::error(QString errorString)
{
	QColor backup_color = ui->outputCommands->textColor();
	ui->outputCommands->setTextColor(Qt::red);
	ui->outputCommands->append(errorString);
	ui->outputCommands->setTextColor(backup_color);
}


/*
 * Send the new line to the command interpreter
 */

void mdiOctaveInterface::newCommandLine()
{
    QString strCommand = ui->lineEdit->text();
    if (strCommand.isEmpty())
        return;
#ifdef OCTAVE_THREAD
    //bool bRes =
#endif
    interfaceData->appendCommand(strCommand);
#ifdef OCTAVE_THREAD
    if (_elabThread->isPaused()) _elabThread->resume();
#else
//    if (!bRes) error(QString("Error while parsing \n")+strCommand);
#endif
    ui->lineEdit->setText("");

}


void mdiOctaveInterface::octaveCompletedTask(QString task, int errorcode)
{
    // Do we need to update the table?
    if (_workspace==nullptr)
        return;

    //ui->outputCommands->append(_interfaceData->get_last_output());
    ui->historyCommands->setTextColor( errorcode==0? QColor( "white" ):QColor("red") );
    ui->historyCommands->append(task);

}

void mdiOctaveInterface::add_variable_row(int row, const std::string& name, const octave_value& val, bool internal)
{
    if (name.empty())
        return;

    QTableWidgetItem *iconItem = new QTableWidgetItem();
    iconItem->setIcon(internal ? iconInternal : iconOctave);
    ui->workspaceList->setItem(row,0,iconItem);

    // Column 1: name
    ui->workspaceList->setItem(row,1,new QTableWidgetItem(QString::fromStdString(name)));

    // Column 2: size
    dim_vector varsize = val.dims();
    QString strSize="";
    for (int sid = 0; sid < varsize.ndims(); sid++)
    {
        strSize += QString::number(varsize(sid));
        if (sid < varsize.ndims()-1)
            strSize += QString(" x ");
    }

    ui->workspaceList->setItem(row,2,new QTableWidgetItem(strSize));

    // Column 3: value type

    QString strVal = QString::fromStdString(val.class_name());

    if (val.is_complex_scalar()||val.is_complex_matrix())
        strVal = "complex";
    ui->workspaceList->setItem(row,3,new QTableWidgetItem(strVal));
}

void mdiOctaveInterface::clear_and_init_var_table()
{
	ui->workspaceList->clear();
	ui->workspaceList->setColumnCount(4);
	ui->workspaceList->setHorizontalHeaderLabels(QStringList({"","name","size","type"}));
	ui->workspaceList->setRowCount(0);
}

void  mdiOctaveInterface::updateVarTable()
{
	//return;
	// Get the list of selected rows
	/*
	QModelIndexList selected = ui->workspaceList->selectionModel()->selectedRows();
	QStringList		selectedVars;
	for (const auto& sel : selected)
		selectedVars.append(ui->workspaceList->item(sel.row(),1)->text());
*/
	QStringList vars = _interfaceData->get_workspace()->get_all_vars();

	// Remove unnecessary rows
	int n=0;
	while (n < ui->workspaceList->rowCount())
	{
		QString var_table = ui->workspaceList->item(n,1)->text();
		if (vars.contains(var_table))
			n++;
		else
			ui->workspaceList->removeRow(n);
	}

	int rmax = ui->workspaceList->rowCount();
	for (auto& v: vars)
	{
		bool bfound = false;
		for (int row = 0; (row < rmax)&&(!bfound); row++)
		{
			if (ui->workspaceList->item(row,1)->text()==v)
				bfound = true;
		}

		if (bfound) continue;
		std::string vstring = v.toStdString();
		bool internal = _workspace->is_internal(vstring);
		octave_value val = _workspace->var_value(vstring);
		add_variable_row(ui->workspaceList->rowCount(), vstring, val, internal);
	}
}

void mdiOctaveInterface::viewDataInTable()
{
    QModelIndexList selected = ui->workspaceList->selectionModel()->selectedRows();

    for (const auto& sel : selected)
    {
        QString vname = ui->workspaceList->item(sel.row(),1)->text();
        bool bfound = false;
        for (auto &child: ui->mdiArea->subWindowList())
        {
            wnddatatable* wnd = qobject_cast<wnddatatable*>(child->widget());
            if (wnd==nullptr)
                continue;

            if (wnd->getVariable()==vname)
            {
                wnd->showMaximized();
                bfound = true;
                break;
            }
        }

        if (!bfound)
        {
            wnddatatable* wnd = new wnddatatable(_interfaceData, vname, this);
            connect(interfaceData, &octaveInterface::updatedVariables, wnd, &wnddatatable::workSpaceModified);
            ui->mdiArea->addSubWindow(wnd);
            wnd->showMaximized();
        }
    }
}


void mdiOctaveInterface::workspaceTableRightClick(QPoint pos)
{
    QList<QTableWidgetItem *> selected = ui->workspaceList->selectedItems();
    if (selected.empty())
        return;

    QMenu *menu = new QMenu(this);
    QAction *view = new QAction("View");
    menu->addAction(view);
    QMenu *plot = menu->addMenu("Plot");
    QMenu *scatter = menu->addMenu("Scatter");
    QMenu *boxplot = menu->addMenu("Boxplot");
    QMenu *area   = menu->addMenu("Area");
    QMenu *barv   = menu->addMenu("Bar");
    QMenu* dens   = menu->addMenu("Density");
    QMenu* arrow  = menu->addMenu("Arrow plot");
    QMenu* vector = menu->addMenu("2D Vector field");
	QMenu* qwtplot= menu->addMenu("Fast plot");
	QMenu* qwtscatter = menu->addMenu("Fast scatter plot");
	QMenu* qwtimage   = menu->addMenu("Fast density plot");
    //---------- PLOT --------------------------
	QAction *plotNewPlot       = new QAction("New plot (each curve into own plot)");
	QAction *plotNewSinglePlot = new QAction("New plot (all data into the same plot)");
	QAction *plotNewSinglePlotXData = new QAction("New plot (x, y1,...yn)");
    connect(view, &QAction::triggered, this, &mdiOctaveInterface::viewDataInTable);
	connect(plotNewPlot, &QAction::triggered, this, &mdiOctaveInterface::variablePlot);
	connect(plotNewSinglePlot, &QAction::triggered, this, &mdiOctaveInterface::variablePlotAllInOne);
	connect(plotNewSinglePlotXData, &QAction::triggered, this, &mdiOctaveInterface::variablePlotXData);

    plotNewSinglePlotXData->setEnabled(ui->workspaceList->selectionModel()->selectedRows().count()>=2);

    plot->addAction(plotNewPlot);
    plot->addAction(plotNewSinglePlot);
    plot->addAction(plotNewSinglePlotXData);
    //---------- SCATTER --------------------------
	QAction *plotNewScatterPlot        = new QAction("New scatter-plot (each curve into own plot)");
	QAction *plotNewSingleScatterPlot = new QAction("New scatter-plot (all data into the same plot)");
	QAction *plotNewSingleScatterPlotXData   = new QAction("New scatter-plot (x, y1,...yn)");

    scatter->addAction(plotNewScatterPlot);
    scatter->addAction(plotNewSingleScatterPlot);
    scatter->addAction(plotNewSingleScatterPlotXData);

	connect(plotNewScatterPlot, &QAction::triggered, this, &mdiOctaveInterface::variableScatterPlot);
	connect(plotNewSingleScatterPlot, &QAction::triggered, this, &mdiOctaveInterface::variableScatterPlotAllInOne);
	connect(plotNewSingleScatterPlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableScatterPlotXData);
    //---------- BOXPLOT --------------------------
	QAction *plotNewBoxPlot        = new QAction("New Box-plot (each curve into own plot)");
	QAction *plotNewSingleBoxPlot = new QAction("New Box-plot (all data into the same plot)");
	QAction *plotNewSingleBoxPlotXData   = new QAction("New Box-plot (x, y1,...yn)");

    boxplot->addAction(plotNewBoxPlot);
    boxplot->addAction(plotNewSingleBoxPlot);
    boxplot->addAction(plotNewSingleBoxPlotXData);

	connect(plotNewBoxPlot, &QAction::triggered, this, &mdiOctaveInterface::variableBoxPlot);
	connect(plotNewSingleBoxPlot, &QAction::triggered, this, &mdiOctaveInterface::variableBoxPlotAllInOne);
	connect(plotNewSingleBoxPlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableBoxPlotXData);

    //---------- AREA --------------------------
	QAction *plotNewAreaPlot        = new QAction("New Area-plot (each curve into own plot)");
	QAction *plotNewSingleAreaPlot = new QAction("New Area-plot (all data into the same plot)");
	QAction *plotNewSingleAreaPlotXData   = new QAction("New Area-plot (x, y1,...yn)");

    area->addAction(plotNewAreaPlot);
    area->addAction(plotNewSingleAreaPlot);
    area->addAction(plotNewSingleAreaPlotXData);

	connect(plotNewAreaPlot, &QAction::triggered, this, &mdiOctaveInterface::variableAreaPlot);
	connect(plotNewSingleAreaPlot, &QAction::triggered, this, &mdiOctaveInterface::variableAreaPlotAllInOne);
	connect(plotNewSingleAreaPlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableAreaPlotXData);

    //---------- BAR --------------------------
	QAction *plotNewBarPlot        = new QAction("New vBar-plot (each curve into own plot)");
	QAction *plotNewSingleBarPlot = new QAction("New vBar-plot (all data into the same plot)");
	QAction *plotNewSingleBarPlotXData   = new QAction("New vBar-plot (x, y1,...yn)");

    barv->addAction(plotNewBarPlot);
    barv->addAction(plotNewSingleBarPlot);
    barv->addAction(plotNewSingleBarPlotXData);

	connect(plotNewBarPlot, &QAction::triggered, this, &mdiOctaveInterface::variableBarPlot);
	connect(plotNewSingleBarPlot, &QAction::triggered, this, &mdiOctaveInterface::variableBarPlotAllInOne);
	connect(plotNewSingleBarPlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableBarPlotXData);

    //---------- ARROW-------------------------
	QAction *plotNewArrowPlot        = new QAction("New vert. arrow plot (each curve into own plot)");
	QAction *plotNewSingleArrowPlot = new QAction("New vert. arrow plot (all data into the same plot)");
	QAction *plotNewSingleArrowPlotXData   = new QAction("New vert. arrow plot (x, y1,...yn)");

    arrow->addAction(plotNewArrowPlot);
    arrow->addAction(plotNewSingleArrowPlot);
    arrow->addAction(plotNewSingleArrowPlotXData);

	connect(plotNewArrowPlot, &QAction::triggered, this, &mdiOctaveInterface::variableArrowPlot);
	connect(plotNewSingleArrowPlot, &QAction::triggered, this, &mdiOctaveInterface::variableArrowPlotAllInOne);
	connect(plotNewSingleArrowPlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableArrowPlotXData);

    //---------- DENSITY-------------------------
	QAction *plotNewDensityPlot        = new QAction("New density plot (each image into own plot");
	QAction *plotNewDensityPlotXData   = new QAction("New density plot (rows,cols, data_1,  ... rows_n, cols_n, data_xn, data_n)");

    dens->addAction(plotNewDensityPlot);
    dens->addAction(plotNewDensityPlotXData);

	connect(plotNewDensityPlot, &QAction::triggered, this, &mdiOctaveInterface::variableDensityPlot);
	connect(plotNewDensityPlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableDensityPlotXData);
    //---------- VECTOR-------------------------
	QAction *plotNewVectorPlot      = new QAction("New vector plot (each quiver into own plot");
	QAction *plotNewVectorPlotXData = new QAction("New vector plot (rows,cols, data_x1, data_y1, ... rows_n, rows_n, data_xn, data_yn");

    vector->addAction(plotNewVectorPlot);
    vector->addAction(plotNewVectorPlotXData);
	connect(plotNewVectorPlot, &QAction::triggered, this, &mdiOctaveInterface::variableVectorPlot);
	connect(plotNewVectorPlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableVectorPlotXData);

	//---------- QWT PLOT-------------------------
	QAction *plotQwtNewPlot       = new QAction("New plot (each curve into own plot)");
	QAction *plotQwtNewSinglePlot = new QAction("New plot (all data into the same plot)");
	QAction *plotQwtNewSinglePlotXData = new QAction("New plot (x, y1,...yn)");

	qwtplot->addAction(plotQwtNewPlot);
	qwtplot->addAction(plotQwtNewSinglePlot);
	qwtplot->addAction(plotQwtNewSinglePlotXData);

	connect(plotQwtNewPlot, &QAction::triggered, this, &mdiOctaveInterface::variableQwtPlot);
	connect(plotQwtNewSinglePlot, &QAction::triggered, this, &mdiOctaveInterface::variableQwtPlotAllInOne);
	connect(plotQwtNewSinglePlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableQwtPlotXData);

	plotNewSinglePlotXData->setEnabled(ui->workspaceList->selectionModel()->selectedRows().count()>=2);
	//---------- QWT SCATTER --------------------------
	QAction *plotNewQwtScatterPlot        = new QAction("New scatter-plot (each curve into own plot)");
	QAction *plotNewQwtSingleScatterPlot = new QAction("New scatter-plot (all data into the same plot)");
	QAction *plotNewQwtSingleScatterPlotXData   = new QAction("New scatter-plot (x, y1,...yn)");

	qwtscatter->addAction(plotNewQwtScatterPlot);
	qwtscatter->addAction(plotNewQwtSingleScatterPlot);
	qwtscatter->addAction(plotNewQwtSingleScatterPlotXData);

	connect(plotNewQwtScatterPlot, &QAction::triggered, this, &mdiOctaveInterface::variableQwtScatterPlot);
	connect(plotNewQwtSingleScatterPlot, &QAction::triggered, this, &mdiOctaveInterface::variableQwtScatterPlotAllInOne);
	connect(plotNewQwtSingleScatterPlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableQwtScatterPlotXData);


	//---------- DENSITY-------------------------
	QAction *plotNewQwtDensityPlot        = new QAction("New density plot (each image into own plot");
	QAction *plotNewQwtDensityPlotXData   = new QAction("New density plot (rows,cols, data_1,  ... rows_n, cols_n, data_xn, data_n)");

	qwtimage->addAction(plotNewQwtDensityPlot);
	qwtimage->addAction(plotNewQwtDensityPlotXData);

	connect(plotNewQwtDensityPlot, &QAction::triggered, this, &mdiOctaveInterface::variableQwtDensityPlot);
	connect(plotNewQwtDensityPlotXData, &QAction::triggered, this, &mdiOctaveInterface::variableQwtDensityPlotXData);


	menu->popup(ui->workspaceList->viewport()->mapToGlobal(pos));
}

// ---------------------------------------------------------------------------------
// Plot callback

void mdiOctaveInterface::variablePlot()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();

    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        wnd2d->add_plot(varname,"");
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variablePlotAllInOne()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    int plot = -1;
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        wnd2d->add_plot(varname,"", plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
        plot = 0;
    }
    return;
}

void mdiOctaveInterface::variablePlotXData()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_plot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

// ---------------------------------------------------------------------------------
// Scatterplot callback

void mdiOctaveInterface::variableScatterPlot()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();

    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();

    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        wnd2d->add_scatterplot(varname,"");
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variableScatterPlotAllInOne()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    int plot = -1;
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        wnd2d->add_scatterplot(varname,"", plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
        plot = 0;
    }
    return;
}

void mdiOctaveInterface::variableScatterPlotXData()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_scatterplot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}
// ---------------------------------------------------------------------------------
// Boxplot callback
void mdiOctaveInterface::variableBarPlotAllInOne()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_barplot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variableBarPlot()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();

    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        wnd2d->add_barplot(varname,"");
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variableBarPlotXData()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_barplot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;

}
// ---------------------------------------------------------------------------------
// Boxplot callback
void mdiOctaveInterface::variableBoxPlot()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();

    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        wnd2d->add_boxplot(varname,"");
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variableBoxPlotAllInOne()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_boxplot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variableBoxPlotXData()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_boxplot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}
// ---------------------------------------------------------------------------------
// Area callback
void mdiOctaveInterface::variableAreaPlot()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();

    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        wnd2d->add_areaplot(varname,"");
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variableAreaPlotAllInOne()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_areaplot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variableAreaPlotXData()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_areaplot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}


// ---------------------------------------------------------------------------------
// Arrow callback
void mdiOctaveInterface::variableArrowPlot()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();

    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        wnd2d->add_arrowplot(varname,"");
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variableArrowPlotAllInOne()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_arrowplot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}

void mdiOctaveInterface::variableArrowPlotXData()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()<2)
        return;

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    int plot = -1;
    QString xname="";
    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();

        if (plot==-1)
        {
            xname = varname;
            plot = 0;
            continue;
        }
        wnd2d->add_arrowplot(varname,xname, plot);
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}
// ---------------------------------------------------------------------------------
// 7. Density
void mdiOctaveInterface::variableDensityPlot()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();

    foreach (QModelIndex index, indexList)
    {
        int row = index.row();
        // Get var name
        QString varname = ui->workspaceList->item(row,1)->text();
        wnd2d->add_density_plot(varname,"");
        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}


void mdiOctaveInterface::variableDensityPlotXData()
{
    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()%3!=0)
    {
        QMessageBox::critical(this, "Error", "Multiple of 3 selection is needed");
        return;
    }


    for (int n=0; n < indexList.count(); n+=3)
    {

        // Get var and indep var names
        QString namex   = ui->workspaceList->item(indexList.at(n).row(),1)->text();
        QString namey   = ui->workspaceList->item(indexList.at(n+1).row(),1)->text();

        QString varname = ui->workspaceList->item(indexList.at(n+2).row(),1)->text();
        wnd2d->add_density_plot(varname,namex,namey);

        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }
    return;
}
// ---------------------------------------------------------------------------------
// 8. Vector plot
void mdiOctaveInterface::variableVectorPlot()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()%2!=0)
    {
        QMessageBox::critical(this, "Error", "Even number of data is needed ");
        return;
    }

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QString varx="",vary="";

    for (int n=0; n< indexList.count(); n+=2)
    {
        varx = ui->workspaceList->item(indexList.at(n).row(),1)->text();
        vary = ui->workspaceList->item(indexList.at(n+1).row(),1)->text();
        wnd2d->add_vector_plot(varx,vary);

        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }

    return;
}



void mdiOctaveInterface::variableVectorPlotXData()
{
    QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
    if (indexList.count()%4!=0)
    {
        QMessageBox::critical(this, "Error", "Multiple of 4 selection is needed ");
        return;
    }

    wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
    ui->mdiArea->addSubWindow(wnd2d);
    wnd2d->showMaximized();
    _plot2d_children.append(wnd2d);

    QString varx="",vary="";
    QString namex="", namey="";

    for (int n=0; n< indexList.count(); n+=4)
    {
        namex= ui->workspaceList->item(indexList.at(n).row(),1)->text();
        namey= ui->workspaceList->item(indexList.at(n+1).row(),1)->text();

        varx = ui->workspaceList->item(indexList.at(n+2).row(),1)->text();
        vary = ui->workspaceList->item(indexList.at(n+3).row(),1)->text();
        wnd2d->add_vector_plot(varx,vary,namex,namey);

        QString last_error = wnd2d->last_error();
        if (!last_error.isEmpty())
        {
            QMessageBox::critical(this,"Error creating plot",last_error);
            //return;
        }
    }

    return;
}
// ---------------------------------------------------------------------------------
void mdiOctaveInterface::updatedSingleVar(const std::string& varname)
{
    if (varname.empty()) return;
    QString vname = QString::fromStdString(varname);

    octave_value var = _workspace->var_value(varname);
    bool internal = _workspace->is_internal(varname);
    for (int n=0; n< ui->workspaceList->rowCount(); n++)
    {
        QString varTable = ui->workspaceList->item(n,1)->text();
        if (vname != varTable) continue;

        // Column 2: size
        dim_vector varsize = var.dims();
        QString strSize="";
        for (int sid = 0; sid < varsize.ndims(); sid++)
        {
            strSize += QString::number(varsize(sid));
            if (sid < varsize.ndims()-1)
                strSize += QString(" x ");
        }
        if (ui->workspaceList->item(n,2)==nullptr)
            ui->workspaceList->setItem(n,2,new QTableWidgetItem(strSize));
        else
            ui->workspaceList->item(n,2)->setText(strSize);

        // Column 3
        QString strVal = QString::fromStdString(var.class_name());

        if (var.is_complex_scalar()||var.is_complex_matrix())
            strVal = "complex";

        if (ui->workspaceList->item(n,3)==nullptr)
            ui->workspaceList->setItem(n,3,new QTableWidgetItem(strVal));
        else
            ui->workspaceList->item(n,3)->setText(strVal);

        for (const auto& child: _plot2d_children)
		{
            if (child!=nullptr)
                //if (child->isFullScreen())
                child->update_workspace(_workspace);
		}

		for (const auto& child: _plot_qwt_children)
		{
			if (child!=nullptr)
				//if (child->isFullScreen())
				child->update_workspace();
		}

        return;
    }

    // Need to add a new row
    int nrow = ui->workspaceList->rowCount();
    ui->workspaceList->setRowCount(nrow+1);
    add_variable_row(nrow,varname,var,internal);

}
// ---------------------------------------------------------------------------------
void mdiOctaveInterface::updatedVars(const std::set<std::string>& varlist)
{
    if (_workspace==nullptr) return;
    int n=0;
    while (n < ui->workspaceList->rowCount())
    {
        QString vqname = ui->workspaceList->item(n,1)->text();
        std::string vname = vqname.toStdString();
        if ((!_workspace->is_internal(vname))&&(!_workspace->is_octave(vname)))
        {
            ui->workspaceList->removeRow(n);
            for (auto child: _plot2d_children)
                if (child!=nullptr)
                    if (child->has_var(vqname))
                    {
                        child->remove_plot(vqname);
                    }

            for (auto child: _plot_qwt_children)
            {
                if (child==nullptr) continue;
                if (child->has_var(vqname))
                {
                    child->remove_var(vqname);
                }
            }
        }
        else
            n++;
    }

	if (varlist.empty()) return;
	for (auto &v : varlist)
        updatedSingleVar(v);

	for (const auto& child: _plot2d_children)
        if (child!=nullptr)
            child->update_workspace(_workspace);

	for (const auto& child: _plot_qwt_children)
	{
		if (child==nullptr) continue;
		child->update_data(varlist);
	}

}
// ---------------------------------------------------------------------------------
// Mesh callback
void    mdiOctaveInterface::delete_children(wndOctaveScript* child)
{
    _scripts_children.removeAll(child);
}
// ---------------------------------------------------------------------------------
void    mdiOctaveInterface::delete_children(wndPlot2d* child)
{
    if (_b_deleting) return;
    _plot2d_children.removeAll(child);

}
// ---------------------------------------------------------------------------------
void    mdiOctaveInterface::delete_children(dlgQWTPlot* child)
{
	if (_b_deleting) return;
	_plot_qwt_children.removeAll(child);

}
// ---------------------------------------------------------------------------------
void mdiOctaveInterface::saveWorkspaceData()
{
    QDateTime date = QDateTime::currentDateTime();
    QString formatted_date_tme = date.toString("yyyy_MM_dd_hh_mm_ss");

    QString projectFile = QFileDialog::getSaveFileName(this,"Workspace file",
                                                       ariasdk_projects_path.absolutePath()+QDir::separator()+QString("data_")+formatted_date_tme,
                                                       tr("Data workspace(*.mat);;All files (*.*)"),
                                                       nullptr,
                                                       QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));
    if (projectFile.isEmpty())
        return;

    _workspace->save_to_file(projectFile.toStdString());
}

// ---------------------------------------------------------------------------------
void mdiOctaveInterface::variableQwtPlot()
{
	dlgQWTPlot*  wnd2d = new dlgQWTPlot(this,this, _workspace, PTQWT_PLOT);

	connect(_interfaceData, &octaveInterface::workspaceUpdated, wnd2d, &dlgQWTPlot::update_workspace);
	connect(_interfaceData, &octaveInterface::updatedVariables, wnd2d, &dlgQWTPlot::update_data);

	ui->mdiArea->addSubWindow(wnd2d);
	wnd2d->showMaximized();
	_plot_qwt_children.append(wnd2d);

	QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
	QStringList	     vars;
	foreach (QModelIndex index, indexList)
	{
		int row = index.row();
		// Get var name
		QString varname = ui->workspaceList->item(row,1)->text();
		vars.append(varname);
		//wnd2d->add_plot(varname,"");
		//QString last_error = wnd2d->last_error();
		//if (!last_error.isEmpty())
		//{
		//	QMessageBox::critical(this,"Error creating plot",last_error);
			//return;
		//}
	}

	if (vars.size()==1)
	{
		wnd2d->assign_vars(vars[0]);
	}

	return;
}

void mdiOctaveInterface::variableQwtPlotAllInOne()
{
	dlgQWTPlot*  wnd2d = new dlgQWTPlot(this,this, _workspace, PTQWT_PLOT);

	connect(_interfaceData, &octaveInterface::workspaceUpdated, wnd2d, &dlgQWTPlot::update_workspace);
	connect(_interfaceData, &octaveInterface::updatedVariables, wnd2d, &dlgQWTPlot::update_data);

	ui->mdiArea->addSubWindow(wnd2d);
	wnd2d->showMaximized();
	_plot_qwt_children.append(wnd2d);

	QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
	QStringList	     vars;
	foreach (QModelIndex index, indexList)
	{
		int row = index.row();
		// Get var name
		QString varname = ui->workspaceList->item(row,1)->text();
		vars.append(varname);
		//wnd2d->add_plot(varname,"");
		//QString last_error = wnd2d->last_error();
		//if (!last_error.isEmpty())
		//{
		//	QMessageBox::critical(this,"Error creating plot",last_error);
		//return;
		//}
	}

	if (vars.size()==1)
	{
		wnd2d->assign_vars(vars[0]);
	}

	return;

}

void mdiOctaveInterface::variableQwtPlotXData()
{
	dlgQWTPlot*  wnd2d = new dlgQWTPlot(this, this,_workspace, PTQWT_PLOT);

	connect(_interfaceData, &octaveInterface::workspaceUpdated, wnd2d, &dlgQWTPlot::update_workspace);
	connect(_interfaceData, &octaveInterface::updatedVariables, wnd2d, &dlgQWTPlot::update_data);

	ui->mdiArea->addSubWindow(wnd2d);
	wnd2d->showMaximized();
	_plot_qwt_children.append(wnd2d);

	QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
	QStringList	     vars;
	foreach (QModelIndex index, indexList)
	{
		int row = index.row();
		// Get var name
		QString varname = ui->workspaceList->item(row,1)->text();
		vars.append(varname);
		//wnd2d->add_plot(varname,"");
		//QString last_error = wnd2d->last_error();
		//if (!last_error.isEmpty())
		//{
		//	QMessageBox::critical(this,"Error creating plot",last_error);
		//return;
		//}
	}

	if (vars.size()==1)
	{
		wnd2d->assign_vars(vars[0]);
	}

	return;
}

// ---------------------------------------------------------------------------------
// Scatterplot callback

void mdiOctaveInterface::variableQwtScatterPlot()
{
	wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
	ui->mdiArea->addSubWindow(wnd2d);
	wnd2d->showMaximized();

	_plot2d_children.append(wnd2d);

	QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();

	foreach (QModelIndex index, indexList)
	{
		int row = index.row();
		// Get var name
		QString varname = ui->workspaceList->item(row,1)->text();
		wnd2d->add_scatterplot(varname,"");
		QString last_error = wnd2d->last_error();
		if (!last_error.isEmpty())
		{
			QMessageBox::critical(this,"Error creating plot",last_error);
			//return;
		}
	}
	return;
}

void mdiOctaveInterface::variableQwtScatterPlotAllInOne()
{
	wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
	ui->mdiArea->addSubWindow(wnd2d);
	wnd2d->showMaximized();
	_plot2d_children.append(wnd2d);

	QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
	int plot = -1;
	foreach (QModelIndex index, indexList)
	{
		int row = index.row();
		// Get var name
		QString varname = ui->workspaceList->item(row,1)->text();
		wnd2d->add_scatterplot(varname,"", plot);
		QString last_error = wnd2d->last_error();
		if (!last_error.isEmpty())
		{
			QMessageBox::critical(this,"Error creating plot",last_error);
			//return;
		}
		plot = 0;
	}
	return;
}

void mdiOctaveInterface::variableQwtScatterPlotXData()
{
	QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
	if (indexList.count()<2)
		return;

	wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
	ui->mdiArea->addSubWindow(wnd2d);
	wnd2d->showMaximized();
	_plot2d_children.append(wnd2d);

	int plot = -1;
	QString xname="";
	foreach (QModelIndex index, indexList)
	{
		int row = index.row();
		// Get var name
		QString varname = ui->workspaceList->item(row,1)->text();

		if (plot==-1)
		{
			xname = varname;
			plot = 0;
			continue;
		}
		wnd2d->add_scatterplot(varname,xname, plot);
		QString last_error = wnd2d->last_error();
		if (!last_error.isEmpty())
		{
			QMessageBox::critical(this,"Error creating plot",last_error);
			//return;
		}
	}
	return;
}
// ---------------------------------------------------------------------------------
// 7. Qwt Density
void mdiOctaveInterface::variableQwtDensityPlot()
{
	dlgQWTPlot*  wnd2d = new dlgQWTPlot(this, this,_workspace, PTQWT_DENSITY);

	connect(_interfaceData, &octaveInterface::workspaceUpdated, wnd2d, &dlgQWTPlot::update_workspace);
	connect(_interfaceData, &octaveInterface::updatedVariables, wnd2d, &dlgQWTPlot::update_data);

	ui->mdiArea->addSubWindow(wnd2d);
	wnd2d->showMaximized();
	_plot_qwt_children.append(wnd2d);

	QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
	QStringList	     vars;
	foreach (QModelIndex index, indexList)
	{
		int row = index.row();
		// Get var name
		QString varname = ui->workspaceList->item(row,1)->text();
		vars.append(varname);
	}

	if (vars.size()==1)
	{
		wnd2d->assign_vars(vars[0],"","");
	}

	return;
}


void mdiOctaveInterface::variableQwtDensityPlotXData()
{
	wndPlot2d*  wnd2d = new wndPlot2d(this, _workspace);
	ui->mdiArea->addSubWindow(wnd2d);
	wnd2d->showMaximized();
	_plot2d_children.append(wnd2d);

	QModelIndexList indexList = ui->workspaceList->selectionModel()->selectedRows();
	if (indexList.count()%3!=0)
	{
		QMessageBox::critical(this, "Error", "Multiple of 3 selection is needed");
		return;
	}


	for (int n=0; n < indexList.count(); n+=3)
	{

		// Get var and indep var names
		QString namex   = ui->workspaceList->item(indexList.at(n).row(),1)->text();
		QString namey   = ui->workspaceList->item(indexList.at(n+1).row(),1)->text();

		QString varname = ui->workspaceList->item(indexList.at(n+2).row(),1)->text();
		wnd2d->add_density_plot(varname,namex,namey);

		QString last_error = wnd2d->last_error();
		if (!last_error.isEmpty())
		{
			QMessageBox::critical(this,"Error creating plot",last_error);
			//return;
		}
	}
	return;
}

// Attempt to close all scripts
bool		mdiOctaveInterface::close_scripts()
{
	for (auto &child: ui->mdiArea->subWindowList())
	{
		wndOctaveScript* wnd = qobject_cast<wndOctaveScript*>(child->widget());
		if (wnd==nullptr) continue;

		wnd->close();
		if (wnd->isClosed())
		{
			delete wnd->parent();
		}
		else
			return false;

	}
	return true;
}


void       mdiOctaveInterface::updateFont()
{
    for (auto &child: ui->mdiArea->subWindowList())
    {
        wndOctaveScript* wnd = qobject_cast<wndOctaveScript*>(child->widget());
        if (wnd==nullptr) continue;
        wnd->updateFont();
    }
}


void mdiOctaveInterface::updateScriptsProject(radarProject* proj)
{
    _project = proj;
    for (auto &child: ui->mdiArea->subWindowList())
    {
        wndOctaveScript* wnd = qobject_cast<wndOctaveScript*>(child->widget());
        if (wnd==nullptr) continue;
        wnd->updateProject(proj);
    }
}
