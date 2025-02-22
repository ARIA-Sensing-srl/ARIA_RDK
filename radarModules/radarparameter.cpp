/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "radarparameter.h"
#include "ov.h"
#include "Array.h"
#include "Array.cc"
#include <ariautils.h>

QRegularExpression re("(.+)\\[(\\d+)\\]$");

RADARPARAMTYPE strToTypeId(QString typeString)
{
    if (typeString == "float") return RPT_FLOAT;
    if (typeString == "int8")   return RPT_INT8;
    if (typeString == "uint8")  return RPT_UINT8;
    if (typeString == "int16")  return RPT_INT16;
    if (typeString == "uint16") return RPT_UINT16;
    if (typeString == "int32")  return RPT_INT32;
    if (typeString == "uint32") return RPT_UINT32;
    if (typeString == "enum")   return RPT_ENUM;
    if (typeString == "string") return RPT_STRING;
    if (typeString == "void") return RPT_VOID;
    return RPT_UNDEFINED;
}

RADARPARAMIOTYPE strToIOType(QString ioString)
{
    if (ioString=="input")  return RPT_IO_INPUT;
    if (ioString=="output") return RPT_IO_OUTPUT;
    return RPT_IO_IO;
}

QString typeIdToString(RADARPARAMTYPE rpt)
{
    if (rpt==RPT_FLOAT)    return "float";
    if (rpt==RPT_INT8)      return "int8";
    if (rpt==RPT_UINT8)     return "uint8";
    if (rpt==RPT_INT16)     return "int16";
    if (rpt==RPT_UINT16)    return "uint16";
    if (rpt==RPT_INT32)     return "int32";
    if (rpt==RPT_UINT32)    return "uint32";
    if (rpt==RPT_ENUM)      return "enum";
    if (rpt==RPT_STRING)    return "string";
    if (rpt==RPT_VOID)      return "void";
    return "undefined";
}

QString ioTypeToString(RADARPARAMIOTYPE rpio)
{
    if (rpio==RPT_IO_INPUT)  return "input";
    if (rpio==RPT_IO_OUTPUT) return "output";
    return "io";
}

QString strSizeToString(RADARPARAMSIZE rpsize)
{
    if (rpsize==RPT_SCALAR)  return "scalar";
    if (rpsize==RPT_NDARRAY) return "ndarray";
    return "scalar";
}


RADARPARAMSIZE strToSizeType(QString sizeString)
{
    if (sizeString=="scalar")  return RPT_SCALAR;
    if (sizeString=="ndarray") return RPT_NDARRAY;
    return RPT_SCALAR;
}


bool       radarParamBase::is_linked_to_octave()
{
    return !(_var.isEmpty());
}

void       radarParamBase::set_compound_name(bool b_compound_name )
{
    _b_compound_name = b_compound_name;
}
bool       radarParamBase::is_compound_name()
{
    return _b_compound_name;
}


std::shared_ptr<radarParamBase> radarParamBase::create_from_xml_node(QDomDocument& owner, QDomElement& root)
{
    std::shared_ptr<radarParamBase> out(nullptr);

    if (root.isNull())
        return out;

    QString paramname = root.attribute("param_name");
    if (paramname.isEmpty()) return out;

    RADARPARAMTYPE rpt = strToTypeId(root.attribute("param_type"));
    if (rpt == RPT_UNDEFINED) return out;

    RADARPARAMIOTYPE rpio = strToIOType(root.attribute("param_io"));

    QString group = root.attribute("param_group","");

    if (rpt==RPT_FLOAT)
        out = std::make_shared<radarParameter<float>>(paramname);
    if (rpt==RPT_INT8)
        out = std::make_shared<radarParameter<int8_t>>(paramname);
    if (rpt==RPT_UINT8)
        out = std::make_shared<radarParameter<uint8_t>>(paramname);
    if (rpt==RPT_INT16)
        out = std::make_shared<radarParameter<int16_t>>(paramname);
    if (rpt==RPT_UINT16)
        out = std::make_shared<radarParameter<uint16_t>>(paramname);
    if (rpt==RPT_INT32)
        out = std::make_shared<radarParameter<int32_t>>(paramname);
    if (rpt==RPT_UINT32)
        out = std::make_shared<radarParameter<uint32_t>>(paramname);
    if (rpt==RPT_ENUM)
        out = std::make_shared<radarParameter<enumElem>>(paramname);
    if (rpt==RPT_STRING)
        out = std::make_shared<radarParameter<QString>>(paramname);
    if (rpt==RPT_VOID)
        out = std::make_shared<radarParameter<void>>(paramname);

    RADARPARAMSIZE size = strToSizeType(root.attribute("scalar","scalar"));


    out->set_io_type(rpio);
    out->set_group(group);
    out->set_size(size);
    out->set_command_string(root.attribute("command_string"));
    out->set_alias_octave_name(root.attribute("octave_alias",""));
    if (out->is_linked_to_octave())
    {
        bool name_is_compound    = root.attribute("name_compound","false") == "true";
        out->set_compound_name(name_is_compound);
    }

    out->set_plotted(root.attribute("plotted","false")=="true");
    if (out->is_plotted())
    {
        out->set_plot_type(strToPlotTypeId(root.attribute("plot_type","plot")));
    }
    if (!out->load_xml(owner, root)) return std::shared_ptr<radarParamBase>(nullptr);

    root = root.nextSiblingElement("parameter");
    return out;
}
//------------------------------------------------------
QByteArray radarParamBase::get_command_group()
{
    return _pure_command;
}
//------------------------------------------------------
int     radarParamBase::get_command_order_group()
{
    return _group_order;
}
//------------------------------------------------------
bool    radarParamBase::is_command_group()
{
    return _is_grouped;
}
//------------------------------------------------------
void    radarParamBase::set_command_string(QString command)
{
    _command_string = command;

    QRegularExpressionMatch match = re.match(command);
    if (match.hasMatch())
    {
        _pure_command = QByteArray::fromHex(match.captured(1).toLatin1());
        _group_order  = match.captured(2).toInt();
        _is_grouped   = true;
    }
    else
    {
        _pure_command = QByteArray::fromHex(command.toLatin1());
        _group_order  = -1;
        _is_grouped   = false;
    }
}

//------------------------------------------------------
radarParamBase& radarParamBase::operator = (radarParamBase& v2)
{
    _rpt = v2._rpt;
    _rpt_io = v2._rpt_io;
    _paramname = v2._paramname;
    _group = v2._group;
    _hasMinMax=v2._hasMinMax;
    _isValid = v2._isValid;
    _has_inquiry_value = v2._has_inquiry_value;
    _command_string = v2._command_string;
    _rpt_size = v2._rpt_size;
    _var = v2._var;
    _is_grouped = v2._is_grouped;
    _group_order = v2._group_order;
    _pure_command= v2._pure_command;
    _b_compound_name = v2._b_compound_name;
    _workspace = v2._workspace;
    _b_is_transmitting = v2._b_is_transmitting;
    _status = v2._status;
    return *this;
}
//------------------------------------------------------
radarParamBase& radarParamBase::operator = (const radarParamBase& v2)
{
    _rpt = v2._rpt;
    _rpt_io = v2._rpt_io;
    _paramname = v2._paramname;
    _b_compound_name = v2._b_compound_name;
    _group = v2._group;
    _hasMinMax=v2._hasMinMax;
    _isValid = v2._isValid;
    _has_inquiry_value = v2._has_inquiry_value;
    _command_string = v2._command_string;
    _rpt_size = v2._rpt_size;
    _var = v2._var;
    _is_grouped = v2._is_grouped;
    _group_order = v2._group_order;
    _pure_command= v2._pure_command;
    _workspace = v2._workspace;
    _b_is_transmitting = v2._b_is_transmitting;
    _status = v2._status;
    return *this;
}
//------------------------------------------------------
void    radarParamBase::set_alias_octave_name(const QString& var)
{
    _var = var;
}
//------------------------------------------------------
void    radarParamBase::unset_alias_octave_name()
{
    _var = "";
}
//------------------------------------------------------
QString radarParamBase::get_alias_octave_name()
{
    return _var;
}
//------------------------------------------------------
void               radarParamBase::link_to_workspace(octavews* workspace)
{
    _workspace = workspace;
}
octavews*          radarParamBase::get_workspace()
{
    return _workspace;
}

//------------------------------------------------------
octave_value radarParamBase::get_value()
{
    return octave_value();
}
//------------------------------------------------------
void         radarParamBase::set_value(const octave_value& )
{}
//------------------------------------------------------
template<typename T> radarParameter<T>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_UNDEFINED),
	_value(dim_vector({0,0}))
{
}
//------------------------------------------------------
bool    radarParamBase::operator == (const radarParamBase& param)
{
    if (_rpt    != param._rpt)              return false;
    if (_rpt_io != param._rpt_io)           return false;
    if (_rpt_size!=param._rpt_size)         return false;
    if (_paramname != param._paramname)     return false;
    if (_group != param._group)             return false;
    if (_hasMinMax != param._hasMinMax)     return false;
    if (_has_inquiry_value!=param._has_inquiry_value)
        return false;
    if (_command_string!=param._command_string)
        return false;
    if (_var != param._var)                 return false;
    if (_is_grouped!=param._is_grouped)     return false;
    if (_group_order!=param._group_order)   return false;
    if (_pure_command!=param._pure_command) return false;
    if (_b_compound_name!=param._b_compound_name)
        return false;

    return true;
}


//------------------------------------------------------
template<typename T> radarParameter<T>::radarParameter() : radarParamBase("paramName", RPT_UNDEFINED),
	_value(dim_vector({0,0}))
{
}
template <typename T> bool radarParameter<T>::has_available_set()
{
    return !(_availableset.isempty());
}
//------------------------------------------------------
template<typename T>   const Array<T>&  radarParameter<T>::value()
{
    return _value;
}

//----------------------------------------------------
template<typename T> bool radarParameter<T>::validate_data()
{
    for (int n=0; n < _value.numel(); n++)
        if (!is_valid(_value(n)))
            return false;

    return true;
}
//----------------------------------------------------
template<typename T>   bool radarParameter<T>::is_scalar()
{
    return _value.numel()==1;
}

//----------------------------------------------------
template<typename T>   bool radarParameter<T>::is_valid(T val)
{

    if (_hasMinMax)
        return ((val >= _min)&&(val <=_max));

    if (has_available_set())
    {
        for (int n=0; n < _availableset.numel(); n++)
            if (_availableset(n)==val) return true;

        return false;
    }
    return true;
}
//----------------------------------------------------
template<typename T>   bool radarParameter<T>::is_valid(QVariant val)
{
    return is_valid(val.value<T>());
}
//------------------------------------------------------
template<typename T>   void  radarParameter<T>::value(const T& val)
{
    _isValid = is_valid(val);
    if (!_isValid)
        return;
    _value.resize(dim_vector(1,1),val);
}
//------------------------------------------------------
template<typename T>   void  radarParameter<T>::value(T& val)
{
    _isValid = is_valid(val);
    if (!_isValid)
        return;
    _value.resize(dim_vector(1,1),val);
}

//------------------------------------------------------
template<typename T>   void  radarParameter<T>::value(Array<T>& val)
{
    for (int n=0; n<val.numel(); n++)
        if (!is_valid(val(n)))
        {
            _isValid = false;
            return;
        }

    _value = val;
}
//-----------------------------------------------------
template<typename T>   void radarParameter<T>::value(QVector<T>& val)
{
    for (int n=0; n<val.count(); n++)
        if (!is_valid(val[n]))
        {
            _isValid = false;
            return;
        }
    int data_size = get_size() == RPT_SCALAR ? 1 : val.count();
    if (val.count()==0) data_size = 0;
    _value.resize(dim_vector(1,data_size));
    for (int n=0; n < data_size; n++)
        _value(n) = val[n];
}

//------------------------------------------------------
template<typename T>   void radarParameter<T>::set_min_max(T min, T max)
{
    _hasMinMax = true;    
    _availableset.clear();

    _min = min;
    _max = max;


    _isValid = validate_data();
}
//------------------------------------------------------
template<typename T>   void radarParameter<T>::set_min_max(QVariant min, QVariant max)
{
    _hasMinMax = true;
    _availableset.clear();

    _min = min.value<T>();
    _max = max.value<T>();


    _isValid = validate_data();

}
//----------------------------------------------------
template<typename T> void radarParameter<T>::remove_available_set()
{
    _availableset.clear();
}


//------------------------------------------------------
template<typename T> void radarParameter<T>::remove_min_max()
{
    _hasMinMax = false;
}
//------------------------------------------------------
template<typename T>   void radarParameter<T>::set_list_available_values(Array<T> valList)
{
    if (!valList.isempty())
        _hasMinMax = false;


    _availableset = valList;
    _isValid = validate_data();
}
//------------------------------------------------------
template<typename T>   void radarParameter<T>::set_list_available_values(QVector<T> valList)
{

    if (!valList.empty())
        _hasMinMax= false;

    _availableset.resize(dim_vector(1,valList.count()));
    for (int n=0; n < valList.count(); n++)
        _availableset(n)=valList[n];

    _isValid = validate_data();
}
//------------------------------------------------------
template<typename T> radarParameter<T>& radarParameter<T>::operator = (radarParameter<T>& t2)
{
    // Call base equal
    radarParamBase::operator=(t2);
    _value       = t2._value;
    _availableset       = t2._availableset;
    _min                = t2._min;
    _max                = t2._max;

    return *this;
}
//------------------------------------------------------
template<typename T> radarParameter<T>& radarParameter<T>::operator = (const radarParameter<T>& t2)
{
    // Call base equal
    radarParamBase::operator=(t2);
    _value       = t2._value;
    _availableset       = t2._availableset;
    _hasMinMax          = t2._hasMinMax;
    _min                = t2._min;
    _max                = t2._max;

    return *this;
}

//------------------------------------------------------
template<typename T> QVector<QVariant>       radarParameter<T>::value_to_variant()
{
    QVector<QVariant> out(_value.numel());
    for (int n=0; n < _value.numel(); n++)
        out[n] = QVariant::fromValue<T>(_value.elem(n));

    return out;
}
//------------------------------------------------------
template<typename T> QVector<QVariant>       radarParameter<T>::availableset_to_variant()
{
    QVector<QVariant> out(_availableset.numel());
    for (int n=0; n < _availableset.numel(); n++)
        out[n] = QVariant::fromValue<T>(_availableset.elem(n));

    return out;
}
//------------------------------------------------------
template<typename T> void       radarParameter<T>::set_inquiry_value(const QByteArray& inquiry_val)
{
    _has_inquiry_value = inquiry_val.length()>0;

    _inquiry_value = inquiry_val;

    if (!_has_inquiry_value) return;
    if (inquiry_val.length()>(int)sizeof(T))
        _inquiry_value.truncate(sizeof(T));
    if (inquiry_val.length()<(int)sizeof(T))
        _inquiry_value.append(sizeof(T)-inquiry_val.length(),char(0x00));

}
//------------------------------------------------------
template<typename T> QByteArray          radarParameter<T>::get_inquiry_value()
{
    return _inquiry_value;
}
//------------------------------------------------------
template<typename T> void       radarParameter<T>::variant_to_inquiry_value(const QVariant& data)
{
    QString str = data.toString();
    QByteArray inquiry_val = QByteArray::fromHex(str.toLatin1());

    _has_inquiry_value = inquiry_val.length()>0;

    _inquiry_value = inquiry_val;

    if (!_has_inquiry_value) return;
    if (inquiry_val.length()>(int)sizeof(T))
        _inquiry_value.truncate(sizeof(T));
    if (inquiry_val.length()<(int)sizeof(T))
        _inquiry_value.append(sizeof(T)-inquiry_val.length(),char(0x00));
}
//------------------------------------------------------
template<typename T> QVariant   radarParameter<T>::inquiry_value_to_variant()
{
    return QVariant(_inquiry_value.toHex(0));
}

//------------------------------------------------------
template<typename T> void radarParameter<T>::variant_to_value(QVector<QVariant>& data)
{
    int data_size = get_size() == RPT_SCALAR ? 1 : data.count();
    // Check availability
    if (data.count()==0) data_size= 0;
    for (int n=0; n < data_size; n++)
    {
        T val = data[n].value<T>();
        if (!is_valid(val))
        {
            _isValid = false;
            return;
        }
    }
    _isValid = true;
    _value.resize(dim_vector(1,data_size));

    for (int n=0; n < data_size; n++)
        _value(n) = data[n].value<T>();
}
//------------------------------------------------------
template<typename T> void radarParameter<T>::variant_to_value(const QVector<QVariant>& data)
{
    int data_size = get_size() == RPT_SCALAR ? 1 : data.count();
    // Check availability
    if (data.count()==0) data_size= 0;
    for (int n=0; n < data_size; n++)
    {
        T val = data[n].value<T>();
        if (!is_valid(val))
        {
            _isValid = false;
            return;
        }
    }
    _isValid = true;
    _value.resize(dim_vector(1,data_size));

    for (int n=0; n < data_size; n++)
        _value(n) = data[n].value<T>();
}
//------------------------------------------------------
template<typename T> void radarParameter<T>::variant_to_availabeset(QVector<QVariant>& data)
{
    _availableset.resize(dim_vector(1,data.count()));
    // Check availability

    for (int n=0; n < data.count(); n++)
        _availableset(n) = data[n].value<T>();

    if (data.count()>0)
        _hasMinMax = false;

    _isValid = validate_data();
}
//------------------------------------------------------
template<typename T> void radarParameter<T>::variant_to_availabeset(const QVector<QVariant>& data)
{
    _availableset.resize(dim_vector(1,data.count()));
    // Check availability
    for (int n=0; n < data.count(); n++)
        _availableset(n) = data[n].value<T>();

    if (data.count()>0)
            _hasMinMax = false;

    _isValid = validate_data();
}
//------------------------------------------------------
template<typename T> QString radarParameter<T>::get_min_string()
{
    QVariant t=QVariant::fromValue<T>(_min);
    return t.toString();
}
//------------------------------------------------------
template<typename T> QString radarParameter<T>::get_max_string()
{
    QVariant t=QVariant::fromValue<T>(_max);
    return t.toString();
}
//------------------------------------------------------
template<typename T> int  radarParameter<T>::get_index_of_value(QVariant value)
{
    T val = value.value<T>();

    for (int n=0; n<_availableset.numel(); n++)
        if (_availableset(n)==val)
            return n;
    return -1;
}
//------------------------------------------------------
template<typename T> bool radarParameter<T>::set_min_string(QString strval)
{
    QVariant t = strval;
    T val = t.value<T>();
    _min = val;
    _hasMinMax = true;
    _availableset.clear();

    return validate_data();
}
//------------------------------------------------------
template<typename T> bool radarParameter<T>::set_max_string(QString strval)
{
    QVariant t = strval;
    T val = t.value<T>();
    _max = val;
    _hasMinMax = true;
    _availableset.clear();

    return validate_data();
}
//------------------------------------------------------
template<typename T> bool radarParameter<T>::validate_string(QString strval)
{
    QVariant t = strval;
    T val = t.value<T>();
    return is_valid(val);
}
//------------------------------------------------------
template<typename T> bool    radarParameter<T>::save_xml(QDomDocument& owner, QDomElement& root)
{
    QDomElement element = owner.createElement("parameter");
    element.setAttribute("param_name",_paramname);
    element.setAttribute("param_type",typeIdToString(_rpt));
    element.setAttribute("param_io",  ioTypeToString(_rpt_io));
    element.setAttribute("param_group",_group);
    element.setAttribute("command_string", _command_string);
    element.setAttribute("size",strSizeToString(get_size()));

    if (is_linked_to_octave())
    {
        element.setAttribute("name_compound",_b_compound_name ? "true":"false");
        element.setAttribute("octave_alias",_var);
    }

    element.setAttribute("plotted",_b_plotted ? QString("true") : QString("false") );
    if (_b_plotted)
        element.setAttribute("plot_type",plotTypeIdToString(_plottype));

    if (_hasMinMax)
    {
        QDomElement minmax = owner.createElement("min_max_values");
        minmax.setAttribute("min_val", get_min_string());
        minmax.setAttribute("max_val", get_max_string());
        element.appendChild(minmax);
    }

    if (!_availableset.isempty())
    {
        QDomElement availableset_elem = owner.createElement("available_set");
        availableset_elem.setAttribute("size",QString::number(_availableset.numel()));

        QStringList availStream;
        for (int n=0; n < _availableset.numel(); n++)
        {
            availStream << QString::number((double)(_availableset(n)));
            //QVariant var = QVariant::fromValue<T>(_availableset(n));
            //availStream << var.toString();
        }

        QDomText fText = owner.createTextNode(availStream.join("\n"));
        availableset_elem.appendChild(fText);

        element.appendChild(availableset_elem);
    }

    if (_has_inquiry_value)
    {
        QDomElement inquiry_value = owner.createElement("inquiry_value");
        inquiry_value.setAttribute("value", QString(_inquiry_value.toHex(0)));
        element.appendChild(inquiry_value);
    }


    QDomElement current_set = owner.createElement("values");
    current_set.setAttribute("size",QString::number(_value.numel()));

    QStringList valueStream;
    for (int n=0; n < _value.numel(); n++)
    {
        //QVariant var = QVariant::fromValue<T>(_value(n));

        valueStream << QString::number((double)(_value(n)));
    }

    QDomText fText = owner.createTextNode(valueStream.join("\n"));
    current_set.appendChild(fText);

    element.appendChild(current_set);

    root.appendChild(element);

    return true;
}
//------------------------------------------------------
template<typename T> bool    radarParameter<T>::load_xml(QDomDocument& owner, QDomElement& param)
{    
    QDomElement minmaxElem = param.firstChildElement("min_max_values");

    if (!minmaxElem.isNull())
    {
        double vmin = QVariant(minmaxElem.attribute("min_val","0")).value<T>();
        double vmax = QVariant(minmaxElem.attribute("max_val","1")).value<T>();
        if (vmax < vmin) {double t = vmax; vmax = vmin; vmin=t;}
        set_min_max(vmin,vmax);

    }
    else
        remove_min_max();

    bool bOk;
    QDomElement availset_elem = param.firstChildElement("available_set");
    if (!availset_elem.isNull())
    {
        int expected_size = availset_elem.attribute("size").toInt(&bOk);
        if (!bOk)
            return false;

        QStringList dataStream;
        dataStream = availset_elem.text().split("\n");
        dataStream.removeAll("");
        if (dataStream.count()!=expected_size)
            return false;
        _availableset.resize(dim_vector(1,expected_size));
        for (int n=0; n < dataStream.count(); n++)
        {
            T val = (T)(dataStream[n].toDouble());
            _availableset(n) = val;
        }
    }
    else
        remove_available_set();

    QDomElement inquiry_value = param.firstChildElement("inquiry_value");
    if (inquiry_value.isNull())
    {
        _has_inquiry_value = false;
    }
    else
    {
        QVariant val_str = QVariant(inquiry_value.attribute("value",""));
        variant_to_inquiry_value(val_str);
    }

    QDomElement current_elem = param.firstChildElement("values");
    if (current_elem.isNull())
    {
        _value.clear();
    }
    else
    {
        int expected_size = current_elem.attribute("size").toInt(&bOk);
        if (!bOk)
            return false;

        QStringList dataStream = current_elem.text().split("\n");
        dataStream.removeAll("");
        if (dataStream.count()!=expected_size)
            return false;

        _value.resize(dim_vector(1,expected_size));
        for (int n=0; n < dataStream.count(); n++)
        {
            T val = (T)(dataStream[n].toDouble());
            _value(n) = val;
        }
    }
    return true;
}
//------------------------------------------------------
template<typename T> QByteArray radarParameter<T>::chain_values()
{
    QByteArray out;
    for (int n=0; n<_value.numel();n++)
    {
        T val = _value(n);
        int nchar = sizeof(T);

        char* ptr = (char*)(&val);
        out.append(ptr,nchar);
    }
    return out;
}
//------------------------------------------------------
template<typename T> void radarParameter<T>::convert_data(const octave_value& data_in)
{
    if (data_in.is_int8_type())
    {
        _value.resize(data_in.dims());
        int8NDArray array_in = data_in.int8_array_value();
        for (int n=0; n < array_in.numel(); n++)
            _value(n)=(T)array_in(n);
        return;
    }
    if (data_in.is_uint8_type())
    {
        _value.resize(data_in.dims());
        uint8NDArray array_in = data_in.uint8_array_value();
        for (int n=0; n < array_in.numel(); n++)
            _value(n)=(T)(array_in(n));
        return;
    }
    if (data_in.is_int16_type())
    {
        _value.resize(data_in.dims());
        int16NDArray array_in = data_in.int16_array_value();
        for (int n=0; n < array_in.numel(); n++)
            _value(n)=(T)array_in(n);
        return;
    }
    if (data_in.is_uint16_type())
    {
        _value.resize(data_in.dims());
        uint16NDArray array_in = data_in.uint16_array_value();
        for (int n=0; n < array_in.numel(); n++)
            _value(n)=(T)(array_in(n));
        return;
    }
    if (data_in.is_int32_type())
    {
        _value.resize(data_in.dims());
        int32NDArray array_in = data_in.int32_array_value();
        for (int n=0; n < array_in.numel(); n++)
            _value(n)=(T)array_in(n);
        return;
    }
    if (data_in.is_uint32_type())
    {
        _value.resize(data_in.dims());
        uint32NDArray array_in = data_in.uint32_array_value();
        for (int n=0; n < array_in.numel(); n++)
            _value(n)=(T)(array_in(n));
        return;
    }
    if (data_in.is_char_matrix())
    {
        _value.resize(data_in.dims());
        charNDArray array_in = data_in.char_array_value();
        for (int n=0; n < array_in.numel(); n++)
            _value(n)=(T)(array_in(n));
        return;
    }
    if (data_in.isfloat())
    {
        _value.resize(data_in.dims());
        FloatNDArray array_in = data_in.float_array_value();
        for (int n=0; n < array_in.numel(); n++)
            _value(n)=(T)(array_in(n));
        return;
    }
    if (data_in.is_double_type())
    {
        _value.resize(data_in.dims());
        NDArray array_in = data_in.array_value();
        for (int n=0; n < array_in.numel(); n++)
            _value(n)=(T)(array_in(n));
        return;
    }

}
/*
template<typename T> bool radarParameter<T>::split_values(const QByteArray& data)
{
    _value.clear();
    div_t res = std::div(data.count(), sizeof(T));
    if (res.rem!=0) return false;
    _value.resize(dim_vector(1,res.quot));
    for (int n=0, p=0; n < res.quot; n++, p+=sizeof(T))
    {
        QVariant val(data.mid(p, sizeof(T)));
        _value.elem(n) = val.value<T>();
    }
    return true;
}
*/
//------------------------------------------------------
template<> QString radarParameter<uint8_t>::get_min_string()
{
    QVariant t=QVariant::fromValue<int>((int)_min);
    return t.toString();
}
//------------------------------------------------------
template<> QString radarParameter<uint8_t>::get_max_string()
{
    QVariant t=QVariant::fromValue<int>((int)_max);
    return t.toString();
}
//------------------------------------------------------
template<> unsigned int radarParameter<int8_t>::value_bytes_count()
{
    return _value.numel();
}
//------------------------------------------------------
template<> unsigned int radarParameter<uint8_t>::value_bytes_count()
{
    return _value.numel();
}
//------------------------------------------------------
template<> unsigned int radarParameter<int16_t>::value_bytes_count()
{
    return 2*_value.numel();
}
//------------------------------------------------------
template<> unsigned int radarParameter<uint16_t>::value_bytes_count()
{
    return 2*_value.numel();
}
//------------------------------------------------------
template<> unsigned int radarParameter<int32_t>::value_bytes_count()
{
    return 4*_value.numel();
}
//------------------------------------------------------
template<> unsigned int radarParameter<uint32_t>::value_bytes_count()
{
    return 4*_value.numel();
}
//------------------------------------------------------
template<> unsigned int radarParameter<float>::value_bytes_count()
{
    return 4*_value.numel();
}
//------------------------------------------------------
template<> unsigned int radarParameter<char>::value_bytes_count()
{
    return _value.numel();
}


//------------------------------------------------------
template<typename T> int    radarParameter<T>::get_expected_payload_size()
{
    //if (is_scalar())
        return (int)(sizeof(T));

    return -1;
}
//------------------------------------------------------
template <typename T> QVariant radarParameter<T>::get_min()
{
    return QVariant::fromValue<T>(_min);
}
//------------------------------------------------------
template <typename T> QVariant radarParameter<T>::get_max()
{
    return QVariant::fromValue<T>(_max);
}
//------------------------------------------------------
// Enum type
//------------------------------------------------------

radarParameter<enumElem>&      radarParameter<enumElem>::operator = (radarParameter<enumElem>& t2)
{
    radarParamBase::operator=(t2);
    _value       = t2._value;
    _availableset       = t2._availableset;
    return *this;

}
//------------------------------------------------------
radarParameter<enumElem>&      radarParameter<enumElem>::operator = (const radarParameter<enumElem>& t2)
{
    radarParamBase::operator=(t2);
    _value       = t2._value;
    _availableset       = t2._availableset;
    return *this;
}
//------------------------------------------------------
void radarParameter<enumElem>::update_variable()
{
    if (_var.isEmpty()) return;
    if (_workspace==nullptr) return;
    Array<std::string> out;
    out.resize(dim_vector(_value.count(),1));
    for (int n=0; n<_value.count(); n++)
        out(n) = _value[n].first.toStdString();

    _workspace->add_variable(_var.toStdString(), false, out);
//    _var->updateValue(out);
}
//------------------------------------------------------
bool radarParameter<enumElem>::is_scalar()
{
    return _value.count()==1;
}

//------------------------------------------------------
radarEnumIterator   radarParameter<enumElem>::get_element(QString value)
{
    radarEnumIterator avail = _availableset.begin();
    for ( ; avail != _availableset.end(); avail++ )
        if ((*avail).first == value)
            return avail;

    return _availableset.end();
}

//------------------------------------------------------
radarEnumIterator   radarParameter<enumElem>::get_element(uint8_t valint)
{
    radarEnumIterator avail = _availableset.begin();
    for ( ; avail != _availableset.end(); avail++ )
        if ((*avail).second == valint)
            return avail;

    return _availableset.end();
}
//------------------------------------------------------
void radarParameter<enumElem>::value(QString currval)
{
    radarEnumIterator avail = get_element(currval);
    _isValid = avail != _availableset.end();

    if (!_isValid) return;
    _value.resize(1);
    _value[0] = *avail;
}
//------------------------------------------------------
void radarParameter<enumElem>::value(QVector<QString> currval)
{
    int data_size = get_size() == RPT_SCALAR ? 1 : currval.count();
    if (currval.count()==0) data_size = 0;
    QVector<enumElem> pairs(data_size);
    for (int n=0; n<data_size; n++)
    {
        radarEnumIterator avail = get_element(currval[n]);
        _isValid = avail != _availableset.end();
        if (!_isValid) return;
        pairs[n]=(*avail);
    }
    _isValid = true;

    _value=pairs;
}

//----------------------------------------------------
bool radarParameter<enumElem>::validate_data()
{
    for (int n=0; n < _value.count(); n++)
        if (!is_valid(_value[n]))
            return false;

    return true;
}


//----------------------------------------------------
bool radarParameter<enumElem>::is_valid(QString val)
{
    return get_element(val) != _availableset.end();
}

//----------------------------------------------------
bool radarParameter<enumElem>::is_valid(uint8_t val)
{
    return get_element(val) != _availableset.end();
}
//----------------------------------------------------
bool radarParameter<enumElem>::is_valid(enumElem val)
{
    for (int n=0; n < _availableset.count(); n++)
        if ((_availableset[n].first == val.first)&&(_availableset[n].second==val.second))
            return true;
    return false;
}
//------------------------------------------------------
bool radarParameter<enumElem>::is_valid(QVariant val)
{
    return is_valid(val.value<enumElem>());
}
//------------------------------------------------------
QVector<QVariant> radarParameter<enumElem>::value_to_variant()
{
    QVector<QVariant> out(_value.count());
    for (int n=0; n<_value.count();n++)
        out[n] = QVariant::fromValue<enumElem>(_value[n]);
    return out;
}
//------------------------------------------------------
QVector<QVariant> radarParameter<enumElem>::availableset_to_variant()
{
    QVector<QVariant> out(_availableset.count());
    for (int n=0; n<_availableset.count();n++)
        out[n] = QVariant::fromValue<enumElem>(_availableset[n]);

    return out;
}
//------------------------------------------------------
void  radarParameter<enumElem>::variant_to_value(QVector<QVariant>& data)
{
    int data_size = get_size() == RPT_SCALAR ? 1 : data.count();
    if (data.count()==0) data_size = 0;
    QVector<enumElem> pairs(data_size);
    for (int n=0; n < data_size; n++)
    {
        enumElem elem = data[n].value<enumElem>();
        if (!is_valid(elem))
        {
            _isValid = false;
            return;
        }
        pairs[n] = elem;
    }
    _isValid = true;
    _value = pairs;
}
//------------------------------------------------------
void  radarParameter<enumElem>::variant_to_value(const QVector<QVariant>& data)
{
    int data_size = get_size() == RPT_SCALAR ? 1 : data.count();
    if (data.count()==0) data_size = 0;
    QVector<enumElem> pairs(data_size);
    for (int n=0; n < data_size; n++)
    {
        enumElem elem = data[n].value<enumElem>();
        if (!is_valid(elem))
        {
            _isValid = false;
            return;
        }
        pairs[n] = elem;
    }
    _isValid = true;
    _value = pairs;
}
//------------------------------------------------------
void radarParameter<enumElem>::set_inquiry_value(const QByteArray& inquiry_val)
{
    _has_inquiry_value = inquiry_val.length()>0;

    _inquiry_value = inquiry_val;

    if (!_has_inquiry_value) return;
    if (inquiry_val.length()>(int)sizeof(enumElem::second_type))
        _inquiry_value.truncate(sizeof(enumElem::second_type));
    if (inquiry_val.length()<(int)sizeof(enumElem::second_type))
        _inquiry_value.append(sizeof(enumElem::second_type)-inquiry_val.length(),char(0x00));
}
//------------------------------------------------------

QByteArray   radarParameter<enumElem>::get_inquiry_value()
{
    return _inquiry_value;
}
//------------------------------------------------------
void radarParameter<enumElem>::variant_to_inquiry_value(const QVariant& data)
{
    QString str = data.toString();
    QByteArray inquiry_val = QByteArray::fromHex(str.toLatin1());

    _has_inquiry_value = inquiry_val.length()>0;

    _inquiry_value = inquiry_val;

    if (!_has_inquiry_value) return;
    if (inquiry_val.length()>(int)sizeof(enumElem::second_type))
        _inquiry_value.truncate(sizeof(enumElem::second_type));
    if (inquiry_val.length()<(int)sizeof(enumElem::second_type))
        _inquiry_value.append(sizeof(enumElem::second_type)-inquiry_val.length(),char(0x00));

}
//------------------------------------------------------
QVariant radarParameter<enumElem>::inquiry_value_to_variant()
{
    return QVariant(_inquiry_value.toHex(0));
}
//------------------------------------------------------
void  radarParameter<enumElem>::variant_to_availabeset(QVector<QVariant>& data)
{
    _availableset.resize(data.count());
    for (int n=0; n < data.count(); n++)
        _availableset[n] = data[n].value<enumElem>();
    _isValid = validate_data();
    if (data.count()>0)
        _hasMinMax = false;
}
//------------------------------------------------------
void  radarParameter<enumElem>::variant_to_availabeset(const QVector<QVariant>& data)
{
    _availableset.resize(data.count());
    for (int n=0; n < data.count(); n++)
        _availableset[n] = data[n].value<enumElem>();
    _isValid = validate_data();
    if (data.count()>0)
        _hasMinMax = false;
}

//------------------------------------------------------
void  radarParameter<enumElem>::value(enumElem val)
{
    _isValid = is_valid(val);
    if (!_isValid) return;
    _value.resize(1);
    _value[0] = val;
}
//------------------------------------------------------
int  radarParameter<enumElem>::get_index_of_value(QVariant value)
{
    enumElem val = value.value<enumElem>();

    for (int n=0; n<_availableset.count(); n++)
        if (_availableset[n]==val)
            return n;
    return -1;
}
//------------------------------------------------------
int  radarParameter<enumElem>::get_index_of_value(QString value)
{

    for (int n=0; n<_availableset.count(); n++)
        if (_availableset[n].first==value)
            return n;
    return -1;
}
//------------------------------------------------------
int  radarParameter<enumElem>::get_index_of_value(uint8_t value)
{

    for (int n=0; n<_availableset.count(); n++)
        if (_availableset[n].second==value)
            return n;
    return -1;
}


bool radarParameter<enumElem>::has_available_set()
{
    return _availableset.count()>0;
}
//------------------------------------------------------
bool    radarParameter<enumElem>::load_xml(QDomDocument& owner, QDomElement& param)
{
    bool bOk;


    QDomElement availset_elem = param.firstChildElement("available_set");
    if (!availset_elem.isNull())
    {
        int expected_size = availset_elem.attribute("size").toInt(&bOk);
        if (!bOk)
            return false;

        QStringList dataStream;
        dataStream = availset_elem.text().split("\n");
        dataStream.removeAll("");
        if (dataStream.count()!=2*expected_size)
            return false;
        _availableset.resize(expected_size);
        int index = 0;
        for (int n=0; n < expected_size; n++)
        {
            QString val_descr(dataStream[index++]);
            uint8_t val_id   (dataStream[index++].toInt(&bOk));
            if (val_descr.isEmpty()) return false;
            if (!bOk) return false;
            if ((val_id < 0)||(val_id >UINT8_MAX))
                return false;

            _availableset[n] = QPair<QString,uint8_t>(val_descr,val_id);
        }
    }
    else
        remove_available_set();

    _has_inquiry_value = false;

    QDomElement inquiry_val = param.firstChildElement("inquiry_value");
    if (!inquiry_val.isNull())
    {
        _has_inquiry_value = false;
    }
    else
    {
        QString value = inquiry_val.attribute("value","");
        variant_to_inquiry_value(QVariant(value));
    }


    QDomElement current_elem = param.firstChildElement("values");
    if (!current_elem.isNull())
    {

        int expected_size = current_elem.attribute("size").toInt(&bOk);
        if (!bOk)
            return false;

        QStringList dataStream = current_elem.text().split("\n");
        dataStream.removeAll("");
        if (dataStream.count()!=expected_size)
            return false;

        _value.resize(expected_size);

        for (int n=0; n < expected_size; n++)
        {
            QString val_descr(dataStream[n]);
            if (val_descr.isEmpty()) return false;
            if (!bOk) return false;

            int val_id = get_index_of_value(val_descr);
            if ((val_id < 0)||(val_id >UINT8_MAX))
                return false;

            _value[n] =  QPair<QString,uint8_t>(val_descr,val_id);
        }

    }
    else
        _value.clear();

    return true;
}


bool    radarParameter<enumElem>::save_xml(QDomDocument& owner, QDomElement& root)
{

    QDomElement element = owner.createElement("parameter");
    element.setAttribute("param_name",_paramname);
    element.setAttribute("param_type",typeIdToString(_rpt));
    element.setAttribute("param_io",  ioTypeToString(_rpt_io));
    element.setAttribute("param_group",_group);
    element.setAttribute("command_string",_command_string);
    element.setAttribute("size",strSizeToString(get_size()));

    if (is_linked_to_octave())
    {
        element.setAttribute("name_compound",_b_compound_name ? "true":"false");
        element.setAttribute("octave_alias",_var);
    }

    element.setAttribute("plotted",_b_plotted? QString("true"):QString("false"));

    if (_b_plotted)
        element.setAttribute("plot_type",plotTypeIdToString(_plottype));

    QDomElement availableset_elem = owner.createElement("available_set");
    availableset_elem.setAttribute("size",QString::number(_availableset.count()));

    QStringList availStream;
    for (int n=0; n < _availableset.count(); n++)
    {
        availStream << _availableset.at(n).first;
        availStream << QString::number(_availableset.at(n).second);

    }

    QDomText fText = owner.createTextNode(availStream.join("\n"));
    availableset_elem.appendChild(fText);

    element.appendChild(availableset_elem);

    if (_has_inquiry_value)
    {
        QDomElement inquiry_value = owner.createElement("inquiry_value");
        inquiry_value.setAttribute("value",QString(_inquiry_value.toHex(0)));
        element.appendChild(inquiry_value);
    }

    QDomElement current_set = owner.createElement("values");
    current_set.setAttribute("size",QString::number(_value.count()));

    QStringList valueStream;
    for (int n=0; n < _value.count(); n++)
        valueStream << _value.at(n).first;

    fText = owner.createTextNode(valueStream.join("\n"));
    current_set.appendChild(fText);

    element.appendChild(current_set);

    root.appendChild(element);

    return true;
}


QByteArray radarParameter<enumElem>::chain_values()
{
    QByteArray out;

    for (int n=0; n<_value.count();n++)
        out.append(_value[n].second);
//        out.append((_value[n].first+"\0").toUtf8());

    return out;
}
//------------------------------------------------------

bool radarParameter<enumElem>::split_values(const QByteArray& data)
{
    QVector<int> values(data.size());

    for (int n=0 ; n < data.size(); n++)
        values[n] = data[n];

    _value.clear();
    for (int n=0; n < values.size(); n++)
    {
        int nfound = get_pair(values[n]);
        if (nfound < 0)
            return false;

        _value.append(_availableset[nfound]);
    }
    return true;
}
//------------------------------------------------------
int radarParameter<enumElem>::get_pair(const QString& strValue)
{
    int nfound = -1;
    for (int m = 0; m < _availableset.size(); m++)
    {
        if (strValue==_availableset[m].first)
        {
            nfound = m; break;
        }
    }
    return nfound;

}

//------------------------------------------------------
int radarParameter<enumElem>::get_pair(int strIndex)
{
    int nfound = -1;
    for (int m = 0; m < _availableset.size(); m++)
    {
        if (strIndex==_availableset[m].second)
        {
            nfound = m; break;
        }
    }
    return nfound;

}
//------------------------------------------------------
void radarParameter<enumElem>::var_changed()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    Array<std::string> var_content = _workspace->var_value(_var.toStdString()).cellstr_value();

    for (int n=0; n < var_content.numel(); n++)
    {
        if (!is_valid(QString::fromStdString(var_content(n))))
            return;
    }
    _value.clear();
    _value.resize(var_content.numel());

    for (int n=0; n < var_content.numel(); n++)
    {
        enumElem pair;
        int nfound = get_pair(QString::fromStdString(var_content(n)));
        if (nfound < 0) nfound = 0;
        _value[n] = _availableset[nfound];
    }
}

unsigned int radarParameter<enumElem>::value_bytes_count()
{
    return _value.count();
}

int    radarParameter<enumElem>::get_expected_payload_size()
{
    if (is_scalar())
        return (int)(sizeof(char));

    return -1;
}

//------------------------------------------------------
// String type
//------------------------------------------------------

radarParameter<QString>&      radarParameter<QString>::operator = (radarParameter<QString>& t2)
{
    radarParamBase::operator=(t2);
    _value       = t2._value;
    _availableset       = t2._availableset;
    return *this;

}
//------------------------------------------------------
radarParameter<QString>&      radarParameter<QString>::operator = (const radarParameter<QString>& t2)
{
    radarParamBase::operator=(t2);
    _value       = t2._value;
    _availableset       = t2._availableset;
    return *this;
}
//------------------------------------------------------
bool    radarParameter<QString>::is_valid(QString val)
{
    if (has_available_set())
    {
        for (int n=0; n < _availableset.count(); n++)
        {
            if (val == _availableset[n])
                return true;
        }
        return false;
    }

    if (_hasMinMax)
        return ((val>=_min)&&(val<=_max));

    return true;
}
//------------------------------------------------------
bool radarParameter<QString>::is_valid(QVariant val)
{
    return is_valid(val.toString());
}
//------------------------------------------------------
bool radarParameter<QString>::is_scalar()
{
    return _value.count()==1;
}

//------------------------------------------------------
const QVector<QString>&     radarParameter<QString>::value()
{
    return _value;
}
//------------------------------------------------------
void    radarParameter<QString>::value(const QString& val)
{
    _value.resize(1);
    _value[0] = val;
}
//------------------------------------------------------
void    radarParameter<QString>::value(QString& val)
{
    _value.resize(1);
    _value[0] = val;
}
//------------------------------------------------------
void    radarParameter<QString>::value(QVector<QString>& val)
{
    int data_size = get_size() == RPT_SCALAR ? 1 : val.count();
    if (val.count()==0) data_size = 0;
    _value.resize(data_size);

    if (data_size == val.count())
        _value = val;
    else
        _value[0] = val[0];
}
//------------------------------------------------------
void  radarParameter<QString>::set_min_max(QString min, QString max)
{
    _min = min;
    _max = max;
    _hasMinMax = true;
    _availableset.clear();
}
//------------------------------------------------------
void  radarParameter<QString>::set_min_max(QVariant min, QVariant max)
{
    _min = min.toString();
    _max = max.toString();
    _hasMinMax = true;
    _availableset.clear();
}
//------------------------------------------------------
void  radarParameter<QString>::remove_min_max()
{
    _hasMinMax = false;
}
//------------------------------------------------------
void  radarParameter<QString>::remove_available_set()
{
    _availableset.clear();
}
//------------------------------------------------------
void  radarParameter<QString>::set_list_available_values(QVector<QString> valList)
{
    _availableset = valList;
    if (_availableset.count()>0)
        _hasMinMax = false;
}

//------------------------------------------------------
QVector<QVariant>  radarParameter<QString>::value_to_variant()
{
    QVector<QVariant> out(_value.count());
    for (int n=0; n < _value.count(); n++)
        out[n] = _value[n];
    return out;
}
//------------------------------------------------------
QVector<QVariant>  radarParameter<QString>::availableset_to_variant()
{
    QVector<QVariant> out(_availableset.count());
    for (int n=0; n < _availableset.count(); n++)
        out[n] = _availableset[n];
    return out;
}
//------------------------------------------------------
int radarParameter<QString>::get_expected_payload_size()
{
    int ncount = 0;
    for (int n=0; n < _value.count(); n++)
        ncount += _value[n].length();
    return ncount;
}
//------------------------------------------------------
void radarParameter<QString>::variant_to_value(QVector<QVariant>& data)
{
    int data_size = get_size() == RPT_SCALAR ? 1 : data.count();
    if (data.count()==0) data_size = 0;
    _value.resize(data_size);
    for (int n=0; n < data_size; n++)
        _value[n] = data[n].toString();
    return;
}
//------------------------------------------------------
void radarParameter<QString>::variant_to_value(const QVector<QVariant>& data)
{
    int data_size = get_size() == RPT_SCALAR ? 1 : data.count();
    if (data.count()==0) data_size = 0;
    _value.resize(data_size);
    for (int n=0; n < data_size; n++)
        _value[n] = data[n].toString();
    return;
}

//------------------------------------------------------
void radarParameter<QString>::variant_to_availabeset(QVector<QVariant>& data)
{
    _availableset.resize(data.count());
    for (int n=0; n < data.count(); n++)
        _availableset[n] = data[n].toString();

    if (_availableset.count()>0)
        _hasMinMax = false;
    return;
}
//------------------------------------------------------
void radarParameter<QString>::set_inquiry_value(const QByteArray& inquiry_val)
{
    _has_inquiry_value = inquiry_val.length()>0;

    _inquiry_value = inquiry_val;
}
//------------------------------------------------------
QByteArray radarParameter<QString>::get_inquiry_value()
{
    return _inquiry_value;
}
//------------------------------------------------------
void    radarParameter<QString>::variant_to_inquiry_value(const QVariant& data)
{
    QString str = data.toString();
    QByteArray inquiry_val = QByteArray::fromHex(str.toLatin1());

    _has_inquiry_value = inquiry_val.length()>0;

    _inquiry_value = inquiry_val;
}
//------------------------------------------------------
QVariant radarParameter<QString>::inquiry_value_to_variant()
{
    return QVariant(_inquiry_value);
}

//------------------------------------------------------
void radarParameter<QString>::variant_to_availabeset(const QVector<QVariant>& data)
{
    _availableset.resize(data.count());
    for (int n=0; n < data.count(); n++)
        _availableset[n] = data[n].toString();

    if (_availableset.count()>0)
        _hasMinMax = false;

    return;
}
//------------------------------------------------------
QString radarParameter<QString>::get_min_string()
{
    return _min;
}
//------------------------------------------------------
QString radarParameter<QString>::get_max_string()
{
    return _max;
}
//------------------------------------------------------
bool radarParameter<QString>::has_available_set()
{
    return _availableset.count()>0;
}
//------------------------------------------------------
bool radarParameter<QString>::set_min_string(QString strval)
{
    _min = strval;
    _hasMinMax = true;
    _availableset.clear();

    return validate_data();
}
//------------------------------------------------------
bool radarParameter<QString>::set_max_string(QString strval)
{
    _max = strval;
    _hasMinMax = true;
    _availableset.clear();
    return validate_data();
}
//------------------------------------------------------
bool radarParameter<QString>::validate_string(QString strval)
{
    return is_valid(strval);
}
//------------------------------------------------------
int  radarParameter<QString>::get_index_of_value(QVariant value)
{
    QString val = value.toString();
    for (int n=0; n<_availableset.count(); n++)
        if (_availableset[n]==val)
            return n;
    return -1;
}
//------------------------------------------------------
bool radarParameter<QString>::validate_data()
{
    for (int n=0; n < _value.count(); n++)
        if (!is_valid(_value[n]))
            return false;

    return true;
}


//------------------------------------------------------
bool    radarParameter<QString>::load_xml(QDomDocument& owner, QDomElement& param)
{
    QDomElement minmaxElem = param.firstChildElement("min_max");
    if (!minmaxElem.isNull())
    {
        set_min_string(param.attribute("min"));
        set_max_string(param.attribute("max"));
    }

    bool bOk;
    QDomElement availset_elem = param.firstChildElement("available_set");
    if (!availset_elem.isNull())
    {
        int expected_size = availset_elem.attribute("size").toInt(&bOk);
        if (!bOk)
            return false;

        QStringList dataStream;
        dataStream = availset_elem.text().split("\n");
        dataStream.removeAll("");
        if (dataStream.count()!=expected_size)
            return false;
        _availableset.resize(expected_size);

        for (int n=0; n < dataStream.count(); n++)
            _availableset[n] = dataStream[n];
    }
    else
        remove_available_set();

    QDomElement inquiry_value = param.firstChildElement("inquiry_value");
    if (inquiry_value.isNull())
    {
        _has_inquiry_value = false;
    }
    else
    {
        QString val = inquiry_value.attribute("value","");
        variant_to_inquiry_value(QVariant(val));

    }


    QDomElement current_elem = param.firstChildElement("values");
    if (current_elem.isNull())
    {
        _value.clear();
    }
    else
    {
        int expected_size = current_elem.attribute("size").toInt(&bOk);
        if (!bOk)
            return false;

        QStringList dataStream = current_elem.text().split("\n");
        dataStream.removeAll("");
        if (dataStream.count()!=expected_size)
            return false;
    }

    return true;
}

//------------------------------------------------------
bool    radarParameter<QString>::save_xml(QDomDocument& owner, QDomElement& root)
{

    QDomElement element = owner.createElement("parameter");
    element.setAttribute("param_name",_paramname);
    element.setAttribute("param_type",typeIdToString(_rpt));
    element.setAttribute("param_io",  ioTypeToString(_rpt_io));
    element.setAttribute("param_group",_group);
    element.setAttribute("command_string",_command_string);
    element.setAttribute("size",strSizeToString(get_size()));

    if (is_linked_to_octave())
    {
        element.setAttribute("name_compound",_b_compound_name ? "true":"false");
        element.setAttribute("octave_alias",_var);
    }

    element.setAttribute("plotted",_b_plotted? QString("true"):QString("false"));
    if (_b_plotted)
        element.setAttribute("plot_type",plotTypeIdToString(_plottype));


    QDomElement availableset_elem = owner.createElement("available_set");
    availableset_elem.setAttribute("size",QString::number(_availableset.count()));

    QStringList availStream;
    for (int n=0; n < _availableset.count(); n++)
        availStream << _availableset.at(n);

    QDomText fText = owner.createTextNode(availStream.join("\n"));
    availableset_elem.appendChild(fText);

    element.appendChild(availableset_elem);

    if (_has_inquiry_value)
    {
        QDomElement inquiry_value = owner.createElement("inquiry_value");
        inquiry_value.setAttribute("value",QString(_inquiry_value.toHex(0)));
        element.appendChild(inquiry_value);
    }

    QDomElement current_set = owner.createElement("values");
    current_set.setAttribute("size",QString::number(_value.count()));

    QStringList valueStream;
    for (int n=0; n < _value.count(); n++)
        valueStream << _value.at(n);

    fText = owner.createTextNode(valueStream.join("\n"));
    current_set.appendChild(fText);

    element.appendChild(current_set);

    root.appendChild(element);

    return true;
}

//------------------------------------------------------
QByteArray radarParameter<QString>::chain_values()
{
    QByteArray out;
    for (int n=0; n<_value.count(); n++)
        out.append( (_value[n]+"\0").toUtf8());
    return out;
}
//------------------------------------------------------
bool radarParameter<QString>::split_values(const QByteArray& data)
{
    _value.clear();
    QString str(data);

    _value.fromList(str.split("\0",Qt::SkipEmptyParts));

    return true;
}
//------------------------------------------------------
void radarParameter<QString>::var_changed()
{
    if (_workspace==nullptr) return;
    if (_var.isEmpty()) return;

    Array<std::string> sv = _workspace->var_value(_var.toStdString()).cellstr_value();

    for (int n=0; n < sv.numel(); n++)
    {
        if (!is_valid(QString::fromStdString(sv(n))))
            return;
    }
    _value.clear(); _value.resize(sv.numel());
    for (int n=0; n < sv.numel(); n++)
        _value[n] = QString::fromStdString(sv(n));
}
//------------------------------------------------------
void radarParameter<QString>::update_variable()
{
    if (_var.isEmpty()) return;
    if (_workspace==nullptr) return;

    Array<std::string> sv(dim_vector(_value.count(),1));
    for (int n=0; n < _value.count(); n++)
        sv(n) = _value[n].toStdString();
    _workspace->add_variable(_var.toStdString(), false, octave_value(sv));
}
//------------------------------------------------------
unsigned int radarParameter<QString>::value_bytes_count()
{
    unsigned int totsize=0;
    const QVector<QString>& currvalarray = _value;

    for (const auto& curr : currvalarray)
        totsize += curr.length();
    return totsize;
}
//------------------------------------------------------
void radarParameter<QString>::convert_data(const octave_value& in)
{
    if (in.iscellstr())
    {
        Array<std::string> in_array = in.cellstr_value();
        _value.resize(in.numel());
        for (int n=0; n < in.numel(); n++)
            _value[n] = QString::fromStdString(in_array(n));

        return;
    }
    if (in.is_char_matrix())
    {
        int ns = in.dims()(0);
        int nc = in.dims()(1);
        _value.resize(ns);
        charNDArray in_array = in.char_array_value();
        for (int n=0; n < ns; n++)
        {
            _value[n].resize(nc);
            for (int c=0; c < nc; c++)
                _value[n][c] = in_array(n,c);
        }

        return;
    }
}
//------------------------------------------------------
void radarParameter<enumElem>::convert_data(const octave_value& in)
{
    if (in.is_uint8_type())
    {
        uint8NDArray in_array = in.uint8_array_value();
        _value.resize(in.numel());
        const auto& avail = _availableset;
        for (int n=0; n < in.numel(); n++)
        {
            uint8_t val = (uint8_t)in_array(n);
            for (const auto& check : avail)
            {
                if (check.second==val)
                {
                    _value[n] = enumElem(check.first, val);
                    break;
                }
            }
        }
        return;
    }

    if (in.is_int8_type())
    {
        int8NDArray in_array = in.int8_array_value();
        _value.resize(in.numel());
        const auto& avail = _availableset;
        for (int n=0; n < in.numel(); n++)
        {
            uint8_t val = (uint8_t)in_array(n);
            for (const auto& check : avail)
            {
                if (check.second==val)
                {
                    _value[n] = enumElem(check.first, val);
                    break;
                }
            }
        }
        return;
    }
    if (in.is_uint16_type())
    {
        uint16NDArray in_array = in.uint16_array_value();
        _value.resize(in.numel());
        const auto& avail = _availableset;
        for (int n=0; n < in.numel(); n++)
        {
            uint8_t val = (uint8_t)in_array(n);
            for (const auto& check : avail)
            {
                if (check.second==val)
                {
                    _value[n] = enumElem(check.first, val);
                    break;
                }
            }
        }
        return;
    }

    if (in.is_int16_type())
    {
        int16NDArray in_array = in.int16_array_value();
        _value.resize(in.numel());
        const auto& avail = _availableset;
        for (int n=0; n < in.numel(); n++)
        {
            uint8_t val = (uint8_t)in_array(n);
            for (const auto& check : avail)
            {
                if (check.second==val)
                {
                    _value[n] = enumElem(check.first, val);
                    break;
                }
            }
        }
        return;
    }
    if (in.is_uint32_type())
    {
        uint32NDArray in_array = in.uint32_array_value();
        _value.resize(in.numel());
        const auto& avail = _availableset;
        for (int n=0; n < in.numel(); n++)
        {
            uint8_t val = (uint8_t)in_array(n);
            for (const auto& check : avail)
            {
                if (check.second==val)
                {
                    _value[n] = enumElem(check.first, val);
                    break;
                }
            }
        }
        return;
    }

    if (in.is_int32_type())
    {
        int32NDArray in_array = in.int32_array_value();
        _value.resize(in.numel());
        const auto& avail = _availableset;
        for (int n=0; n < in.numel(); n++)
        {
            uint8_t val = (uint8_t)in_array(n);
            for (const auto& check : avail)
            {
                if (check.second==val)
                {
                    _value[n] = enumElem(check.first, val);
                    break;
                }
            }
        }
        return;
    }
    if (in.is_char_matrix())
    {
        charNDArray in_array = in.char_array_value();
        _value.resize(in.numel());
        const auto& avail = _availableset;
        for (int n=0; n < in.numel(); n++)
        {
            uint8_t val = (uint8_t)in_array(n);
            for (const auto& check : avail)
            {
                if (check.second==val)
                {
                    _value[n] = enumElem(check.first, val);
                    break;
                }
            }
        }
        return;
    }

    if (in.isfloat())
    {
        FloatNDArray in_array = in.float_array_value();
        _value.resize(in.numel());
        const auto& avail = _availableset;
        for (int n=0; n < in.numel(); n++)
        {
            uint8_t val = (uint8_t)(round(in_array(n)));
            for (const auto& check : avail)
            {
                if (check.second==val)
                {
                    _value[n] = enumElem(check.first, val);
                    break;
                }
            }
        }
        return;
    }
    if (in.is_double_type())
    {
        NDArray in_array = in.array_value();
        _value.resize(in.numel());
        const auto& avail = _availableset;
        for (int n=0; n < in.numel(); n++)
        {
            uint8_t val = (uint8_t)(round(in_array(n)));
            for (const auto& check : avail)
            {
                if (check.second==val)
                {
                    _value[n] = enumElem(check.first, val);
                    break;
                }
            }
        }
        return;
    }
}

template<> radarParameter<int8_t>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_INT8), _value(dim_vector({0,0})){}
template<> radarParameter<uint8_t>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_UINT8), _value(dim_vector({0,0})) {}
template<> radarParameter<int16_t>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_INT16),  _value(dim_vector({0,0})){}
template<> radarParameter<uint16_t>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_UINT16),  _value(dim_vector({0,0})) {}
template<> radarParameter<int32_t>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_INT32),  _value(dim_vector({0,0})) {}
template<> radarParameter<uint32_t>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_UINT32),  _value(dim_vector({0,0})) {}
template<> radarParameter<float>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_FLOAT), _value(dim_vector({0,0})) {}
template<> radarParameter<char>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_UINT8), _value(dim_vector({0,0})) {}

template<> radarParameter<int8_t>::radarParameter() : radarParamBase("noname", RPT_INT8), _value(dim_vector({0,0})){}
template<> radarParameter<uint8_t>::radarParameter() : radarParamBase("noname", RPT_UINT8), _value(dim_vector({0,0})) {}
template<> radarParameter<int16_t>::radarParameter() : radarParamBase("noname", RPT_INT16),  _value(dim_vector({0,0})){}
template<> radarParameter<uint16_t>::radarParameter() : radarParamBase("noname", RPT_UINT16),  _value(dim_vector({0,0})) {}
template<> radarParameter<int32_t>::radarParameter() : radarParamBase("noname", RPT_INT32),  _value(dim_vector({0,0})) {}
template<> radarParameter<uint32_t>::radarParameter() : radarParamBase("noname", RPT_UINT32),  _value(dim_vector({0,0})) {}
template<> radarParameter<float>::radarParameter(): radarParamBase("noname", RPT_FLOAT), _value(dim_vector({0,0})) {}
template<> radarParameter<char>::radarParameter() : radarParamBase("noname", RPT_UINT8), _value(dim_vector({0,0})) {}
//------------------------------------------------------
template<> octave_value radarParameter<int8_t>::get_value()
{
    return octave_value( (Array<octave_int8>)_value);
}
//------------------------------------------------------
template<> void         radarParameter<int8_t>::set_value(const octave_value& val )
{
    //_value = val.int8_array_value();
    convert_data(val);
}
//------------------------------------------------------
template<> octave_value radarParameter<uint8_t>::get_value()
{
    return octave_value( (Array<octave_uint8>)_value);
}
//------------------------------------------------------
template<> void         radarParameter<uint8_t>::set_value(const octave_value& val )
{
    //_value = val.uint8_array_value();
    convert_data(val);
}

//------------------------------------------------------
template<> octave_value radarParameter<int16_t>::get_value()
{
    return octave_value( (Array<octave_int16>)_value);
}
//------------------------------------------------------
template<> void         radarParameter<int16_t>::set_value(const octave_value& val )
{
    //_value = val.int16_array_value();
    convert_data(val);
}
//------------------------------------------------------
template<> octave_value radarParameter<uint16_t>::get_value()
{
    return octave_value( (Array<octave_uint16>)_value);
}
//------------------------------------------------------
template<> void         radarParameter<uint16_t>::set_value(const octave_value& val )
{
    //_value = val.uint16_array_value();
    convert_data(val);
}
//------------------------------------------------------
template<> octave_value radarParameter<int32_t>::get_value()
{
    return octave_value( (Array<octave_int32>)_value);
}
//------------------------------------------------------
template<> void         radarParameter<int32_t>::set_value(const octave_value& val )
{
    //_value = val.int32_array_value();
    convert_data(val);
}
//------------------------------------------------------
template<> octave_value radarParameter<uint32_t>::get_value()
{
    return octave_value( (Array<octave_uint32>)_value);
}
//------------------------------------------------------
template<> void         radarParameter<uint32_t>::set_value(const octave_value& val )
{
    //_value = val.uint32_array_value();
    convert_data(val);
}

//------------------------------------------------------
template<> octave_value radarParameter<char>::get_value()
{
    return octave_value( (Array<char>)_value);
}
//------------------------------------------------------
template<> void         radarParameter<char>::set_value(const octave_value& val )
{
    //_value = val.char_array_value(true);
    convert_data(val);
}

//------------------------------------------------------
template<> octave_value radarParameter<float>::get_value()
{
    return octave_value( (Array<float>)_value);
}
//------------------------------------------------------
template<> void         radarParameter<float>::set_value(const octave_value& val )
{
    //_value = val.float_array_value(true);
    convert_data(val);
}

//------------------------------------------------------
octave_value radarParameter<QString>::get_value()
{
    Array<std::string> out; out.resize(dim_vector({1,_value.count()}));
    int n=0;
    const auto& x = _value;
    for (const auto& str : x)
        out(n++) = str.toStdString();

    return octave_value(out);
}
//------------------------------------------------------
void         radarParameter<QString>::set_value(const octave_value& val )
{
    convert_data(val);
}


//------------------------------------------------------
bool radarParameter<QString>::operator == (const radarParameter<QString>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (_value!=param._value) return false;
    if (_min != param._min) return false;
    if (_max != param._max) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (_availableset!=param._availableset) return false;
    return true;
}
//------------------------------------------------------
QVariant radarParameter<QString>::get_min()
{
    return QVariant(_min);
}
//------------------------------------------------------
QVariant radarParameter<QString>::get_max()
{
    return QVariant(_max);
}
//------------------------------------------------------
octave_value radarParameter<enumElem>::get_value()
{
    uint8NDArray out; out.resize(dim_vector({1,_value.count()}));
    int n=0;
    const auto& x = _value;
    for (const auto& str : x)
        out(n++) = str.second;

    return octave_value(out);
}
//------------------------------------------------------
void         radarParameter<enumElem>::set_value(const octave_value& val )
{
    convert_data(val);
}

//------------------------------------------------------
bool radarParameter<enumElem>::operator == (const radarParameter<enumElem>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (_value!=param._value) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (_availableset!=param._availableset) return false;
    return true;
}
//------------------------------------------------------
octave_value radarParameter<void>::get_value()
{
    return octave_value();
}
//------------------------------------------------------
void radarParameter<void>::convert_data(const octave_value& val)
{

}
//------------------------------------------------------
void        radarParameter<void>::set_value(const octave_value& val )
{ }
//------------------------------------------------------
bool         radarParameter<void>::operator == (const radarParameter<void>& param)
{
    return radarParamBase::operator==(param);
}
//------------------------------------------------------
template<> bool radarParameter<int8_t>::split_values(const QByteArray& data)
{
    _value.clear();
    int nbytes = data.length();
    _value.resize(dim_vector(1,nbytes));
    for (int n=0; n < nbytes; n++)
        _value.elem(n) = (int8_t)data[n];

    return true;
}

template<> bool radarParameter<char>::split_values(const QByteArray& data)
{
    _value.clear();
    int nbytes = data.length();
    _value.resize(dim_vector(1,nbytes));
    for (int n=0; n < nbytes; n++)
        _value.elem(n) = (char)data[n];

    return true;
}

template<> bool radarParameter<uint8_t>::split_values(const QByteArray& data)
{
    _value.clear();
    int nbytes = data.length();
    _value.resize(dim_vector(1,nbytes));
    for (int n=0; n < nbytes; n++)
        _value.elem(n) = (uint8_t)data[n];
    return true;
}



template<> bool radarParameter<uint16_t>::split_values(const QByteArray& data)
{
    _value.clear();
    div_t res = std::div((int)data.length(), (int)(2));
    if (res.rem!=0) return false;
    _value.resize(dim_vector(1,res.quot));
    for (int n=0, p=0; n < res.quot; n++, p+=2)
    {
        uint16_t dest;
        char* _pdest = (char*)(&dest);
        memcpy(_pdest,data.mid(p, 2).data(),2);
        _value.elem(n) = dest;
    }
    return true;
}


template<> bool radarParameter<int16_t>::split_values(const QByteArray& data)
{
    _value.clear();
    div_t res = std::div((int)data.length(), 2);
    if (res.rem!=0) return false;
    _value.resize(dim_vector(1,res.quot));
    for (int n=0, p=0; n < res.quot; n++, p+=2)
    {
        int16_t dest;
        char* _pdest = (char*)(&dest);
        memcpy(_pdest,data.mid(p, 2).data(),2);
        _value.elem(n) = dest;
    }
    return true;
}

template<> bool radarParameter<int32_t>::split_values(const QByteArray& data)
{
    _value.clear();
    div_t res = std::div((int)data.length(), 4);
    if (res.rem!=0) return false;
    _value.resize(dim_vector(1,res.quot));
    for (int n=0, p=0; n < res.quot; n++, p+=4)
    {
        int32_t dest;
        char* _pdest = (char*)(&dest);
        memcpy(_pdest,data.mid(p, 4).data(),4);
        _value.elem(n) = dest;
    }
    return true;
}


template<> bool radarParameter<uint32_t>::split_values(const QByteArray& data)
{
    _value.clear();
    div_t res = std::div((int)data.length(), 4);
    if (res.rem!=0) return false;
    _value.resize(dim_vector(1,res.quot));
    for (int n=0, p=0; n < res.quot; n++, p+=4)
    {
        uint32_t dest;
        char* _pdest = (char*)(&dest);
        memcpy(_pdest,data.mid(p, 4).data(),4);
        _value.elem(n) = dest;
    }
    return true;
}
//------------------------------------------------------
template<> bool radarParameter<char>::operator == (const radarParameter<char>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (!is_equal(charNDArray(_value), charNDArray(param._value))) return false;
    if (_min != param._min) return false;
    if (_max != param._max) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (!is_equal((charNDArray)_availableset, (charNDArray)param._availableset)) return false;
    return true;
}
//------------------------------------------------------
template<> bool radarParameter<int8_t>::operator == (const radarParameter<int8_t>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (!is_equal((int8NDArray)_value, (int8NDArray)param._value)) return false;
    if (_min != param._min) return false;
    if (_max != param._max) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (!is_equal((int8NDArray)_availableset, (int8NDArray)param._availableset)) return false;
    return true;
}
//------------------------------------------------------
template<> bool radarParameter<uint8_t>::operator == (const radarParameter<uint8_t>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (!is_equal((uint8NDArray)_value, (uint8NDArray)param._value)) return false;
    if (_min != param._min) return false;
    if (_max != param._max) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (!is_equal((uint8NDArray)_availableset, (uint8NDArray)param._availableset)) return false;
    return true;
}
//------------------------------------------------------
template<> bool radarParameter<int16_t>::operator == (const radarParameter<int16_t>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (!is_equal((int16NDArray)_value, (int16NDArray)param._value)) return false;
    if (_min != param._min) return false;
    if (_max != param._max) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (!is_equal((int16NDArray)_availableset, (int16NDArray)param._availableset)) return false;
    return true;
}
//------------------------------------------------------
template<> bool radarParameter<uint16_t>::operator == (const radarParameter<uint16_t>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (!is_equal((uint16NDArray)_value, (uint16NDArray)param._value)) return false;
    if (_min != param._min) return false;
    if (_max != param._max) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (!is_equal((uint16NDArray)_availableset, (uint16NDArray)param._availableset)) return false;
    return true;
}
//------------------------------------------------------
template<> bool radarParameter<int32_t>::operator == (const radarParameter<int32_t>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (!is_equal((int32NDArray)_value, (int32NDArray)param._value)) return false;
    if (_min != param._min) return false;
    if (_max != param._max) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (!is_equal((int32NDArray)_availableset, (int32NDArray)param._availableset)) return false;
    return true;
}
//------------------------------------------------------
template<> bool radarParameter<uint32_t>::operator == (const radarParameter<uint32_t>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (!is_equal((uint32NDArray)_value, (uint32NDArray)param._value)) return false;
    if (_min != param._min) return false;
    if (_max != param._max) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (!is_equal((uint32NDArray)_availableset, (uint32NDArray)param._availableset)) return false;
    return true;
}

//------------------------------------------------------
template<> bool radarParameter<float>::operator == (const radarParameter<float>& param)
{
    if (!radarParamBase::operator==(param)) return false;
    if (!is_equal((FloatNDArray)_value, (FloatNDArray)param._value)) return false;
    if (_min != param._min) return false;
    if (_max != param._max) return false;
    if (_inquiry_value != param._inquiry_value) return false;
    if (!is_equal((FloatNDArray)_availableset, (FloatNDArray)param._availableset)) return false;
    return true;
}
//------------------------------------------------------
template<> bool radarParameter<float>::split_values(const QByteArray& data)
{
    _value.clear();
    div_t res = std::div((int)data.length(), sizeof(float));
    if (res.rem!=0) return false;
    _value.resize(dim_vector(1,res.quot));
    for (int n=0, p=0; n < res.quot; n++, p+=sizeof(float))
    {
        float dest;
        char* _pdest = (char*)(&dest);
        memcpy(_pdest,data.mid(p, sizeof(float)).data(),sizeof(float));
        _value.elem(n) = dest;
    }
    return true;
}
//template<> radarParameter<enumElem>::radarParameter(QString paramName) : radarParamBase(paramName, RPT_ENUM, RPT_SCALAR) {_hasMinMax = false;}
//------------------------------------------------------
template<> void radarParameter<int8_t>::var_changed()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    int8NDArray var_value = _workspace->var_value(_var.toStdString()).int8_array_value();

    for (int n=0; n < var_value.numel(); n++)
        if (!is_valid(var_value(n))) return;

    _value = var_value;
}

//------------------------------------------------------
template<> void radarParameter<int8_t>::update_variable()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    _workspace->add_variable(_var.toStdString(), false, octave_value((Array<octave_int8>(_value))));
}

//------------------------------------------------------
template<> void radarParameter<uint8_t>::var_changed()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;
    uint8NDArray var_value = _workspace->var_value(_var.toStdString()).uint8_array_value();

    for (int n=0; n < var_value.numel(); n++)
        if (!is_valid(var_value(n))) return;

    _value = var_value;
}

//------------------------------------------------------
template<> void radarParameter<uint8_t>::update_variable()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    _workspace->add_variable(_var.toStdString(), false, octave_value((Array<octave_uint8>(_value))));
}

//------------------------------------------------------

template<> void radarParameter<int16_t>::var_changed()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;
    int16NDArray var_value = _workspace->var_value(_var.toStdString()).int16_array_value();

    for (int n=0; n < var_value.numel(); n++)
        if (!is_valid(var_value(n))) return;

    _value = var_value;
}
//------------------------------------------------------
template<> void radarParameter<int16_t>::update_variable()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    _workspace->add_variable(_var.toStdString(), false, octave_value((Array<octave_int16>(_value))));
}

//------------------------------------------------------

template<> void radarParameter<uint16_t>::var_changed()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;
    uint16NDArray var_value = _workspace->var_value(_var.toStdString()).uint16_array_value();

    for (int n=0; n < var_value.numel(); n++)
        if (!is_valid(var_value(n))) return;

    _value = var_value;
}
//------------------------------------------------------
template<> void radarParameter<uint16_t>::update_variable()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    _workspace->add_variable(_var.toStdString(), false, octave_value((Array<octave_uint16>(_value))));
}

//------------------------------------------------------


template<> void radarParameter<int32_t>::var_changed()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;
    int32NDArray var_value = _workspace->var_value(_var.toStdString()).int32_array_value();

    for (int n=0; n < var_value.numel(); n++)
        if (!is_valid(var_value(n))) return;

    _value = var_value;
}
//------------------------------------------------------
template<> void radarParameter<int32_t>::update_variable()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    _workspace->add_variable(_var.toStdString(), false, octave_value((Array<octave_int32>(_value))));
}

//------------------------------------------------------

template<> void radarParameter<uint32_t>::var_changed()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;
    uint32NDArray var_value = _workspace->var_value(_var.toStdString()).uint32_array_value();

    for (int n=0; n < var_value.numel(); n++)
        if (!is_valid(var_value(n))) return;

    _value = var_value;
}

//------------------------------------------------------
template<> void radarParameter<uint32_t>::update_variable()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    _workspace->add_variable(_var.toStdString(), false, octave_value((Array<octave_uint32>(_value))));
}

//------------------------------------------------------

template<> void radarParameter<float>::var_changed()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;
    FloatNDArray var_value = _workspace->var_value(_var.toStdString()).float_array_value();

    for (int n=0; n < var_value.numel(); n++)
        if (!is_valid(var_value(n))) return;

    _value = var_value;
}
//------------------------------------------------------
template<> void radarParameter<float>::update_variable()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    _workspace->add_variable(_var.toStdString(), false, octave_value(_value));
}

//------------------------------------------------------


template<> void radarParameter<char>::var_changed()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;
    charNDArray var_value = _workspace->var_value(_var.toStdString()).char_array_value();

    for (int n=0; n < var_value.numel(); n++)
        if (!is_valid(var_value(n))) return;

    _value = var_value;
}
//------------------------------------------------------
template<> void radarParameter<char>::update_variable()
{
    if (_workspace == nullptr) return;
    if (_var.isEmpty()) return;

    _workspace->add_variable(_var.toStdString(), false, octave_value((Array<char>(_value))));
}

//------------------------------------------------------

radarParameter<void>&      radarParameter<void>::operator = (radarParameter<void>& t2)
{
    radarParamBase::operator=(t2);
    return *this;
}

//------------------------------------------------------
radarParameter<void>&      radarParameter<void>::operator = (const radarParameter<void>& t2)
{
    radarParamBase::operator=(t2);
    return *this;
}

//------------------------------------------------------
bool    radarParameter<void>::save_xml(QDomDocument& owner, QDomElement& root)
{
    QDomElement element = owner.createElement("parameter");
    element.setAttribute("param_name",_paramname);
    element.setAttribute("param_type",typeIdToString(_rpt));
    element.setAttribute("param_io",  ioTypeToString(_rpt_io));
    element.setAttribute("param_group",_group);
    element.setAttribute("command_string",_command_string);
    element.setAttribute("size",strSizeToString(get_size()));

    if (is_linked_to_octave())
    {
        element.setAttribute("name_compound",_b_compound_name ? "true":"false");
        element.setAttribute("octave_alias",_var);
    }

    element.setAttribute("plotted",_b_plotted?QString("true"):QString("false"));
    if (_b_plotted)
        element.setAttribute("plot_type",plotTypeIdToString(_plottype));

    root.appendChild(element);
    return true;
}

//------------------------------------------------------
bool    radarParameter<void>::load_xml(QDomDocument& owner, QDomElement& root)
{
    return true;
}


//------------------------------------------------------
QByteArray radarParameter<void>::chain_values()
{
    return QByteArray();
}

//------------------------------------------------------
bool    radarParameter<void>::split_values(const QByteArray& data)
{
    return true;
}

//------------------------------------------------------
void       radarParameter<void>::var_changed()
{

}
