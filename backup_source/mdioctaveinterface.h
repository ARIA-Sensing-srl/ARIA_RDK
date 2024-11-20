/*
 * ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 */
#ifndef MDIOCTAVEINTERFACE_H
#define MDIOCTAVEINTERFACE_H

#include <QDialog>
#include <QThread>
#include <QIcon>

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



signals:
    void plotVariable(QString varname);
public slots:
    void newScript();
    void openScript();
    void saveScript();
    void saveSctiptAs();
    void closeScript();

    void error(QString errorString);
    // Bottom editor
    void newCommandLine();
    void closeEvent( QCloseEvent* event );
    void octaveCompletedTask(QString task, int errorcode);
    void updateVarTable();

    void workspaceTableRightClick(QPoint pos);
// Context menu
    void variablePlot();
    void variableImage();

private:
    Ui::mdiOctaveInterface *ui;
    class qDataThread           *_elabThread;
    class qDataEngine           *_interfaceData;
    class dataWorkspace         *_dataws;

    QVector<class wndOctaveScript*>     _scripts_children;
    QVector<class wndPlot2d*>           _plot2d_children;

};

#endif // MDIOCTAVEINTERFACE_H
