/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef MDIOCTAVEINTERFACE_H
#define MDIOCTAVEINTERFACE_H

#include <QDialog>
#include <QThread>
#include <QIcon>
#include <QTextEdit>
#include <set>
#include <string>
#include <ov.h>
#include <radarparameter.h>
#include <octavescript.h>
namespace Ui {
class mdiOctaveInterface;
}



class mdiOctaveInterface : public QDialog
{
    Q_OBJECT
public:
    explicit mdiOctaveInterface(class qDataThread* worker=nullptr,  QWidget *parent = nullptr);
    ~mdiOctaveInterface();
    QIcon       iconInternal;
    QIcon       iconOctave;

    void        delete_children(class wndOctaveScript* child);
    void        delete_children(class wndPlot2d* child);
    QTextEdit*  get_textoutput();
    void        update_octave_interface();
    void        add_variable_row(int row, const std::string& name, const octave_value& val, bool internal);
signals:
    void plotVariable(QString varname);
public slots:
    void newScript();
    void openScript();
    void saveScript();
    void saveSctiptAs();
    void closeScript();
    void openProjectScript(octaveScript* script);

    void error(QString errorString);
    // Bottom editor
    void newCommandLine();
    void closeEvent( QCloseEvent* event );
    void octaveCompletedTask(QString task, int errorcode);
    void updateVarTable();

    void workspaceTableRightClick(QPoint pos);

    void saveWorkspaceData();
// Context menu
    // 1. Plot
    void variablePlot();
    void variablePlotAllInOne();
    void variablePlotXData();
    // 2. Scatter
    void variableScatterPlot();
    void variableScatterPlotAllInOne();
    void variableScatterPlotXData();
    // 3. BoxPlot
    void variableBoxPlot();
    void variableBoxPlotAllInOne();
    void variableBoxPlotXData();
    // 4. Area
    void variableAreaPlot();
    void variableAreaPlotAllInOne();
    void variableAreaPlotXData();
    // 5. Bar
    void variableBarPlot();
    void variableBarPlotAllInOne();
    void variableBarPlotXData();
    // 5. Arrow
    void variableArrowPlot();
    void variableArrowPlotAllInOne();
    void variableArrowPlotXData();
    // 7. Density
    void variableDensityPlot();
    void variableDensityPlotXData();
    // 8. Vector plot
    void variableVectorPlot();
    void variableVectorPlotXData();

    // Updated var(s)
    void updatedSingleVar(const std::string& varname);
    void updatedVars(const std::set<std::string>& varlist);

private:
    Ui::mdiOctaveInterface *ui;
    class qDataThread           *_elabThread;
    class octaveInterface       *_interfaceData;
    class octavews              *_workspace;
    class dataWorkspace         *_dataws;

    QVector<class wndOctaveScript*>     _scripts_children;
    QVector<class wndPlot2d*>           _plot2d_children;
    bool                                _b_deleting;
};

#endif // MDIOCTAVEINTERFACE_H
