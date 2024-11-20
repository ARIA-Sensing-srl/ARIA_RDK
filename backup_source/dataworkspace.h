/*
 * ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 */
#ifndef DATAWORKSPACE_H
#define DATAWORKSPACE_H
#include <QObject>
#include <QMutex>
#include <QVector>
#include <octave.h>
#include <QMap>
#include "variable.h"
/* ---------------------------------------
 * Main Workspace class
 * ---------------------------------------*/
using namespace octave;

namespace octave
{
    class interpreter;
}

class qOctaveWorkerData;

class dataWorkspace : public QObject
{
    Q_OBJECT
private:
    QMutex                    sync;
    QMap<QString,variable*>   varMap;   // This structure contains the definition of all variables
    dataWorkspace            *parent;
    QVector<dataWorkspace*>   children;
    octave::interpreter*      octaveInterpreter;

public:
    dataWorkspace(octave::interpreter* octave=nullptr);
    ~dataWorkspace();

    bool      addVariable(variable*);
    QMap<QString,variable*>::Iterator      removeVariable(variable*);
    void                                   updateInterpreterWithInternalVars();

    variable*      getVarByName(QString name);

    size_t         getVarCount();
    size_t         getOctaveVarCount();
    QString        getVarName(size_t varnum);
    QStringList    getVarList();
    QStringList    getOctaveVarList();
    QList<variable*> getVars();

    void  cleanAllVariables();
    void  cleanOctaveVariables();
    void  updateFromOctave();

    octave::interpreter* getOctaveInterpreter() {return octaveInterpreter;}

    void signalVariableChanged(variable* var);

signals:
    void variableChanged(variable* var);
    void variableDeleted(variable* var);

};




#endif // DATAWORKSPACE_H
