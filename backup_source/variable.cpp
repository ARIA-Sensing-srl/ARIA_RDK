/*
 * ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 */

#include "variable.h"
#include "dataworkspace.h"

#include <octave.h>
#include <interpreter.h>
#include "radarparameter.h"
#include "qdebug.h"
size_t variable::varCount = 0;
//---------------------------------------------
// Initialize generic var
//---------------------------------------------
variable::variable(QString varName, dataWorkspace* ws) : variable()
{
    independentVars = QStringList();
    dependentVars   = QStringList();
    dataws = ws;
    if (dataws!=NULL)
    {
        // Check if variable exists. If so, change name
        variable* vcheck;
        do{
            vcheck = dataws->getVarByName(varName);
            if (vcheck!=nullptr)
                varName+="_1";
        }while(vcheck!=nullptr);
    }

    name = varName;

    if (dataws!=nullptr)
        dataws->addVariable(this);
}

//---------------------------------------------
// Initialize var with the independent vars
//---------------------------------------------
variable::variable(QString varName, const octave_value& matValue, dataWorkspace* ws,  QStringList indep) : variable(varName,ws)
{
    if (ws==nullptr)
        return;

    value = matValue;

    independentVars = indep;

    for (QStringList::iterator var = indep.begin(); var != indep.end(); var++)
    {
        variable* varp= ws->getVarByName((*var));
        if (varp!=nullptr)
            varp->dependentVars.append(name);
    }
}

//---------------------------------------------
// Delete var and update workspace if necessary
//---------------------------------------------
variable::~variable()
{
    if (!isWSDeleting)
        if (dataws!=nullptr)
            dataws->removeVariable(this);
}

//---------------------------------------------
// Break all dependencies
//---------------------------------------------
void variable::breakDependencies()
{
    if (dataws==nullptr)
    {
            dependentVars.clear();
            independentVars.clear();
            return;
    };

    // Get depending variables and remove *this
    for (QStringList::iterator depending = dependentVars.begin(); depending != dependentVars.end(); depending++)
    {
        variable* dep = dataws->getVarByName(*depending);

        if ((dep)!=nullptr)
            (dep)->removeIndependentVar(name);
    }

    // Get independent var and remove this
    for (QStringList::iterator indep = independentVars.begin(); indep != independentVars.end(); indep++)
    {
        variable* var = dataws->getVarByName(*indep);

        if ((var)!=nullptr)
            (var)->removeDependentVar(this);
    }

    independentVars.clear();
    dependentVars.clear();
}
//---------------------------------------------
// Clear prev independent var list and set
// new ones
//---------------------------------------------
void variable::setIndependentVar(QStringList indep)
{
    for (int n=0; n < independentVars.count(); n++)
        this->removeIndependentVar(independentVars.at(n));

    independentVars = indep;

    if (dataws!=nullptr)
        return;

    for (int n=0; n < independentVars.count(); n++)
    {
        variable *vdep = dataws->getVarByName(independentVars.at(n));
        if (vdep==nullptr) continue;
        vdep->dependentVars.append(name);
    }
}
//---------------------------------------------
// Clear prev dependent var list and set new
// ones
//---------------------------------------------
void variable::setDependentVar(QStringList dep)
{
    for (int n=0; n < dependentVars.count(); n++)
        this->removeDependentVar(dependentVars.at(n));

    dependentVars = dep;

    if (dataws!=nullptr)
        return;

    for (int n=0; n < dependentVars.count(); n++)
    {
        variable *vdep = dataws->getVarByName(dependentVars.at(n));
        if (vdep==nullptr) continue;
        vdep->independentVars.append(name);
    }
}

//---------------------------------------------
// Remove "var" from the list of independent vars
//---------------------------------------------
void  variable::removeIndependentVar(variable* var)
{
    if (var==nullptr) return;
    independentVars.removeAll(var->getName());
    var->dependentVars.removeAll(name);


    if (dataws==nullptr) return;

    variable *varp = dataws->getVarByName(name);
    if (varp!=nullptr)
        varp->dependentVars.removeAll(name);
}

//---------------------------------------------
// Remove variable identified by "varName" from the list of independent vars
//---------------------------------------------
void  variable::removeIndependentVar(const QString& varName)
{

    independentVars.removeAll(varName);

    if (dataws==nullptr) return;

    variable* var = dataws->getVarByName(varName);
    if (var!=nullptr)
        var->removeDependentVar(name);
}

//---------------------------------------------
// Remove "var" from the list of dependent vars
//---------------------------------------------
void variable::removeDependentVar(variable* var)
{
    if (var==nullptr) return;
    dependentVars.removeAll(var->getName());

   var->independentVars.removeAll(name);

   if (dataws==nullptr) return;

   variable *varp = dataws->getVarByName(name);
   if (varp!=nullptr)
        varp->independentVars.removeAll(name);
}


//---------------------------------------------
// Remove variable identified by "varName" from the list of dependent vars
//---------------------------------------------
void variable::removeDependentVar(const QString& varName)
{
    // Go thru list AND workspace
    dependentVars.removeAll(varName);
    if (dataws==nullptr)
        return;

    variable* var = dataws->getVarByName(varName);
    if (var!=nullptr)
        var->independentVars.removeAll(name);
}

//---------------------------------------------
// Mark the variable as GUI Internal or Octave
// internal
//---------------------------------------------
void variable::setInternal(bool bInternalOrOctave)
{
    bInternalOctave = bInternalOrOctave;
}
//---------------------------------------------
// Return the status of the Internal variable
//---------------------------------------------
bool variable::isInternal(void)
{
    return bInternalOctave;
}

//---------------------------------------------
// Eval the tree
//---------------------------------------------
void variable::eval()                     // Evaluate function
{
    if (dataws==nullptr)
        return;
    if (!bUpdated)
    {
        // First of all, evaluate all independent functions
        for (QStringList::Iterator indepvar = independentVars.begin(); indepvar != independentVars.end(); indepvar++)
        {
            variable* var = dataws->getVarByName(*indepvar);

            if (var!=nullptr)
                (var)->eval();
        }
        // Run the evaluation script (this belongs to the dataws)
        if (!evalScript.isEmpty()&&(dataws!=nullptr))
        {
            // Something similar dataws->evalfunction blabla
        }
        bUpdated = true;
        dataws->signalVariableChanged(this);
    }
    // Now update also depending vars?
    for (QStringList::Iterator depvar = dependentVars.begin(); depvar!= dependentVars.end(); depvar++)
    {
        variable* var = dataws->getVarByName(*depvar);

        if (var!=nullptr)
            (var)->eval();
    }

    var_changed();
}
//---------------------------------------------
// Eval the tree
//---------------------------------------------
void  variable::setEvalScript()            // Set eval script (only if *this is a calculated function from other variables)
{

}

//---------------------------------------------
// Eval the tree
//---------------------------------------------
void  variable::updateValue(const octave_value &newval)
{
    setToUpdate();

    value = newval;
    bUpdated = true;

    if (dataws==nullptr)
        return;

    dataws->signalVariableChanged(this);
    // Now update also depending vars?
    for (QStringList::Iterator depvar = dependentVars.begin(); depvar!= dependentVars.end(); depvar++)
    {
        variable* var = dataws->getVarByName(*depvar);

        if (var!=nullptr)
            (var)->eval();
    }
    var_changed();
}
//-----------------------------------------------
// Set this variable as "not updated yet
// Propagates the flag to depending variables
void  variable::setToUpdate()        // Signals that this variable needs update
{
    bUpdated = false;
    if (dataws==nullptr)
        return;

    for (QStringList::Iterator depvar = dependentVars.begin(); depvar!= dependentVars.end(); depvar++)
    {
        variable* var = dataws->getVarByName(*depvar);

        if (var!=nullptr)
            (var)->setToUpdate();
    }
}
//---------------------------------------------
// DB management
//---------------------------------------------
variable*       variable::getIndepAt(int i)
{
    if (dataws==nullptr) return nullptr;
    return (i<independentVars.count())?dataws->getVarByName(independentVars.at(i)):nullptr;

}


variable*       variable::getDependentAt(int i)
{

    if (dataws==nullptr) return nullptr;
    return (i<dependentVars.count())?dataws->getVarByName(dependentVars.at(i)):nullptr;
}


int variable::getIndepCount()
{
    return independentVars.count();
}


int variable::getDependentCount()
{
    return dependentVars.count();
}

//---------------------------------------------
// datatype management
//---------------------------------------------
bool variable::isNumber()
{
    return value.isreal() || value.iscomplex();
}

bool variable::isReal()
{
    return value.isreal();
}

bool variable::isComplex()
{
    return value.iscomplex();
}

//---------------------------------------------
// Get variable's dimensions
//---------------------------------------------
QVector<size_t> variable::getSize()
{
    QVector<size_t> out;
    dim_vector size = value.dims();
    int    ndims = size.ndims();
    for (int rc = 0; rc < ndims; rc++)
        out.append(size(rc));

    return out;
}

//---------------------------------------------
// Link to radar parameters
//---------------------------------------------
void    variable::var_changed()
{
    for (size_t n=0; n < _param_ptr.size(); n++)
        if (_param_ptr[n]!=nullptr)
            _param_ptr[n]->var_changed();
}

int     variable::param_index(std::shared_ptr<radarParamBase> param)
{
    int res = -1;
    for (size_t n=0; n < _param_ptr.size(); n++)
        if (_param_ptr[n]==param)
        { res = (int)n; break;}
    return res;
}

void    variable::link_to_param(std::shared_ptr<radarParamBase> param)
{
    if (param_index(param)>=0)
        return;

    _param_ptr.insert(_param_ptr.end(), param);
}
void    variable::remove_link_to_param(std::shared_ptr<radarParamBase> param)
{
    int npar = param_index(param);
    if (npar < 0) return;
    _param_ptr.erase(_param_ptr.begin()+npar);

}

