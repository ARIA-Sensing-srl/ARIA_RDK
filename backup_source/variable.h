/*
 * ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 */
#ifndef VARIABLE_H
#define VARIABLE_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QObject>

#include <octave.h>
#include <ov.h>
#include <ovl.h>
#include <mxarray.h>


using namespace octave;
class dataWorkspace;
class radarParamBase;

struct dataWithName
{
    octave_value          value;                // Move to Oct files
    std::string           name;
};

class variable : public QObject , public std::enable_shared_from_this<variable>
{
    Q_OBJECT
private:
    static  size_t        varCount;
    dataWorkspace         *dataws;
    QStringList           independentVars;      // List of variables this var depends on
    QStringList           dependentVars;        // List of variables depending on this
    octave_value          value;
    QString               name;
    QStringList           evalScript;           // Octave evaluation script (if any)
    bool                  bUpdated;             // Avoid any dupicate evaluation
    bool                  bInternalOctave;
    bool                  isWSDeleting;         // True iff this is going to be deleted by the dataws and not from other processes
    std::vector<std::shared_ptr<radarParamBase>> _param_ptr; // Link to radar parameter
private:
    void                  setToUpdate();        // Signals that this variable needs update
    void                  setWsDeleting(bool iswsdeleting=true) {isWSDeleting = iswsdeleting;}
    friend class dataWorkspace;
    friend class radarParamBase;
    void            var_changed();
    void            link_to_param(std::shared_ptr<radarParamBase> param);
    void            remove_link_to_param(std::shared_ptr<radarParamBase> param);
    int             param_index(std::shared_ptr<radarParamBase> param);
public:
    //----------------------------------------
    // Basic data is Octave
    variable() :
        dataws(NULL),
        independentVars(),
        dependentVars(),
        value(),
        name(QString("noname")+QString::number(varCount)),
        evalScript(),
        bInternalOctave(true),
        isWSDeleting(false),
        _param_ptr(),
        customEvalFunction(nullptr)
        {varCount = varCount+1;}

    variable(QString varName, dataWorkspace* ws);
    variable(QString varName, const octave_value& matValue, dataWorkspace* ws,  QStringList indep=QStringList());
    variable(variable& v2);
    ~variable();
    void setInternal(bool bInternalOrOctave);
    bool isInternal(void);
    //octave_value    getVal();
    QVector<size_t> getSize();
    QString         getName() {return name;}
    void            breakDependencies();
    void            setIndependentVar(QStringList indep);
    void            setDependentVar(QStringList dep);
    void            removeIndependentVar(variable* var);
    void            removeIndependentVar(const QString& varName);
    void            removeDependentVar(variable* var);
    void            removeDependentVar(const QString& varName);
    QStringList     getIndependentVars() {return independentVars;}
    variable*       getIndepAt(int i);
    variable*       getDependentAt(int i);
    int             getIndepCount();
    int             getDependentCount();
    std::shared_ptr<variable>   get_std_ptr() {return shared_from_this();}

    void            eval();                             // Evaluate function
    void        (variable::*customEvalFunction)(const octave_value_list& in);      // Pointer to internal function (is it needed? by now we declare it and maybe we'll use it=
    void            setEvalScript();                    // Set eval script (only if *this is a calculated function from other variables)

    void            updateValue(const octave_value &newval);
    const octave_value&
                    getOctValue() {return value;}
    mxArray*        getMxValue()  {return value.as_mxArray();}
    QString         getClassName(){return QString::fromStdString(value.class_name());}

    dataWorkspace*  getWS(){return dataws;}

    bool            isNumber();
    bool            isReal();
    bool            isComplex();





};



#endif // VARIABLE_H
