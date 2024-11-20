/*
 * ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 */

#include "dataworkspace.h"
#include <octave.h>
#include <oct.h>
#include <parse.h>
#include <ov.h>
#include <ovl.h>


dataWorkspace::dataWorkspace(octave::interpreter* octave) : varMap(), parent(nullptr), children(), octaveInterpreter(octave)
{
}


dataWorkspace::~dataWorkspace()
{
    cleanAllVariables();
}



bool      dataWorkspace::addVariable(variable* var)
{
    //sync.lock();
    if (var==nullptr)
        return false;

    if (var->getName().isEmpty())
    {
        //sync.unlock();
        return false;
    }

    QMap<QString,variable*>::iterator itVar = varMap.find(var->getName());
    if (itVar!=varMap.end())
    {
        // Remove variable
        removeVariable((itVar).value());
    }

    varMap[var->getName()] = var;
    //sync.unlock();
    return true;
}

QMap<QString,variable*>::Iterator      dataWorkspace::removeVariable(variable* var)
{
    if (var==nullptr)
        return varMap.end();
    //sync.lock();
    QString name(var->getName());
    var->breakDependencies();
    emit variableDeleted(var);
    var->setWsDeleting();
    delete var;
    return varMap.erase(varMap.find(name));

    //sync.unlock();
}

variable* dataWorkspace::getVarByName(QString name)
{
    QMap<QString,variable*>::Iterator iter = varMap.find(name);
    return iter==varMap.end() ? nullptr : iter.value();
}


size_t dataWorkspace::getVarCount()
{
    return varMap.count();
}


size_t dataWorkspace::getOctaveVarCount()
{
    //sync.lock();
    size_t count=0;
    for (QMap<QString,variable*>::iterator itVar = varMap.begin(); itVar != varMap.end(); itVar++)
    {
        variable* var = itVar.value();
        if ((var!=nullptr)&&(!var->isInternal()))
            count++;
    }
    //sync.unlock();
    return count;
}


QString dataWorkspace::getVarName(size_t varnum)
{
    //sync.lock();
    variable* var = varMap.values().at(varnum);
    if (var==nullptr)
        return QString("");

    //sync.unlock();
    return var->getName();
}


QStringList dataWorkspace::getVarList()
{
    return varMap.keys();
}

QStringList dataWorkspace::getOctaveVarList()
{
    //sync.lock();
    QStringList octVars;
    for (QMap<QString,variable*>::iterator itVar = varMap.begin(); itVar != varMap.end(); itVar++)
    {
        variable* var = itVar.value();
        if ((var!=nullptr)&&(!var->isInternal()))
            octVars.append(var->getName());
    }
    //sync.unlock();
    return octVars;
}

void  dataWorkspace::cleanAllVariables()
{
    //sync.lock();
    // Clear all variables
    for (QMap<QString,variable*>::iterator itVar = varMap.begin(); itVar != varMap.end(); itVar++)
        if ((itVar.value())!=nullptr)
        {
            variable *ptr = itVar.value();
            itVar.value()=nullptr;
            ptr->setWsDeleting();
            delete (ptr);
        }

    //sync.unlock();
}

void  dataWorkspace::cleanOctaveVariables()
{
    //sync.lock();
    // Clear all variables
    for (QMap<QString,variable*>::iterator itVar = varMap.begin(); itVar != varMap.end(); itVar++)
        if (((*itVar)!=nullptr)&&((*itVar)->isInternal()))
            delete (*itVar);
    //sync.unlock();
}

void  dataWorkspace::updateFromOctave()
{
    //sync.lock();
    if (octaveInterpreter==nullptr)
        return;

    std::list<std::string> octds = octaveInterpreter->variable_names();


    // Remove all ws Octave variables that are not inside current interpreter database
    for (QMap<QString, variable*>::Iterator varit = varMap.begin(); varit != varMap.end(); )
    {
        if (varit.value()==nullptr)
        {
            varit++;
            continue;
        }

        std::string valname = varit.value()->getName().toStdString();
        std::list<std::string>::iterator varOctave = std::find(octds.begin(), octds.end(), valname);

        if (varOctave == octds.end())
        {
            if (!varit.value()->isInternal())
                // We have an octave variable not found. Erase it from the dataset
                varit = removeVariable(varit.value());

            if (varMap.empty()) break;
        }
        else
            varit++;
    }

    // For all remaining variable, either create a new one or update the internal stored value
    for (std::list<std::string>::iterator varit = octds.begin(); varit!=octds.end(); varit++)
    {
        octave_value octvalue = octaveInterpreter->varval(*varit);
        QString varName = QString::fromStdString(*varit);
        QMap<QString, variable*>::Iterator var = varMap.find(varName);
        if (var!=varMap.end())
        {
            // We have a variable which is not inside Octave
            if (!var.value()->isInternal())
                var.value()->updateValue(octvalue);

        }
        else
        {
            variable* newVar = new variable(varName,this);
            newVar->setInternal(false);
            newVar->updateValue(octvalue);
        }
    }
    //sync.unlock();
}


QList<variable*> dataWorkspace::getVars()
{
    QList<variable*> out = varMap.values();
    return out;
}


void  dataWorkspace::updateInterpreterWithInternalVars()
{
    if (octaveInterpreter==nullptr) return;

    // Remove all ws Octave variables that are not inside current interpreter database
    for (QMap<QString, variable*>::Iterator varit = varMap.begin(); varit != varMap.end(); varit++)
        if (varit.value()->isInternal())
        {
            std::string varname = varit.value()->getName().toStdString();
            // is it already in the dataset?
            if (octaveInterpreter->is_variable(varname))
                octaveInterpreter->assign(varname, varit.value()->getOctValue());
            else
                octaveInterpreter->install_variable(varname, varit.value()->getOctValue(), false);

        }
}


void dataWorkspace::signalVariableChanged(variable* var)
{
    emit variableChanged(var);
}

