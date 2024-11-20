/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef RADARPARAMETER_H
#define RADARPARAMETER_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <stdint.h>
#include <QtXml>

#include <octavews.h>

enum RADARPARAMTYPE{RPT_UNDEFINED, RPT_FLOAT, RPT_INT8, RPT_UINT8, RPT_INT16, RPT_UINT16, RPT_INT32, RPT_UINT32, RPT_ENUM, RPT_STRING, RPT_VOID};
enum RADARPARAMSIZE{RPT_SCALAR, RPT_NDARRAY};
enum RADARPARAMIOTYPE{RPT_IO_INPUT, RPT_IO_OUTPUT, RPT_IO_IO};

#include "plotdescriptor.h"


/*
 * the value of a parameter can be one of the obvious one (double, xINT_nn) or
 * uint16_t  --> enum id
 * QVector<baseType> (to read vectors)
 * _availables can be
 * QMap<QString id, uint_8) (enum)
 * QVector<..>
 * */
typedef QPair<QString,uint8_t>          enumElem;
typedef QVector<enumElem>               radarEnum;
typedef QVector<enumElem>::Iterator     radarEnumIterator;


QString typeIdToString(RADARPARAMTYPE rpt);

class radarParamBase : public std::enable_shared_from_this<radarParamBase>
{
private:
         virtual    void update_variable() = 0;
protected:
    RADARPARAMTYPE      _rpt;
    RADARPARAMIOTYPE    _rpt_io;
    RADARPARAMSIZE      _rpt_size;
    QString             _paramname;
    QString             _group;
    bool                _hasMinMax;
    bool                _isValid;
    bool                _has_inquiry_value;
    QString             _command_string;
    QString             _var;
    bool                _is_grouped;
    int                  _group_order;
    QByteArray           _pure_command;
    octavews*            _workspace;
    bool                 _b_compound_name;
    bool                _b_plotted;
    PLOT_TYPE   _plottype;
public:
    radarParamBase() :
        _rpt(RPT_UNDEFINED),
        _rpt_io(RPT_IO_INPUT),
        _paramname("undefined"),
        _group("ungrouped"),
        _hasMinMax(false),
        _isValid(true),
        _has_inquiry_value(false),
        _command_string(""),
        _var(""),
        _is_grouped(false),
        _group_order(-1),
        _pure_command(""),
        _workspace(nullptr),
        _b_compound_name(false),
        _b_plotted(false),
        _plottype(PTJK_PLOT)
    {

    }

    radarParamBase(QString pname, RADARPARAMTYPE rpt= RPT_UNDEFINED, RADARPARAMIOTYPE rptio = RPT_IO_INPUT) :
        _rpt(rpt),
        _rpt_io(rptio),
        _paramname(pname),
        _group("ungrouped"),
        _hasMinMax(false),
        _isValid(true),
        _has_inquiry_value(false),
        _command_string(""),
        _var(""),
        _is_grouped(false),
        _group_order(-1),
        _pure_command(""),
        _workspace(nullptr),
        _b_compound_name(false)
    {

    }

    radarParamBase(const radarParamBase& v2) :
        _rpt(v2._rpt),
        _rpt_io(v2._rpt_io),
        _rpt_size(v2._rpt_size),
        _paramname(v2._paramname),
        _group(v2._group),
        _hasMinMax(v2._hasMinMax),
        _isValid(v2._isValid),
        _has_inquiry_value(v2._has_inquiry_value),
        _command_string(v2._command_string),
        _var(""),
        _is_grouped(v2._is_grouped),
        _group_order(v2._group_order),
        _pure_command(v2._pure_command),
        _workspace(v2._workspace),
        _b_compound_name(v2._b_compound_name)

    {}

    radarParamBase(radarParamBase& v2) :
        _rpt(v2._rpt),
        _rpt_io(v2._rpt_io),
        _paramname(v2._paramname),
        _group(v2._group),
        _hasMinMax(v2._hasMinMax),
        _isValid(v2._isValid),
        _has_inquiry_value(v2._has_inquiry_value),
        _command_string(v2._command_string),
        _var(""),
        _is_grouped(v2._is_grouped),
        _group_order(v2._group_order),
        _pure_command(v2._pure_command),
        _workspace(v2._workspace),
        _b_compound_name(v2._b_compound_name)

    {}

    ~radarParamBase() {};

    radarParamBase& operator = (radarParamBase& v2);
    radarParamBase& operator = (const radarParamBase& v2);

    RADARPARAMTYPE          get_type()       {return _rpt;}
    QString                 get_name()       {return _paramname;}
    void                    set_name(QString pname)       {_paramname = pname;}
    bool                    has_min_max()    {return _hasMinMax;}
    virtual bool            has_available_set()  {return false;}
    bool                    is_last_assignment_valid() {return _isValid;}
    void                    set_min_max(int a, int b){};

    void                    set_size(RADARPARAMSIZE size) {_rpt_size = size;}
    RADARPARAMSIZE          get_size() {return _rpt_size;}

    virtual bool            is_valid(QVariant v){return false;}
    virtual std::shared_ptr<radarParamBase> clone() const = 0;
     virtual QVector<QVariant>       value_to_variant() {return QVector<QVariant>();};
     virtual QVector<QVariant>       availableset_to_variant() {return QVector<QVariant>();};

     virtual void variant_to_value(QVector<QVariant>& data) = 0;
     virtual void variant_to_availabeset(QVector<QVariant>& data) = 0;
     virtual void variant_to_value(const QVector<QVariant>& data) = 0;
     virtual void variant_to_availabeset(const QVector<QVariant>& data) = 0;
     virtual void variant_to_inquiry_value(const QVariant& data) = 0;
     virtual QVariant inquiry_value_to_variant() = 0;
     virtual QByteArray          get_inquiry_value()  {return QByteArray();}

     virtual bool is_scalar() {return true;}
     virtual void set_min_max(QVariant min, QVariant max) {};
     virtual int  get_index_of_value(QVariant value) {return -1;}
     virtual QString get_min_string() {return "unknown";}
     virtual QString get_max_string() {return "unknown";}
     bool set_min_string(QString strval) {return false;}
     bool set_max_string(QString strval) {return false;}
     bool validate_string(QString strval){return false;}
     void                remove_min_max() {_hasMinMax = false;}
     void                remove_available_set() {}

     virtual int                get_expected_payload_size() {return -1;}


     void               set_io_type(RADARPARAMIOTYPE io)    {_rpt_io = io;}
     RADARPARAMIOTYPE   get_io_type(void)                   {return _rpt_io;}

     QString            get_group(void)                     {return _group;}
     void               set_group(QString group)            {_group = group;}

     virtual            bool    save_xml(QDomDocument& owner, QDomElement& root) = 0;
     virtual            bool    load_xml(QDomDocument& owner, QDomElement& root) = 0;
     static std::shared_ptr<radarParamBase> create_from_xml_node(QDomDocument& owner, QDomElement& root);
     virtual            QByteArray chain_values() = 0;
     virtual            bool       split_values(const QByteArray& data) = 0;


     QString            get_command_string() {return _command_string;}
     QByteArray         get_command_group();
     int                get_command_order_group();
     bool               is_command_group();
     void               set_command_string(QString command);
    // Allow for cross-link (in this case, _var is used instead of paramName)
     void               set_alias_octave_name(const QString& var);
     QString            get_alias_octave_name();
     void               unset_alias_octave_name();
     void               link_to_workspace(octavews* workspace);
     octavews*          get_workspace();

     virtual void   var_changed() = 0;

     bool       has_inquiry_value() {return _has_inquiry_value;}
     void       set_inquiry_value() {_has_inquiry_value = false;}

     virtual    unsigned int value_bytes_count() {return 0;}

     bool       is_linked_to_octave();
     void       set_compound_name(bool b_compound_name = true);
     bool       is_compound_name();
     virtual octave_value get_value();
     virtual void         set_value(const octave_value& val);
     virtual QVariant get_min() {return QVariant();}
     virtual QVariant get_max() {return QVariant();}
     bool         operator == (const radarParamBase& param);

     bool       is_plotted() {return _b_plotted;}
     void       set_plotted(bool bplot) {_b_plotted = bplot;}

     PLOT_TYPE get_plot_type() {return _plottype;}
     void               set_plot_type(PLOT_TYPE plot) {_plottype = plot;}


};


template<typename T> class radarParameter : public radarParamBase
{

private:
    Array<T>                   _value;
    T                          _min, _max;
    QByteArray                 _inquiry_value;
    Array<T>                   _availableset;
    bool                        validate_data();
    void update_variable() override;
public:
    radarParameter();
    radarParameter(QString parameterName);
    radarParameter(const radarParameter<T>& v2) :
        radarParamBase(v2),
        _value(v2._value),
        _min(v2._min),
        _max(v2._max),
        _inquiry_value(v2._inquiry_value),
        _availableset(v2._availableset)
    {
    }
    virtual std::shared_ptr<radarParamBase> clone() const override
    {
        return std::shared_ptr<radarParamBase>(new radarParameter<T>(*this));
    }
    bool                is_valid(T val);
    bool                is_valid(QVariant val) override;
    ~radarParameter() {};
    bool                is_scalar() override;
    const Array<T>&     value();
    void                value(const T& val);
    void                value(T& val);
    void                value(QVector<T>& val);
    void                value(Array<T>& val);
    void                set_min_max(T min, T max);
    void                set_min_max(QVariant min, QVariant max) override;
    void                remove_min_max();
    void                remove_available_set();
    void                set_list_available_values(Array<T> valList=Array<T>());
    void                set_list_available_values(QVector<T> valList=QVector<T>());
    bool                has_available_set() override;
    Array<T>            get_list_available_values() {return _availableset;}

    void                set_inquiry_value(const QByteArray& inquiry_val);
    QByteArray          get_inquiry_value() override;
    void                variant_to_inquiry_value(const QVariant& data) override;
    QVariant            inquiry_value_to_variant() override;

    void                restore_default();
    radarParameter<T>&      operator = (radarParameter<T>& t2);
    radarParameter<T>&      operator = (const radarParameter<T>& t2);

    QVector<QVariant>       value_to_variant() override;
    QVector<QVariant>       availableset_to_variant() override;

    void variant_to_value(QVector<QVariant>& data) override;
    void variant_to_availabeset(QVector<QVariant>& data) override;
    void variant_to_value(const QVector<QVariant>& data) override;
    void variant_to_availabeset(const QVector<QVariant>& data) override;

    QString get_min_string() override;
    QString get_max_string() override;
    bool set_min_string(QString strval);
    bool set_max_string(QString strval);
    bool validate_string(QString strval);
    int                       get_index_of_value(QVariant value) override;
    bool                    save_xml(QDomDocument& owner, QDomElement& root)  override;
    bool                    load_xml(QDomDocument& owner, QDomElement& root)  override;
    QByteArray        chain_values() override;
    bool                    split_values(const QByteArray& data) override;

    void                  var_changed() override;
    unsigned int    value_bytes_count() override;

    int                     get_expected_payload_size() override;

    octave_value get_value() override;
    void                set_value(const octave_value& val) override;
    bool                operator == (const radarParameter<T>& param);
    QVariant get_min() override;
    QVariant get_max() override;
};


template<> class radarParameter<QString> : public radarParamBase
{

private:
    QVector<QString>                   _value;
    QString                            _min, _max;
    QVector<QString>                   _availableset;
    bool                                validate_data();
    QByteArray                         _inquiry_value;
    void update_variable() override;
public:
    radarParameter();
    radarParameter(QString parameterName) : radarParamBase(parameterName, RPT_STRING)
    {

    }

    radarParameter(const radarParameter<QString>& v2) :
        radarParamBase(v2),
        _value(v2._value),
        _min(v2._min),
        _max(v2._max),
        _availableset(v2._availableset),
        _inquiry_value(v2._inquiry_value)
    {
    }
    virtual std::shared_ptr<radarParamBase> clone() const override
    {
        return std::shared_ptr<radarParamBase>(new radarParameter<QString>(*this));
    }
    bool                is_valid(QString val);
    bool                is_valid(QVariant val) override;
    ~radarParameter() {};
    bool                is_scalar() override;
    const QVector<QString>&     value();
    void                value(const QString& val);
    void                value(QString& val);
    void                value(QVector<QString>& val);
    const QVector<QString>&     default_value();
    void                default_value(const QString& val);
    void                default_value(QVector<QString>& val);
    void                set_min_max(QString min, QString max);
    void                set_min_max(QVariant min, QVariant max) override;
    void                remove_min_max();
    void                remove_available_set();
    void                set_list_available_values(QVector<QString> valList=QVector<QString>());

    QVector<QString>            get_list_available_values() {return _availableset;}

    void                        restore_default();
    radarParameter<QString>&      operator = (radarParameter<QString>& t2);
    radarParameter<QString>&      operator = (const radarParameter<QString>& t2);

    QVector<QVariant>       value_to_variant() override;
    QVector<QVariant>       availableset_to_variant() override;

    void variant_to_value(QVector<QVariant>& data) override;
    void variant_to_availabeset(QVector<QVariant>& data) override;
    void variant_to_value(const QVector<QVariant>& data) override;
    void variant_to_availabeset(const QVector<QVariant>& data) override;
    void                set_inquiry_value(const QByteArray& inquiry_val);
    QByteArray          get_inquiry_value() override;
    void                variant_to_inquiry_value(const QVariant& data) override;
    QVariant            inquiry_value_to_variant() override;



    bool                has_available_set() override;
    QString get_min_string() override;
    QString get_max_string() override;
    bool set_min_string(QString strval);
    bool set_max_string(QString strval);
    bool validate_string(QString strval);
    int  get_index_of_value(QVariant value) override;

    bool    save_xml(QDomDocument& owner, QDomElement& root)  override;
    bool    load_xml(QDomDocument& owner, QDomElement& root)  override;
    QByteArray chain_values() override;
    bool       split_values(const QByteArray& data) override;
    void   var_changed() override;
    unsigned int value_bytes_count() override;
    int                get_expected_payload_size() override;

    octave_value get_value() override;
    void         set_value(const octave_value& val) override;
    bool         operator == (const radarParameter<QString>& param);
    QVariant get_min() override;
    QVariant get_max() override;

};

// This is a bit sneaky since when using enums, _availableset is different. In case of
// enum, we want to store description too.
template<> class radarParameter<enumElem> : public radarParamBase
{

private:
    radarEnum                       _value;
    radarEnum                       _availableset;
    bool                            validate_data();
    QByteArray                      _inquiry_value;
    void update_variable() override;

public:
    radarParameter(QString parameterName) : radarParamBase(parameterName, RPT_ENUM)
    {
    }

    radarParameter(const radarParameter<enumElem>& v2) :
        radarParamBase(v2),
        _value(v2._value),
        _availableset(v2._availableset),
        _inquiry_value(v2._inquiry_value)
    {
    }
    ~radarParameter<enumElem>() {};

    virtual std::shared_ptr<radarParamBase> clone() const override
    {
        return std::shared_ptr<radarParamBase>(new radarParameter<enumElem>(*this));
    }

    bool                            is_scalar() override;
    bool                            is_valid(QString val);
    bool                            is_valid(uint8_t val);
    bool                            is_valid(enumElem val);
    bool                            is_valid(QVariant val) override;
    const radarEnum&                value() {return _value;}
    void                            value(QString currval);
    void                            value(enumElem currval);
    void                            value(QVector<QString> currval);
    radarEnumIterator               get_element(QString value);
    radarEnumIterator               get_element(uint8_t valint);
    void                            set_min_max(uint8_t min, uint8_t max) {};
    void                            set_min_max(QVariant min, QVariant max) override {};
    void                            set_list_available_values(const radarEnum& valList)   {_availableset = valList;  if (valList.count()>0) _hasMinMax=false;}
    radarParameter<enumElem>&      operator = (radarParameter<enumElem>& t2);
    radarParameter<enumElem>&      operator = (const radarParameter<enumElem>& t2);

    radarEnum                       get_list_available_values() {return _availableset;}
    radarEnum                       get_values() {return _value;}

    QVector<QVariant>               value_to_variant() override;
    QVector<QVariant>               availableset_to_variant() override;
    bool                            has_available_set() override;

    void                    variant_to_value(QVector<QVariant>& data) override;
    void                    variant_to_availabeset(QVector<QVariant>& data) override;
    void                    variant_to_value(const QVector<QVariant>& data) override;
    void                    variant_to_availabeset(const QVector<QVariant>& data) override;
    void                            set_inquiry_value(const QByteArray& inquiry_val);
    QByteArray                      get_inquiry_value() override;
    void                            variant_to_inquiry_value(const QVariant& data) override;
    QVariant                        inquiry_value_to_variant() override;

    QString get_min_string() override {return "Not Avail for Enums";}
    QString get_max_string() override {return "Not Avail for Enums";}
/*
    bool set_min_string(QString strval);
    bool set_max_string(QString strval);
*/
    bool validate_string(QString strval);
    int  get_index_of_value(QVariant value) override;
    int  get_index_of_value(QString  value);
    int  get_index_of_value(uint8_t   value);

    bool    save_xml(QDomDocument& owner, QDomElement& root)  override;
    bool    load_xml(QDomDocument& owner, QDomElement& root)  override;

    QByteArray chain_values() override;
    bool       split_values(const QByteArray& data) override;

    int        get_pair(const QString& strValue);
    int        get_pair(int strIndex);
    unsigned int value_bytes_count() override;
    int          get_expected_payload_size() override;

    void   var_changed() override;

    octave_value get_value() override;
    void         set_value(const octave_value& val) override;
    bool         operator == (const radarParameter<enumElem>& param);
    QVariant get_min() override {return QVariant();}
    QVariant get_max() override{return QVariant();}
};




template<> class radarParameter<void> : public radarParamBase
{

private:
    bool                                validate_data() {return true;}
    QString                            _inquiry_value;
    void                                update_variable() override {};
public:    
    radarParameter(QString parameterName) : radarParamBase(parameterName, RPT_VOID)
    {
    }

    radarParameter(const radarParameter<void>& v2) :
        radarParamBase(v2),
        _inquiry_value(v2._inquiry_value)
    {
    }

    virtual std::shared_ptr<radarParamBase> clone() const override
    {
        return std::shared_ptr<radarParamBase>(new radarParameter<void>(*this));
    }

    ~radarParameter() {};


    bool                is_valid(QVariant val) override {return val.isNull();}
    bool                is_scalar() override {return false;}
    void                set_min_max(QVariant min, QVariant max) override {}

    radarParameter<void>&      operator = (radarParameter<void>& t2);
    radarParameter<void>&      operator = (const radarParameter<void>& t2);

    QVector<QVariant>       value_to_variant() override       {return QVector<QVariant>();}
    QVector<QVariant>       availableset_to_variant() override  {return QVector<QVariant>();}

    void variant_to_value(QVector<QVariant>& data) override {};
    void variant_to_availabeset(QVector<QVariant>& data) override {};
    void variant_to_value(const QVector<QVariant>& data) override {};
    void variant_to_availabeset(const QVector<QVariant>& data) override{};
    void                set_inquiry_value(const QByteArray& inquiry_val) {};
    QByteArray          get_inquiry_value()   override  {return QByteArray();}
    void                variant_to_inquiry_value(const QVariant& data) override {};
    QVariant            inquiry_value_to_variant() override {return QVariant();}

    bool                has_available_set() override {return false;}
    int                 get_index_of_value(QVariant value) override {return -1;}

    bool    save_xml(QDomDocument& owner, QDomElement& root)  override;
    bool    load_xml(QDomDocument& owner, QDomElement& root)  override;
    QByteArray chain_values() override;
    bool       split_values(const QByteArray& data) override;
    void       var_changed() override;

    QString get_min_string() override {return "Not Avail for Void";}
    QString get_max_string() override {return "Not Avail for Void";}

    unsigned int value_bytes_count() override {return 0;}
    int                get_expected_payload_size() override {return 0;}

    octave_value get_value() override;
    void         set_value(const octave_value& val) override;
    bool         operator == (const radarParameter<void>& param);
    QVariant get_min() override {return QVariant();}
    QVariant get_max() override{return QVariant();}
};

template class radarParameter<int8_t>;
template class radarParameter<uint8_t>;
template class radarParameter<int16_t>;
template class radarParameter<uint16_t>;
template class radarParameter<int32_t>;
template class radarParameter<uint32_t>;
template class radarParameter<float>;
template class radarParameter<char>;


typedef std::shared_ptr<radarParamBase> radarParamPointer;

// Pre-defined

#endif // RADARPARAMETER_H
