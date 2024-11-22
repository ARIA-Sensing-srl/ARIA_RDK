/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef OCTAVEINTERFACE_H
#define OCTAVEINTERFACE_H

#include <QObject>
#include <QSharedDataPointer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <octave.h>
#include <ovl.h>
#include <octavews.h>

#undef OCTAVE_THREAD

class octaveInterface : public QObject
{
    Q_OBJECT
private:
    QStringList             commandList;
    QString                 commandCurrent;
    QString                 last_output;
    bool                    bStopThread;
    QMutex                  sync;
    octave::interpreter     *_octave_engine; // This is the MAIN _octave_engine (only one _octave_engine)
    octavews                *_workspace;
#ifndef OCTAVE_THREAD
    bool                    _b_running_command; // true if we are running a command from the command line
    bool                    _b_running_script;
    int                     _stopped_at;
#endif
    //------------------------------------------------------------------------
    std::shared_ptr<class octaveScript> _current_script;
public:
    octaveInterface();
    ~octaveInterface();
    // Commands from in-line command editor
    bool                    appendCommand(const QString &strCommand);
    const QString &         getCurrentCommand();
    bool                    retrieveCommand(QString &commandToExecute);
    void                    completeCommand();
    void                    clearCommandList();
    const QString &         get_last_output() {return last_output;}
    // Command execution status
    bool                    isReady();
    void                    stopThread(bool bStop=true);
    // Main octave engine
    inline octave::interpreter*    get_octave_engine() {return _octave_engine;}
    void                    run(std::shared_ptr<class octaveScript> script);
    void                    run(const QString &program);
 /*--------------------------------------------
  * Workspace commands
  * ------------------------------------------*/
    void            clearWorkspace();
    bool            appendVariable();
    QString         appendVariable(QString name, const octave_value& val, bool internal, bool toOctave=false, QStringList indep=QStringList(), QStringList dep=QStringList());

    void            refreshWorkspace();
    octave_value_list execute_feval(QString command, octave_value_list& in, int n_outputs);
    int             findFunc(QString funcName,bool wait);

    void              append_variable(QString name, const octave_value& val, bool internal= false);
    void              refresh_workspace();
    octavews*         get_workspace() {return _workspace;}
    void              update_interpreter_internal_vars();
    void              execute_feval(QString command, const string_vector& input, const string_vector& output); // NB we may have empty in some output var
    bool              save_workspace_to_file(QString filename);

signals:
    void            workerError(QString error);
    void            workspaceUpdated();
    void            workspaceDeleted();
    void            workspaceCleared();
    void            workspaceVarAdded();
    void            workspaceVarModified();
    void            workspaceVarDeleted();
#ifndef OCTAVE_THREAD
    void            commandCompleted(QString command, int errorcode);
#endif
signals:
    void            updatedVariable(const std::string& var);
    void            updatedVariables(const std::set<std::string>& vars);
};


#ifdef OCTAVE_THREAD
class qDataThread : public QObject
{
private:

    QMutex          sync;
    QWaitCondition  pauseCond;
    bool            bPause;
    octaveInterface    *data;
    Q_OBJECT
public:

public slots:
    void processDataThread();
    void finishedDataThread();
signals:
    void error(QString err);
    void finished();
    void workspaceUpdated();
    void commandCompleted(QString command, int errorcode);
    void commandStarting(QString command);
    void fifoEmpty();

public:
    explicit qDataThread(QObject * parent = 0);
    ~qDataThread();

    octaveInterface* getData() {return data;}

    void resume();
    void pause();
    bool isPaused() {return bPause;}
    bool isReady()  {return data==NULL?true:data->isReady();}

    void runFile(QString fileName, const octave_value_list& args = octave_value_list (),
                 int nargout = 0);


    int findVar(const char* varname);
    int findFunc(QString funcName,bool wait=false);
};
#endif
#endif // OCTAVEINTERFACE_H
