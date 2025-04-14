/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "radarinstance.h"
#include "qserialportinfo.h"
#include "radarmodule.h"
#include "radarproject.h"
#include <octave.h>
#include <interpreter.h>
#define noop (void)0

#undef SERIAL_CHECK_TX
#undef UPDATE_INSIDE

radarInstance::radarInstance(radarModule* module) : radarModule("newRadar", DT_RADARDEVICE) ,
    _protocol(nullptr),
    _ref_module(module),
    _map_octws_param_names(),
    _workspace(nullptr),
    _radar_uid(QByteArray::fromHex("ffffffff")),
#ifdef INTERFACE_STRUCT
    _radar_cell(),
#endif
    _serial_status(SS_IDLE),
    _serialport(nullptr),
    _serial_timer(nullptr),
    _n_params_to_receive(0)
{
    if (_ref_module != nullptr)
    {
        radarModule::operator=(*_ref_module); // Params are cloned here
        set_inquiry_instanceid_command(_ref_module->get_inquiry_instanceid_command());
        set_inquiry_moduleid_command(_ref_module->get_inquiry_moduleid_command());
        set_inquiry_moduleid_expectedvalue(_ref_module->get_inquiry_moduleid_expectedvalue());
        _protocol = _ref_module->get_comm_protocol();
    }
    _device_name ="";
    _expected_portname ="";
}
//-----------------------------------------------
radarInstance::radarInstance(radarInstance& radar) : radarModule(radar.get_uid().toHex(), DT_RADARDEVICE)
{
    _serialport = nullptr;
    _serial_timer= nullptr;

    operator = (radar);
}
//-----------------------------------------------
radarInstance& radarInstance::operator = (radarInstance& radar)
{
    radarModule::operator=(radar);
    _workspace = radar._workspace;
    _protocol= radar._protocol;
    _ref_module= radar._ref_module;
    _workspace = radar._workspace;

    if (_serialport==nullptr)
        initSerialPortTimer();
    if ((_serialport != nullptr)&&(radar._serialport!=nullptr))
        _serialport->setPort(QSerialPortInfo(*(radar._serialport)));

    _radar_uid=radar._radar_uid;
    _map_octws_param_names = radar._map_octws_param_names;
    _device_name = radar._device_name;
    _expected_portname = radar._expected_portname;
    _fixed_serial_port = radar._fixed_serial_port;
    _fixed_id = radar._fixed_id;

    return *this;
}

//-----------------------------------------------
void radarInstance::initSerialPortTimer()
{
    if (_serialport == nullptr) _serialport     = new QSerialPort(this);
    if (_serial_timer==nullptr) _serial_timer   = new QTimer(this);
    connect(_serialport,    &QSerialPort::errorOccurred,    this, &radarInstance::serial_error);
    connect(_serial_timer,  &QTimer::timeout,               this, &radarInstance::serial_timeout);
	//connect(_serialport,    &QSerialPort::readyRead,        this, &radarInstance::read_data);
	//connect(_serialport,    &QSerialPort::bytesWritten,     this, &radarInstance::bytes_written);
    _serial_status = SS_IDLE;
    _serial_timer->setSingleShot(true);
    _serial_timer->blockSignals(false);
    _serialport->blockSignals(false);
    _bytesToWrite = 0;

    if (_serialport->isOpen())
    {
        _serialport->clear();
        _serialport->flush();
    }
}
//-----------------------------------------------
void radarInstance::clearSerialPortTimer()
{
    disconnect();
    if (_serialport != nullptr)
        delete _serialport;
    if (_serial_timer!=nullptr)
        delete _serial_timer;

    _serial_timer = nullptr;
    _serialport  = nullptr;
    _serial_status = SS_IDLE;
}

//-----------------------------------------------
void    radarInstance::set_module(radarModule *ref)
{
    clear_antenna_table();
    clear_param_table();
    clear_txrx_pairs();
    clear_commands_tables();
    clear_scripts_tables();

    _ref_module = ref;
    if (ref == nullptr) return;

    radarModule::operator=(*ref);

    set_inquiry_instanceid_command(_ref_module->get_inquiry_instanceid_command());
    set_inquiry_moduleid_command(_ref_module->get_inquiry_moduleid_command());
    set_inquiry_moduleid_expectedvalue(_ref_module->get_inquiry_moduleid_expectedvalue());
    _protocol = _ref_module->get_comm_protocol();
}
//-----------------------------------------------
radarInstance::~radarInstance()
{
    clearSerialPortTimer();
    clear_antenna_table();
    clear_param_table();
    clear_txrx_pairs();
    clear_commands_tables();
    clear_scripts_tables();
}
//-----------------------------------------------
QString radarInstance::get_item_descriptor()
{
    return (_device_name.isEmpty()? projectItem::get_name() : _device_name) +
           ( _ref_module==nullptr?":ERROR_UNDEFINED" : QString(":")+ _ref_module->get_name());
}
//-----------------------------------------------
void            radarInstance::attach_to_workspace(octavews *dws)
{
    _workspace = dws;
    const auto& px = get_param_table();
    for (const auto& param: px)
        if (param!=nullptr)
            param->link_to_workspace(_workspace);
}

//-----------------------------------------------
octavews*  radarInstance::get_workspace(void)
{
    return _workspace;
}
//-----------------------------------------------
QString     radarInstance::get_mapped_name(radarParamPointer param)
{
    QString mapped_name;
    if (param == nullptr) return mapped_name;
    if (!param->is_linked_to_octave())
        return param->get_name();

    QString alias = param->get_alias_octave_name();
    QString base_name = alias.isEmpty() ? param->get_name() : alias;


    if (param->is_compound_name())
        mapped_name = (QFileInfo(_device_name).baseName()) + "_" +
                      (_serialport == nullptr ? "noPort" : _serialport->portName()) + "_" + base_name;
    else
        mapped_name = base_name;

    return mapped_name;
}
//-----------------------------------------------
void        radarInstance::create_variable(QString parameterName)
{
    if (_workspace == nullptr) return;

    radarParamPointer param = get_param(parameterName);
    if (param == nullptr) return;

    QString mapped_name = get_mapped_name(param);

    _map_octws_param_names[mapped_name] = param;
}

//-----------------------------------------------
void            radarInstance::create_all_variables()
{
    const auto& px = get_param_table();
    for (const auto& param: px)
    {
        if (param==nullptr) continue;
        if (!param->is_linked_to_octave()) continue;
        QString octave_param_name = get_mapped_name(param);
        _map_octws_param_names[octave_param_name] = param;
    }
}
//-----------------------------------------------
void            radarInstance::remove_variable(QString parameterName)
{
    remove_variable(get_param(parameterName));
}
//-----------------------------------------------
void            radarInstance::remove_variable(radarParamPointer param)
{
    if (param==nullptr)
        return;
    QString param_name = get_mapped_name(param);
    _map_octws_param_names.remove(param_name);
}

//-----------------------------------------------
void            radarInstance::workspace_to_params()
{
    if (_workspace==nullptr) return;
    const auto& mapx = _map_octws_param_names;
    for (auto pair = mapx.begin(); pair != mapx.end(); pair++)
    {
        QString octave_name = pair.key();
        radarParamPointer  param = pair.value();
        if (param==nullptr) continue;

        param->set_value(_workspace->var_value(octave_name.toStdString()));
    }
}
//-----------------------------------------------
void            radarInstance::params_to_workspace()
{
    if (_workspace == nullptr) return;
    const auto& mapx = _map_octws_param_names;
    for (auto pair = mapx.begin(); pair != mapx.end(); pair++)
    {
        radarParamPointer param = pair.value();
        if (param==nullptr) continue;
        _workspace->add_variable(pair.key().toStdString(),false,param->get_value());
    }
}

//-----------------------------------------------
QString         radarInstance::get_uid_string()
{
    return _radar_uid.toHex();
}
//-----------------------------------------------
QByteArray      radarInstance::get_uid()
{
    return _radar_uid;
}
//-----------------------------------------------
void            radarInstance::set_uid_string(const QString& uid)
{
    _radar_uid = QByteArray::fromHex(uid.toLatin1());
}
//-----------------------------------------------
void            radarInstance::set_uid(const QByteArray& uid)
{
    _radar_uid = uid;
}

//-----------------------------------------------
radarModule*    radarInstance::get_module()
{
    return _ref_module;
}

//-----------------------------------------------
void            radarInstance::set_fixed_id(bool fixed_id)
{
    _fixed_id = fixed_id;
}
//-----------------------------------------------
void            radarInstance::set_fixed_port(bool fixed_port)
{
    _fixed_serial_port = fixed_port;
}
//-----------------------------------------------
bool            radarInstance::fixed_id()
{
    return _fixed_id;
}
//-----------------------------------------------
bool            radarInstance::fixed_port()
{
    return _fixed_serial_port;
}
//-----------------------------------------------
bool radarInstance::query_module_id()
{
    QVector<radarParamPointer> params = get_param_group(get_inquiry_moduleid_command());

    // Inquiry the module this device belongs to
    if ((params.count()==0)&&(_fixed_id))
        return false;

    if (!transmit_param_blocking(params)) return false;

    const auto& paramx = params;

    QByteArray pdata;
    for (const auto& param : paramx)
        if (param!=nullptr)
            pdata.append(param->chain_values());

    // is device returning the proper model?
    return (get_inquiry_moduleid_expectedvalue()==pdata);
}
//-----------------------------------------------
QByteArray radarInstance::query_serial_id(bool &bok)
{
    bok = false;
    QByteArray out;
    if (!is_connected())
        return out;

    // We have a correspondant module. Let's inquiry the radar-ID
    QVector<radarParamPointer> params_id = get_param_group(_ref_module->get_inquiry_instanceid_command());

    if (!transmit_param_blocking(params_id))
        out = QByteArray();
    else
        out = get_current_value_group(_ref_module->get_inquiry_instanceid_command());

    bok = true;
    return out;
}
//-----------------------------------------------
/*
 * In this procedure, basically a radar "scans" available serial ports to create a connection between the class instance and the physical unit
 * attached to some serial port.
 *
*/
bool            radarInstance::scan()
{
    if (_ref_module==nullptr)
        return false;

    QList<QSerialPortInfo> _availablePorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& port : _availablePorts)
    {
        if (!set_port(port,true))
            continue;

        // ask for radar id
        bool bok;
        QByteArray radar_id = query_serial_id(bok);
        if (_fixed_id)
        {
            if (radar_id == _radar_uid)
                return true;
        }
    }

    return false;
}

//-----------------------------------------------
void            radarInstance::set_device_name(const QString& device_name)
{
    _device_name = device_name;
}
//-----------------------------------------------
QString         radarInstance::get_device_name()
{
    return _device_name;
}
//-----------------------------------------------
void            radarInstance::set_expected_portname(const QString& portname)
{
    _expected_portname = portname;
}
//-----------------------------------------------
QString         radarInstance::get_expected_portname()
{
    return _expected_portname;
}
//-----------------------------------------------
void            radarInstance::save_extra_project_item_data(QDomDocument& doc,QDomElement& node)
{
}
//-----------------------------------------------
void            radarInstance::load_extra_project_item_data(QDomElement& node)
{
    _device_name = node.attribute("display_name","");
    _fixed_id    = node.attribute("fixed_id","false")=="true"? true : false;
    _fixed_serial_port = node.attribute("fixed_comport","false")=="true"? true : false;
    _expected_portname = node.attribute("port_name","");
    _radar_uid = QByteArray::fromHex(node.attribute("id","").toLatin1());

}
//-----------------------------------------------
bool            radarInstance::can_differentiate()
{
    return _fixed_id || _fixed_serial_port;
}
//-----------------------------------------------
QByteArray      radarInstance::set_port_no_module_check(char module_id_inquiry_command,const QSerialPortInfo& port, const SerialSettings& settings)
{
    QByteArray module_id;

    if (_serialport == nullptr)
        initSerialPortTimer();
    else
        if (is_connected()) disconnect();

    if (_serialport == nullptr)
        return module_id;

    _serialport->setPort(port);
    _serialport->setDataBits(settings.dataBits);
    _serialport->setStopBits(settings.stopBits);
    _serialport->setParity(settings.parity);
    _serialport->setBaudRate(settings.baudRate);
    _serialport->setFlowControl(settings.flowControl);
    _serialport->setReadBufferSize(0);
    port_connect();

    if (!is_connected())
        return module_id;

    QByteArray data_tx;

    if (_protocol!=nullptr)
        data_tx.append(_protocol->get_start());
    data_tx.append(module_id_inquiry_command);
    if (_protocol!=nullptr)
        data_tx.append(_protocol->get_stop());

    transmit_and_wait_answer(data_tx);

    module_id = _protocol->decode_rx(_last_serial_rx);
    return module_id;
}
//-----------------------------------------------
bool    radarInstance::set_port(const QSerialPortInfo& port, const SerialSettings& settings, bool force_connection)
{
    if (_serialport == nullptr)        
        initSerialPortTimer();
    else
        if (is_connected()) disconnect();

    if (_serialport == nullptr)
        return false;

    if (!force_connection)
        if (_fixed_serial_port)
        {
            if (!_expected_serial_description.isEmpty())
                if (port.description()!=_expected_serial_description) return false;

            if (!_expected_serial_manufacturer.isEmpty())
                if (port.manufacturer()!=_expected_serial_manufacturer.toHex()) return false;

            if (!_expected_serial_number.isEmpty())
                if (port.serialNumber()!=_expected_serial_number.toHex()) return false;

            if (_expected_vendorid!=0)
                if (port.vendorIdentifier()!=_expected_vendorid) return false;

            if (_expected_productid!=0)
                if (port.productIdentifier()!=_expected_productid) return false;
        }

    _serialport->setPort(port);
    _serialport->setDataBits(settings.dataBits);
    _serialport->setStopBits(settings.stopBits);
    _serialport->setParity(settings.parity);
    _serialport->setBaudRate(settings.baudRate);
    _serialport->setFlowControl(settings.flowControl);
    _serialport->setReadBufferSize(0);
    port_connect();

    if (!force_connection)
    {
        // Give the query 2 chances
        if (!query_module_id())
            if (!query_module_id())
            {
                disconnect();
                return false;
            }

        if (_fixed_id)
        {
            bool bok = false;
            _actual_radar_uid = query_serial_id(bok);
            if (!bok)
            {
                _actual_radar_uid = QByteArray::fromHex("FFFFFFFF");
            }

            if (_fixed_serial_port && (_serialport->portName()!=_expected_portname))
            {
                disconnect();
                return false;
            }
            if (_fixed_id && (_actual_radar_uid!=_radar_uid))
            {disconnect(); return false;}
        }
    }
    if (is_connected())
    {
        emit connection_done();
        return true;
    }
    return false;
}

bool    radarInstance::set_port(const QSerialPortInfo& port, bool force_connection)
{
    if (_ref_module==nullptr) return false;
    return set_port(port, _ref_module->get_serial_port_configuration(), force_connection);
}

//-----------------------------------------------
void            radarInstance::set_ports(const QList<QSerialPortInfo>& available_ports)
{
    if (_ref_module==nullptr)
        return;
    // We may have:
    // 1. serial port unique, different radars UID
    // 2. radar UID, variable serial port
    // 3. (single radar project) variable serial port, any UID
//    _serialport.setPort();
    QList<QSerialPortInfo> search_ports;
    if (_fixed_serial_port)
    {
        // Narrow down the list of ports to the available one
        for (auto& port: available_ports)
        {
            if (port.portName() == _expected_portname)
                search_ports.append(port);
        }
    }
    else
        search_ports = available_ports;

    for (auto& port : search_ports)
    {
        if (set_port(port,false))
            return;
    }
}
//-----------------------------------------------
void    radarInstance::auto_connect()
{
    set_ports(QSerialPortInfo::availablePorts());
}

//-----------------------------------------------
void    radarInstance::set_param_value(radarParamPointer param, const QVector<QVariant>& val, bool transmit,bool update_workspace)
{
    if (param==nullptr) return;

    if (param->get_io_type()==RPT_IO_OUTPUT)
        return;
    if (param->get_type()==RPT_VOID)
        return;

    RP_STATUS stat = param->get_status();
    if (stat != RPS_MODIFIED)
        return;

    param->variant_to_value(val);

    if (!param->is_last_assignment_valid())
        return;

    if (!is_connected())
    {
        param->set_status(RPS_UPDATED);
        update_param_workspace_gui(param);        
        return;
    }

    if ((transmit)&&(is_connected())&&(param->get_status()==RPS_MODIFIED))
		transmit_param(param);
 }
//---------------------------------------------
// This is a patch to avoid script execution termination
// when immediate_update is applied
void    radarInstance::update_param_internal_copy(radarParamPointer param, octave_value val)
{
    if (param==nullptr) return;
    octave_value valint = val;
    RADARPARAMTYPE type = param->get_type();
    switch (type)
    {
    case RPT_INT8:
        param->set_value(valint.int8_array_value());
        return;
    case RPT_UINT8:
        param->set_value(valint.uint8_array_value());
        return;
    case RPT_INT16:
        param->set_value(valint.int16_array_value());
        return;
    case RPT_UINT16:
        param->set_value(valint.uint16_array_value());
        return;
    case RPT_INT32:
        param->set_value(valint.int32_array_value());
        return;
    case RPT_UINT32:
        param->set_value(valint.uint32_array_value());
        return;
    case RPT_STRING:
        param->set_value(valint.char_array_value());
        return;
    case RPT_FLOAT:
        param->set_value(valint.float_array_value());
        return;
    default:
        return;

    }
    return;
 }


//-----------------------------------------------
void    radarInstance::set_param_value(radarParamPointer param, octave_value val, bool transmit, bool update_workspace)
{

    if (param==nullptr) return;

    if (param->get_io_type()==RPT_IO_OUTPUT)
        return;

    param->set_value(val);

    RP_STATUS stat = param->get_status();
    if (stat != RPS_MODIFIED)
        return;

    if (!is_connected())
    {
        param->set_status(RPS_UPDATED);
        update_param_workspace_gui(param);        
        return;
    }

    if ((transmit)&&(is_connected())&&(param->get_status()==RPS_MODIFIED))
        transmit_param(param);
}

//-----------------------------------------------
void         radarInstance::set_param_value(QString name, octave_value val, bool transmit, bool update_workspace)
{
    if (name.isEmpty()) return;

    for (auto& param: _params)
        if ((param!=nullptr)&&(param->get_io_type()!=RPT_IO_OUTPUT))
        {
            QString mapped_name = get_mapped_name(param);
            if (mapped_name==name)
            {
                if (param->get_io_type()==RPT_IO_OUTPUT)
                    return;

                param->set_value(val);

                RP_STATUS stat = param->get_status();
                if (stat != RPS_MODIFIED)
                    return;

                if (!is_connected())
                {
                    param->set_status(RPS_UPDATED);
                    update_param_workspace_gui(param);                    
                    return;
                }

                if ((transmit)&&(is_connected())&&(param->get_status()==RPS_MODIFIED))
                {
                    // Transmit param may update the value according to device response, we need to update the linked var
                    transmit_param(param);

                }
                return;
            }
        }
}

//-----------------------------------------------
void         radarInstance::set_param_value(int param_index, octave_value val, bool transmit, bool update_workspace)
{
    if ((param_index < 0)||(param_index>=get_param_count()))
            return;
    radarParamPointer param = _params[param_index];
    if (param==nullptr) return;

    if (param->get_io_type()==RPT_IO_OUTPUT)
        return;

    param->set_value(val);

    RP_STATUS stat = param->get_status();
    if (stat != RPS_MODIFIED)
        return;


    if (!is_connected())
    {
        param->set_status(RPS_UPDATED);
        update_param_workspace_gui(param);        
        return;
    }

    if ((transmit)&&(is_connected())&&(param->get_status()==RPS_MODIFIED))
        transmit_param(param);
}

//-----------------------------------------------
// Update a value in the connected device
void    radarInstance::set_param_value_default(int param_index, bool transmit)
{
    if ((param_index < 0)||(param_index>=get_param_count()))
        return;
    if (_params[param_index]==nullptr) return;

    if (_ref_module == nullptr) return;
    if (_params[param_index]->get_io_type()==RPT_IO_OUTPUT)
        return;

    RP_STATUS stat = _params[param_index]->get_status();

    if (stat!=RPS_IDLE) return;
    _params[param_index]->set_status(RPS_MODIFIED);

    radarParamPointer _defpar = _ref_module->get_param(param_index);
    if (_defpar == nullptr)
        return;

    _params[param_index]->set_value(_defpar->get_value());

    if (!is_connected())
    {
        _params[param_index]->set_status(RPS_UPDATED);
        update_param_workspace_gui(_params[param_index]);
        return;
    }

    if ((transmit)&&(is_connected())&&(_params[param_index]->get_status()==RPS_MODIFIED))
        transmit_param(_params[param_index]);

}


//-----------------------------------------------
// Update a value in the connected device
bool radarInstance::transmit_param(radarParamPointer param)
{
    if (!is_connected())
        return false;

    if (param==nullptr) return false;
    QVector<radarParamPointer> params = get_param_group(param);
    if (param->get_status()!=RPS_MODIFIED)
        return false;

/*    for (auto& p: params)
        if (p!=nullptr)
        {
            if (p->get_status()!=RPS_MODIFIED)
                return false;
        }*/
    for (auto& p: params)
        if (p!=nullptr)
        {
            p->set_status(RPS_TRANSMITTING);
        }

    if (params.count()==0)
        return false;

	return transmit_param_blocking(params);
}


//-----------------------------------------------
bool    radarInstance::read_param_value(QString name, bool use_alias)
{
    if (!is_connected())
        return false;

    radarParamPointer param = nullptr;

    for (const auto& p : _params)
        if (p!=nullptr)
        {
            if ((use_alias ? p->get_alias_octave_name() : p->get_name()) == name)
            {
                param = p;
                break;
            }
        }

    if (param==nullptr) return false;
    QVector<radarParamPointer> params = get_param_group(param);

    if (params.count()==0)
        return false;

	return transmit_param_blocking(params, _serialport);
}

//-----------------------------------------------
bool    radarInstance::read_param_value(int param_index)
{
    if (!is_connected())
        return false;

    radarParamPointer param = ((param_index < 0) || (param_index >= _params.count())) ? nullptr : _params[param_index];

    if (param==nullptr) return false;
    QVector<radarParamPointer> params = get_param_group(param);

    if (params.count()==0)
        return false;

	return transmit_param_blocking(params, _serialport);
}
//-------------------------------------------------
bool    radarInstance::save_xml(QDomDocument& document)
{
    QDomElement root = document.createElement("ARIA_Radar_Device");
    QVersionNumber xml_ver({XML_RADARFILE_MAJOR,XML_RADARFILE_MINOR,XML_RADARFILE_SUBVER});
    root.setAttribute("xml_dataformat", xml_ver.toString());
    root.setAttribute("radar_module_name",_module_name);

    document.appendChild(root);

    QDomElement radar_node = document.createElement("radar_device");
    radar_node.setAttribute("radar_module", _ref_module == nullptr ? "" : _ref_module->get_name());
    radar_node.setAttribute("display_name", _device_name);
    radar_node.setAttribute("fixed_id", _fixed_id ? QString("true") : QString("false"));
    radar_node.setAttribute("fixed_comport", _fixed_serial_port ? QString("true") : QString("false"));
    radar_node.setAttribute("port_name",_expected_portname);
    radar_node.setAttribute("id",QString(_radar_uid.toHex(0)));

    QDomElement parameter_list = document.createElement("actual_params");
    parameter_list.setAttribute("count",_params.count());

    for (const auto& param: _params)
        if (param!=nullptr)
            param->save_xml(document,parameter_list);
    radar_node.appendChild(parameter_list);

    QDomElement script_list = document.createElement("init_scripts");
    script_list.setAttribute("count",_init_scripts.count());

    for (auto& script: _init_scripts)
        if (script!=nullptr)
            if (!script->save_xml(document,script_list)) return false;
    radar_node.appendChild(script_list);

    script_list = document.createElement("post_acquisition_scripts");
    script_list.setAttribute("count",_post_acquisition_scripts.count());

    for (auto& script: _post_acquisition_scripts)
        if (script!=nullptr)
            if (!script->save_xml(document,script_list)) return false;
    radar_node.appendChild(script_list);
    root.appendChild(radar_node);
    return true;
}

//-------------------------------------------------
bool    radarInstance::save_xml()
{
    _rfile.setFileName(get_full_filepath());
    if (!_rfile.open(QFile::WriteOnly | QFile::Text ))
    {
        qDebug() << "Already opened or permission issue";
        _rfile.close();
        return false;
    }
    QTextStream xmlContent(&_rfile);

    QDomDocument document;

    if (!save_xml(document)) {_rfile.close(); return false;}
    xmlContent << document.toString();
    _rfile.close();
    return true;
}
//---------------------------------------------
bool radarInstance::load_xml()
{
    QDomDocument document;

    if (!_rfile.open(QIODevice::ReadOnly ))
    {
        return false;
    }
    document.setContent(&_rfile);
    _rfile.close();
    QDomElement root = document.firstChildElement("ARIA_Radar_Device");
    if (root.isNull()) return false;

    QString xml_ver_string = root.attribute("xml_dataformat");

    QVersionNumber xml_ver = QVersionNumber::fromString(xml_ver_string);

    QVersionNumber current_xml({XML_RADARFILE_MAJOR,XML_RADARFILE_MINOR, XML_RADARFILE_SUBVER});

    if (xml_ver > current_xml)
        return false;

    QDomElement radar_node = root.firstChildElement("radar_device");

    if (radar_node.isNull()) return false;

    _device_name        = radar_node.attribute("display_name", "");
    _fixed_id           = radar_node.attribute("fixed_id","false")==QString("true") ? true : false;
    _fixed_serial_port  = radar_node.attribute("fixed_comport","false") == QString("true") ? true : false;
    _expected_portname  = radar_node.attribute("port_name","REQ_PORT_NAME_NOT_PROVIDED");
    if (!_fixed_serial_port) _expected_portname = "";

    _radar_uid =  QByteArray::fromHex(radar_node.attribute("id").toLatin1());

    QString radar_module_name = radar_node.attribute("radar_module","");
    radarModule* ref_mod = (radarModule*)get_root()->get_child(radar_module_name, DT_RADARMODULE);

    if (ref_mod==nullptr)
        return false;
    else
    {
        QString name_backup = this->get_name();
        QString file_backup = this->get_filename();

        set_module(ref_mod);

        this->set_name(name_backup);
        this->set_filename(file_backup);
    }

    // Load parameters
    QDomElement parameter_list = radar_node.firstChildElement("actual_params");
    if (!parameter_list.isNull())
    {
        //int expected_size = parameter_list.attribute("count","0").toInt();

        QDomElement param_elem = parameter_list.firstChildElement("parameter");

        while (!param_elem.isNull())
        {
            radarParamPointer p_param = radarParamBase::create_from_xml_node(document,param_elem);
            if (p_param == nullptr)
                break;

            radarParamPointer existing_param = get_param(p_param->get_name());
            radarParamPointer ref_param = nullptr;
            if (existing_param==nullptr)
            {
                _params.append(p_param);
                ref_param = p_param;
            }
            else
            {
                (*existing_param) = (*p_param);
                p_param.reset();
                ref_param = existing_param;
            }

            if (ref_param!=nullptr)
            {
                // Set for this parameters the limits set by the module
                radarParamPointer def    = _ref_module->get_param(ref_param->get_name());
                if (def==nullptr) continue;


                if (def->has_min_max())
                    ref_param->set_min_max(def->get_min(), def->get_max());

                if (def->has_available_set())
                    ref_param->variant_to_availabeset(def->availableset_to_variant());
            }

        }
      //  if (_params.count()!=expected_size) return false;
    }

    // Load scripts

    QDomElement scripts = radar_node.firstChildElement("init_scripts");
    //bool bOk;
    if (!scripts.isNull())
    {
        //int num_scripts = scripts.attribute("count","0").toInt(&bOk);
        QDomElement script = scripts.firstChildElement("script");
        _init_scripts.resize(0);

        do
        {
            QString script_file = QFileInfo(script.attribute("filename")).fileName();
            octaveScript_ptr octave_script = (octaveScript*)(get_root()->get_child(script_file,DT_SCRIPT));
            if (octave_script!=nullptr)
                _init_scripts.append(octave_script);

            script = script.nextSiblingElement("script");
        }
        while(!script.isNull());

        //if (_init_scripts.count()!=num_scripts) return false;
    }

    scripts = radar_node.firstChildElement("post_acquisition_scripts");

    if (!scripts.isNull())
    {
        //int num_scripts = scripts.attribute("count","0").toInt(&bOk);
        QDomElement script = scripts.firstChildElement("script");
        _post_acquisition_scripts.resize(0);
        do
        {
            QString script_file = QFileInfo(script.attribute("filename")).fileName();
            octaveScript_ptr octave_script = (octaveScript*)(get_root()->get_child(script_file,DT_SCRIPT));
            if (octave_script!=nullptr)
                _post_acquisition_scripts.append(octave_script);

            script = script.nextSiblingElement("script");
        }
        while(!script.isNull());

        //if (_post_acquisition_scripts.count()!=num_scripts) return false;
    }
    return true;
}


//--------------------------------------------------
void    radarInstance::update_module()
{
    if (_ref_module==nullptr) return;
    QVector<QString> to_delete;
    QVector<QString> to_add;                        // Copy a new
    QVector<QString> to_update;                     // Need to update, including current value
    QVector<QString> to_update_preserve_actual;     // Need to update limits / available sets (saving current value)
    QVector<QString> to_delete_ref;
    for (auto param : _params)
    {

        if (param==nullptr)
        {
//            to_delete.append(param->get_name());
            continue;
        }
        radarParamPointer param_ref = _ref_module->get_param(param->get_name());
        // A param is not defined in the refModule
        if (param_ref == nullptr)
        {
            to_delete.append(param->get_name());
            continue;
        }

        if (param_ref->get_type() != param->get_type())
        {
            to_update.append(param->get_name());
            continue;
        }

        if ((param->has_available_set() && (param_ref->availableset_to_variant() != param->availableset_to_variant()))||
            ((param->has_min_max()) && ((param_ref->get_max() != param->get_max()) ||  (param_ref->get_min() != param->get_min()))))
        {
            to_update_preserve_actual.append(param->get_name());
            continue;
        }
    }
    // to_update, to_update_preserve and to_delete's indexes refer to _param vector

    int npref = _ref_module->get_param_count();
    for (int np = 0; np < npref; np++)
    {
        radarParamPointer pref = _ref_module->get_param(np);

        if (pref == nullptr)
        {
//            to_delete_ref.append(pref->get_name());
            continue;
        }

        radarParamPointer pdev = get_param(np);
        if (pdev == nullptr)
            to_add.append(pref->get_name());
    }
    if (!(to_delete.isEmpty() && to_update.isEmpty() && to_add.isEmpty() && to_update_preserve_actual.isEmpty()))
    {
        // Go thru list of update (full update)
        for (const auto& nupdate : to_update)
        {
            radarParamPointer param = get_param(nupdate);
            radarParamPointer ref   = _ref_module->get_param(param->get_name());
            // Save alias, plot and compound
            bool bcompound = param->is_compound_name();
            QString alias  = param->get_alias_octave_name();
            (*param) = (*ref);
            param->set_alias_octave_name(alias);
            param->set_compound_name(bcompound);
        }

        for (const auto& nupdate : to_update_preserve_actual)
        {
            // We just need to update limits
            radarParamPointer param = get_param(nupdate);
            radarParamPointer ref   = _ref_module->get_param(param->get_name());

            if (ref->has_available_set())
                param->variant_to_availabeset(ref->availableset_to_variant());

            if (ref->has_min_max())
                param->set_min_max(ref->get_min(), ref->get_max());

            if ((!ref->has_available_set())&&(!ref->has_min_max()))
            {param->remove_available_set(); param->remove_min_max();}
        }

        for (const auto& np : to_delete)
            remove_param(np);

        for (const auto& np : to_add)
        {
            radarParamPointer new_param = _ref_module->get_param(np)->clone();
            append_param(new_param);
        }
    }

    if (!to_delete_ref.isEmpty())
    {
        for (const auto& np: to_delete_ref)
            _ref_module->remove_param(np);

        _ref_module->save_file(_ref_module->get_full_filepath());
    }

    _init_commands = _ref_module->get_init_commands();
    _postacquisition_commands = _ref_module->get_postacquisition_commands();
    // Inquiry
    set_inquiry_moduleid_command(_ref_module->get_inquiry_moduleid_command());
    set_inquiry_moduleid_expectedvalue(_ref_module->get_inquiry_moduleid_expectedvalue());
    set_inquiry_instanceid_command(_ref_module->get_inquiry_instanceid_command());

    // Serial port
    set_expected_serial_port_data(_ref_module->get_serial_description(),
                                  _ref_module->get_serial_manufacturer(),
                                  _ref_module->get_serial_number(),
                                  _ref_module->get_serial_vendorid(),
                                  _ref_module->get_serial_productid());

    set_serial_port_configuration(_ref_module->get_serial_port_configuration().baudRate,
                                  _ref_module->get_serial_port_configuration().dataBits,
                                  _ref_module->get_serial_port_configuration().parity,
                                  _ref_module->get_serial_port_configuration().stopBits,
                                  _ref_module->get_serial_port_configuration().flowControl
                                  );

    save_xml();
}
//--------------------------------------------------
void  radarInstance::clear_params_update_lists()
{
    _n_params_to_receive = 0;
    _params_to_inquiry.clear();
    _params_to_modify.clear();
    _params_updated.clear();
}
//--------------------------------------------------
bool radarInstance::init_pre()
{
    if (_workspace == nullptr)
        return false;

    if (!is_connected()) return false;

    if (_workspace==nullptr) return false;

    if (_workspace->data_interface()==nullptr) return false;

    clear_params_update_lists();

    //1. Feed params from device to workspace according to policy
    for (int param_index: _init_commands)
    {
        radarParamPointer par = _params[param_index];
        if (par==nullptr) continue;
        QVector<radarParamPointer> params = get_param_group(par);

        RADARPARAMIOTYPE rpt_io = params[0]->get_io_type();

        if ((rpt_io == RPT_IO_OUTPUT)||((rpt_io == RPT_IO_IO)&&(params[0]->has_inquiry_value()))||((rpt_io == RPT_IO_INPUT) && (params[0]->get_type()==RPT_VOID)))
        {
            if (!_params_to_inquiry.contains(params[0]))
            {
                _n_params_to_receive+=params.length();
                _params_to_inquiry.append(params[0]);
            }
        }
    }
    _radar_operation = RADAROP_INIT_PARAMS;

    if (!_params_to_inquiry.isEmpty())
    {
            inquiry_parameter(_params_to_inquiry[0]);

#ifdef UPDATE_INSIDE
            _workspace->set_variable(get_mapped_name(param).toStdString(),   param->get_value());
#endif
#ifdef INTERFACE_STRUCT
            // in the struct maintain the parameter name
            _radar_cell.setfield(param->get_name().toStdString(),param->get_value());
#endif

    }

#ifdef INTERFACE_STRUCT
    _workspace->set_variable(get_device_name().toStdString(), _radar_cell);
#endif
#ifdef UPDATE_INSIDE
    // Update workspace
    _workspace->workspace_to_interpreter();
#endif

    if (_params_to_inquiry.empty())
    {
#ifdef UPDATE_INSIDE
        _workspace->workspace_to_interpreter();
#endif
        emit all_params_updated();
        emit_operation_done();
    }

    return true;
}
//--------------------------------------------------
bool radarInstance::init_scripts()
{
    // Run scripts
    if (_workspace == nullptr)
        return false;
    if (_workspace->data_interface()==nullptr) return false;

    _radar_operation = RADAROP_INIT_SCRIPTS;

    //1. Feed params from device to workspace according to policy
    for (int param_index: _init_commands)
    {
        radarParamPointer par = _params[param_index];
        if (par==nullptr) continue;

        RADARPARAMIOTYPE rpt_io = par->get_io_type();
        QVector<radarParamPointer> params = get_param_group(par);
        if ((rpt_io != RPT_IO_INPUT)&(par->get_type()!=RPT_VOID))
        {
            if (!_params_to_modify.contains(params[0]))
            {
                _n_params_to_receive+=params.length();
                _params_to_modify.append(params[0]);
            }
        }

    }

    for (auto& script: _init_scripts)
        if (script != nullptr)
            if (!_workspace->data_interface()->run(script->get_text(),true)) return false;

    if (_params_to_modify.empty()||_init_scripts.empty())
    {
        clear_params_update_lists();
        emit all_params_updated();
        emit_operation_done();
    }
    if (!_params_to_modify.empty())
    {

		transmit_param_blocking(get_param_group(_params_to_modify[0]), false);
    }
    return true;
}
//--------------------------------------------------
void radarInstance::init()
{
   // Go thru list of
    if (_workspace == nullptr)
        return;

    if (!is_connected()) return;

    if (_workspace==nullptr) return;

    if (_workspace->data_interface()==nullptr) return;

    init_pre();

    init_scripts();
}

//--------------------------------------------------
bool radarInstance::postacquisition_pre()
{
    if (_workspace == nullptr)
        return false;

    if (!is_connected()) return false;

    if (_workspace==nullptr) return false;

    if (_workspace->data_interface()==nullptr) return false;

    clear_params_update_lists();

    //1. Feed params from device to workspace according to policy
    for (int param_index: _postacquisition_commands)
    {
        radarParamPointer par = _params[param_index];
        if (par==nullptr) continue;

        QVector<radarParamPointer> params = get_param_group(par);
        RADARPARAMIOTYPE rpt_io = params[0]->get_io_type();
        if ((rpt_io == RPT_IO_OUTPUT)||((rpt_io == RPT_IO_IO)&&(params[0]->has_inquiry_value())))
        {
            if (!_params_to_inquiry.contains(params[0]))
            {
                _n_params_to_receive+=params.length();
                _params_to_inquiry.append(params[0]);
            }
        }

    }
    _radar_operation = RADAROP_POSTACQ_PARAMS;

    if (!_params_to_inquiry.isEmpty())
    {
        inquiry_parameter(_params_to_inquiry[0]);
#ifdef UPDATE_INSIDE
        _workspace->set_variable(get_mapped_name(param).toStdString(),   param->get_value());
#endif
#ifdef INTERFACE_STRUCT
        // in the struct maintain the parameter name
        _radar_cell.setfield(param->get_name().toStdString(),param->get_value());
#endif
    }

#ifdef INTERFACE_STRUCT
    _workspace->set_variable(get_device_name().toStdString(), _radar_cell);
#endif

    if (_params_to_inquiry.empty())
    {
        emit all_params_updated();
        emit_operation_done();
    }
#ifdef UPDATE_INSIDE
    // Update workspace
    _workspace->workspace_to_interpreter();
#endif

    return true;
}
//--------------------------------------------------
bool radarInstance::postacquisition_scripts()
{
    // Run scripts
    if (_workspace == nullptr)
        return false;
    if (_workspace->data_interface()==nullptr) return false;

    _radar_operation = RADAROP_POSTACQ_SCRIPTS;

    //1. Feed params from device to workspace according to policy
    for (int param_index: _postacquisition_commands)
    {
        radarParamPointer par = _params[param_index];
        if (par==nullptr) continue;

        RADARPARAMIOTYPE rpt_io = par->get_io_type();

        if ((rpt_io != RPT_IO_INPUT)&&(par->get_type()!=RPT_VOID))
        {
            QVector<radarParamPointer> params = get_param_group(par);
            if (!_params_to_modify.contains(params[0]))
            {
                _n_params_to_receive+=params.length();
                _params_to_modify.append(params[0]);
            }
        }
    }

    for (auto& script: _post_acquisition_scripts)
        if (script != nullptr)
            if (!_workspace->data_interface()->run(script->get_text(),true)) return false;

    if (_params_to_modify.empty()||_post_acquisition_scripts.isEmpty())
    {
        clear_params_update_lists();
        emit all_params_updated();
        emit_operation_done();
    }

    if (!_params_to_modify.empty())
    {
		transmit_param_blocking(get_param_group(_params_to_modify[0]), false);
    }
    return true;
}


//--------------------------------------------------
void radarInstance::run()
{
    // Run pre-processing script
    if (_workspace == nullptr)
        return;

}
//--------------------------------------------------
void radarInstance::export_to_octave()
{
    if (_workspace == nullptr)
        return;

    for (auto& param: _params)
    {
        if (param == nullptr) continue;
        if (param->get_type()==RPT_VOID)
            continue;

        // add a single independent variable
        _workspace->set_variable(get_mapped_name(param).toStdString(),   param->get_value());

#ifdef INTERFACE_STRUCT
        // in the struct maintain the parameter name
        _radar_cell.setfield(param->get_name().toStdString(),param->get_value());
#endif
    }

#ifdef INTERFACE_STRUCT
    _workspace->set_variable(get_device_name().toStdString(), _radar_cell);
#endif
    _workspace->workspace_to_interpreter();

}

//--------------------------------------------------
void radarInstance::import_from_octave()
{
    if (_workspace == nullptr)
        return;
#ifdef INTERFACE_STRUCT
    octave_value radar_struct = _workspace->var_value(get_device_name().toStdString());

    if (radar_struct.isempty())
    {
        qDebug() << "empty data";
        return;
    }

    if (!radar_struct.isstruct())
    {
        qDebug() << "not a struct";
        return;
    }
    octave_map octstruct = radar_struct.map_value();
    for (auto & field : octstruct)
    {
        std::cout << field.first;
        std::cout << field.second;
        set_param_value(QString::fromStdString(field.first),octstruct.getfield(field.first).elem(0),false,true);
    }
#endif


}
//--------------------------------------------------
void    radarInstance::immediate_update_variable(const std::string& varname)
{
    if (_workspace == nullptr)
        return;

    if (_workspace->data_interface()==nullptr) return;
    octave_value var_value = _workspace->data_interface()->get_octave_engine()->varval(varname);

    if (varname.empty()) return;

    for (auto& param : _params)
        if (param!=nullptr)
        {
            if (get_mapped_name(param).toStdString() == varname)
            {
                immediate_set_param_value(param, var_value);
                return;
            }
        }
}
//--------------------------------------------------
void    radarInstance::immediate_inquiry_variable(const std::string& varname)
{
    if (varname.empty()) return;

    for (auto& param : _params)
        if (param!=nullptr)
        {
            if (get_mapped_name(param).toStdString() == varname)
            {
                immediate_inquiry_value(param);
                return;
            }
        }

}
//--------------------------------------------------
void    radarInstance::immediate_set_param_value(radarParamPointer param, const octave_value& val)
{
    if (param->get_io_type()==RPT_IO_OUTPUT)
        return;
    if (param->get_type()==RPT_VOID)
        return;

    QVector<radarParamPointer> params = get_param_group(param);

    // Inquiry the module this device belongs to
    if (params.count()==0)
        return;

    param->set_value(val);
    if (!is_connected()) return;
    if (transmit_param_blocking(params))
        param_is_updated(param);
}
//--------------------------------------------------
void    radarInstance::immediate_inquiry_value(radarParamPointer param)
{
    if (param->get_io_type()==RPT_IO_INPUT)
        return;
    if (param->get_type()==RPT_VOID)
        return;

    QVector<radarParamPointer> params = get_param_group(param);
    if ((!params[0]->has_inquiry_value())&&(params[0]->get_io_type()==RPT_IO_IO)) return;
    if (params.count()==0)
        return;
    if (!is_connected()) return;
    if (transmit_param_blocking(params, true))
        param_is_updated(param);
}
//--------------------------------------------------
void    radarInstance::immediate_set_command(radarParamPointer param)
{
    if (param->get_io_type()!=RPT_IO_INPUT)
        return;
    if (param->get_type()!=RPT_VOID)
        return;

    if (!is_connected()) return;
    transmit_command_blocking(param);
}

//--------------------------------------------------
void    radarInstance::immediate_command(const std::string& varname)
{
    if (varname.empty()) return;

    for (auto& param : _params)
        if (param!=nullptr)
        {
            if (get_mapped_name(param).toStdString() == varname)
            {
                immediate_set_command(param);
                return;
            }
        }
}
//--------------------------------------------------
void    radarInstance::update_variable(const std::string& varname)
{

    if (_workspace == nullptr)
        return;

    octave_value var_value = _workspace->var_value(varname);

    if (varname.empty()) return;
#ifdef INTERFACE_STRUCT
    if ((varname == get_device_name().toStdString())&&(var_value.isstruct()))
    {

        octave_map octstruct = var_value.map_value();

        for (auto& field : octstruct)
            set_param_value(QString::fromStdString(field.first),field.second,false,true);

        return;
    }
#endif

    for (auto& param : _params)
        if (param!=nullptr)
        {
            if (get_mapped_name(param).toStdString() == varname)
            {
                set_param_value(param, var_value, true, true);
                return;
            }
        }
}
//--------------------------------------------------
void    radarInstance::update_variables(const std::set<std::string>& varlist)
{
    // First update the struct, then update the mapped fields (higher priority for the latter)
#ifdef INTERFACE_STRUCT
    for (auto& varname : varlist)
    {
        if (varname.empty()) continue;
        if (varname == get_device_name().toStdString())
        {
            update_variable(varname);
            break;
        }
    }
#endif
    for (auto& varname : varlist)
    {
        for (auto& param:_params)
        {
            if (get_mapped_name(param).toStdString() == varname)
                if (param->get_io_type() != RPT_IO_OUTPUT)
                {
                    RP_STATUS status = param->get_status();
                    // In this case the modification is set by the workspace
                    if (status==RPS_IDLE)
                        param->set_status(RPS_MODIFIED);


                    if (!_params_to_modify.contains(param))
                        if (param->get_status()==RPS_MODIFIED)
                        {
                            QVector<radarParamPointer> params = get_param_group(param);
                            if ((params[0]->get_io_type() != RPT_IO_INPUT)&&(params[0]->get_type()!=RPT_VOID))
                            {
                                if (!_params_to_modify.contains(params[0]))
                                {
                                    _n_params_to_receive+=params.length();
                                    _params_to_modify.append(params[0]);
                                }
                            }
                        }
                }
        }
    }

    for (auto& varname : varlist)
    {
#ifdef INTERFACE_STRUCT
        // Skip struct
        if (varname == get_device_name().toStdString()) continue;
#endif
        for (auto& param:_params)
        {
            if (get_mapped_name(param).toStdString() == varname)
            {
                RP_STATUS status = param->get_status();
                if (status==RPS_MODIFIED)
                    update_variable(varname);
            }
        }
    }
}
//--------------------------------------------------
bool    radarInstance::transmit_command(radarParamPointer param)
{
    QVector<radarParamPointer> params = get_param_group(param);

    if (params.count()==0)
        return false;

   return transmit_param_blocking(params, _serialport);
}

//--------------------------------------------------
bool    radarInstance::inquiry_parameter(radarParamPointer param)
{
    QVector<radarParamPointer> params = get_param_group(param);

    if (params.count()==0)
        return false;

    // Check if the parameter is already in the param
    if ((params[0]->get_io_type()!=RPT_IO_INPUT)&&(params[0]->has_inquiry_value()))
    {
        if (!_params_to_inquiry.contains(params[0]))
        {
            _n_params_to_receive+=params.length();
            _params_to_inquiry.append(params[0]);
        }
    }

    if (params[0]->get_io_type()==RPT_IO_OUTPUT)
    {
        if (!_params_to_inquiry.contains(params[0]))
        {
            _n_params_to_receive+=params.length();
            _params_to_inquiry.append(params[0]);
        }
    }


	bool bOk =  transmit_param_blocking(params, true);
    _params_to_inquiry.removeAll(params[0]);
    return bOk;
}


//--------------------------------------------------
bool    radarInstance::transmit_custom(const QByteArray& tx)
{
    if (!is_connected()) return false;

    if (_serial_status==SS_TRANSMITTING)
        return false;

    _serialport->clear(QSerialPort::Output);

    transmit_data(tx);
    return true;
}


/*
 * Serial port signals handlers
 * */
//--------------------------------------------------
bool    radarInstance::is_connected()
{
    if (_serialport==nullptr) return false;
    return _serialport->isOpen();
}
//--------------------------------------------------
void    radarInstance::disconnect()
{
    for (auto& p: _params)
        if (p!=nullptr) p->set_status(RPS_IDLE);

    if (_serialport==nullptr) return;
    if (!is_connected()) return;
    _serialport->clear();
    _serialport->flush();
    _serialport->close();
    emit disconnection();
}
//--------------------------------------------------

void    radarInstance::port_connect()
{
    for (auto& p: _params)
        if (p!=nullptr) p->set_status(RPS_IDLE);

    if (is_connected()) return;

    if (_serialport==nullptr) initSerialPortTimer();
    if (_serialport==nullptr) return;
    _serialport->open(QIODevice::ReadWrite);
    _serial_status = SS_IDLE;
    if (_serialport->isOpen())
    {
        _serialport->clear();
        _serialport->flush();
    }
}
//--------------------------------------------------

bool    radarInstance::transmit_and_wait_answer(QByteArray& datatx)
{
    if (_serial_status != SS_IDLE)
        return false;

    if (!is_connected()) return false;

    _last_serial_tx = datatx;
    _serialport->write(datatx);

    wait_for_answer();

    _serial_status = SS_IDLE;
    return true;
}

//--------------------------------------------------
void    radarInstance::serial_error(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
    {
        _serial_status = SS_ERROR;
        _last_error_serial = QString("Error during transmission: ")+_serialport->errorString();
        _serialport->clearError();
        if (error == QSerialPort::ResourceError)
            disconnect();
        emit serial_error_out(_last_error_serial);
    }
}

//--------------------------------------------------

void    radarInstance::transmit_data(const QByteArray& datatx)
{
    if (!is_connected()) return;
    if (_serial_status == SS_TRANSMITTING)
        return;
    _serial_status = SS_TRANSMITTING;
    _last_serial_tx = datatx;

    _serialport->clear();

    const qint64 written = _serialport->write(datatx);

    if (written == datatx.size()) {
        _bytesToWrite += written;

#ifndef SERIAL_CHECK_TX
        _serial_status = SS_RECEIVING;
        emit new_write_completed(_last_serial_tx);
        _serial_timer->start(kWriteTimeoutRx);
#else
        _serial_timer->start(kWriteTimeoutTx);
#endif
    }
    else
    {
        _last_error_serial = tr("Failed to write all data to port %1.\n"
                                  "Error: %2").arg(_serialport->portName(),
                                        _serialport->errorString());
        emit serial_error_out(_last_error_serial);
    }
}
//--------------------------------------------------

void    radarInstance::serial_timeout()
{
    for (auto& param: _params)
    {
        if (param!=nullptr)
            param->set_status(RPS_IDLE);
    }

    if (_serial_status == SS_RECEIVING)
    {
        emit rx_timeout("Timeout error during data receiption");
    }

    if (_serial_status == SS_TRANSMITTING)
    {
        emit tx_timeout("Timeout error during data transmission");
    }

    if (_serialport->isOpen())
    {
        _serialport->flush();
        _serialport->clear();
    }

    _serial_status = SS_TIMEOUT;
}

//--------------------------------------------------
void    radarInstance::bytes_written(qint64 bytes)
{
    _bytesToWrite -= bytes;
#ifdef SERIAL_CHECK_TX
    if (_bytesToWrite == 0)
    {
        _serial_timer->stop();
        _serial_status = SS_RECEIVING;
        emit new_write_completed(_last_serial_tx);
        // We are done with a transmission, set the serial into "listening" mode
        _serial_timer->start(kWriteTimeoutRx);
    }
#endif
}

//--------------------------------------------------
void radarInstance::serial_chain_data(const QByteArray& data_readAll)
{
    _left_rx.append(data_readAll);
    int indexOfStop = -1;
    while (1)
    {
        indexOfStop = _left_rx.indexOf(_protocol->get_stop());

        if (indexOfStop >= 0)
        {
            _last_serial_rx = (_left_rx.left(indexOfStop+1));
            _left_rx = _left_rx.last(_left_rx.size()-1-indexOfStop);
            _serial_status = SS_RECEIVEDONE;
            _serial_timer->stop();
            update_param_from_last_rx();

            emit new_read_completed(_last_serial_rx);
        }
        else
            break;
    }
}
//--------------------------------------------------
void    radarInstance::read_data()
{
    serial_chain_data(_serialport->readAll());
}
//--------------------------------------------------

void    radarInstance::wait_for_answer   ()
{
    if (_serial_status != SS_RECEIVING) return;
    _serial_timer->start(kWriteTimeoutRx);
    while (_serial_status == SS_RECEIVING)
    {
        QThread::msleep(1);
    };
}


/*
* Transmit a param in "blocking mode".
* In non-blocking mode, scheduler has to handle signals properly
*/

bool    radarInstance::transmit_param_blocking(const QVector<radarParamPointer>& params, bool inquiry)
{
    _last_sent_command = nullptr;
    if (params.count()==0) return false;
    if (params.isEmpty())
        return false;

    if (!is_connected()) return false;

    _serialport->clear(QSerialPort::Output);    
    _n_params_to_receive+=params.length();
    QByteArray data_tx = encode_param_for_transmission(params,inquiry);
    _serial_status = SS_TRANSMITTING;
    _serialport->write(data_tx);
    if (_serialport->waitForBytesWritten(kWriteTimeoutTx)) {
        _last_serial_tx = data_tx;
        _serial_status = SS_RECEIVING;
    }
    else
    {
        serial_timeout();
        return false;
    }
    emit new_write_completed(_last_serial_tx);

    do
    {
        if (_serialport->waitForReadyRead(kWriteTimeoutRx))
        {
            serial_chain_data(_serialport->readAll());
            if (_serial_status == SS_RECEIVEDONE) return true;
        }
        else
        {
            serial_timeout();
            return false;
        }
    }
    while(1);
    return true;
}

/*
* Transmit a param in "blocking mode".
* In non-blocking mode, scheduler has to handle signals properly
*/
//--------------------------------------------------
bool    radarInstance::transmit_command_blocking(radarParamPointer param)
{
    _last_sent_command = nullptr;
    if (!is_connected()) return false;

    _serialport->clear(QSerialPort::Output);

    QByteArray data_tx = encode_param_for_transmission(QVector<radarParamPointer>({param}));
    _serial_status = SS_TRANSMITTING;
    _serialport->write(data_tx);
    if (_serialport->waitForBytesWritten(kWriteTimeoutTx)) {
        _last_serial_tx = data_tx;
        _serial_status = SS_RECEIVING;
    }
    else
    {
        serial_timeout();
        return false;
    }
    emit new_write_completed(_last_serial_tx);

    if (_serialport->waitForReadyRead(kWriteTimeoutRx))
    {
        // read request
        if (!update_command_from_last_rx()) return false;
        _serial_status = SS_RECEIVEDONE;
        _serial_timer->stop();

        if (_serial_status == SS_RECEIVEDONE) return true;
        while (_serialport->waitForReadyRead(10))
        {
            serial_chain_data(_serialport->readAll());
            if (_serial_status == SS_RECEIVEDONE) return true;
        }
    }
    else
    {
        serial_timeout();
        return false;
    }
    return true;
}

//--------------------------------------------------
bool    radarInstance::transmit_param_non_blocking(const QVector<radarParamPointer>& params, bool inquiry)
{
    _last_sent_command = nullptr;
    if (params.count()==0) return false;
    if (params.isEmpty())
        return false;

    if (!is_connected()) return false;

    _serialport->clear(QSerialPort::Output);

    QByteArray data_tx = encode_param_for_transmission(params,inquiry);
    if (inquiry)
    {
        if (!_params_to_inquiry.contains(params[0]))
        {
            _n_params_to_receive+=params.length();
            _params_to_inquiry.append(params[0]);
        }
    }
    else
    {
        if (!_params_to_modify.contains(params[0]))
        {
            _n_params_to_receive+=params.length();
            _params_to_modify.append(params[0]);
        }
    }

    transmit_data(data_tx);

    if (inquiry)
        _params_to_inquiry.removeAll(params[0]);
    else
        _params_to_modify.removeAll(params[0]);
    return true;
}
//---------------------------------------------------
void radarInstance::emit_operation_done()
{
    if (_radar_operation == RADAROP_INIT_PARAMS)    {emit init_params_done(); return;}
    if (_radar_operation == RADAROP_INIT_SCRIPTS)   {emit init_scripts_done(); return;}
    if (_radar_operation == RADAROP_POSTACQ_PARAMS) {emit postacq_params_done(); return;}
    if (_radar_operation == RADAROP_POSTACQ_SCRIPTS) {emit postacq_scripts_done(); return;}
    _radar_operation = RADAROP_NONE;

}
//---------------------------------------------------
void    radarInstance::param_is_updated(radarParamPointer param)
{
    // We have an updated parameter
   // if ((_params_to_inquiry.isEmpty())&&(_params_to_modify.isEmpty())) return;

    QVector<radarParamPointer> params = get_param_group(param);
    radarParamPointer ref = params[0];
    // Store it as updated
    //if (((_params_to_inquiry.contains(param))||(_params_to_modify.contains(param)))&&(param->is_linked_to_octave()))
    if (param->is_linked_to_octave())
    {
        if (!_params_updated.contains(ref))
            _params_updated.append(ref);
    }
    _n_params_to_receive--;
    if (_n_params_to_receive==0)
    {
#ifndef UPDATE_INSIDE

        for (auto& param : _params_updated)
            if ((param!=nullptr)&&(param->is_linked_to_octave()))
            {
                _workspace->set_variable_no_immediate_update(get_mapped_name(param).toStdString(),   param->get_value());
                _workspace->add_variable_to_updatelist(get_mapped_name(param).toStdString());
            }

        _workspace->workspace_to_interpreter_noautolist();
        _workspace->update_after_set_variables();
#endif
        emit all_params_updated();
    // Check operations
        emit_operation_done();
    }
    else
    {
        // We should inquiry the next parameter
        // If params_modified[0] belongs to the same group as the param just received, we don't need to add it to the
        // inquiry list
        if (!param->is_command_group())
        {
            if (!_params_to_inquiry.isEmpty())
            {
                inquiry_parameter(_params_to_inquiry[0]); return;
            }
            if (!_params_to_modify.isEmpty())
            {
				transmit_param_blocking(get_param_group(_params_to_modify[0]), false); return;
            }

            return;
        }

        if (param->is_command_group())
        {
            QByteArray ref = param->get_command_group();
            bool bfound_inquiry = false;
            bool bfound_modify  = false;

            for (auto &p : _params_to_inquiry)
                if (p!=nullptr)
                {
                    if (p->get_command_group()==ref)
                    {bfound_inquiry = true; break;}
                }
            if (!bfound_inquiry)
            {
                for (auto &p : _params_to_modify)
                    if (p!=nullptr)
                    {
                        if (p->get_command_group()==ref)
                        {bfound_modify = true; break;}
                    }
            }
            if (!bfound_inquiry)
            {
                if (!_params_to_inquiry.isEmpty())
                    inquiry_parameter(_params_to_inquiry[0]);
            }

            if (!bfound_modify)
            {
                // Move to the next element in the queue
                if (!_params_to_modify.isEmpty())
					transmit_param_blocking(get_param_group(_params_to_modify[0]), false);
            }
        }
    }
}
//--------------------------------------------------
bool    radarInstance::update_command_from_last_rx()
{
    if (_last_serial_rx.isEmpty()) return false;

    if (_protocol==nullptr) return false;

    QByteArray decoded  = _protocol->decode_rx(_last_serial_rx);

    if (decoded.isEmpty()) return false;

    for (auto & param : _params)
        if (param!=nullptr)
        {
            int expected_command_length = param->get_command_group().length();

            QByteArray received_command = decoded.left(expected_command_length);

            if (received_command!=param->get_command_group())
                continue;

            param->set_status(RPS_RECEIVED);
            return true;
        }

    return false;

}
//--------------------------------------------------
bool    radarInstance::update_param_from_last_rx()
{
    if (_last_serial_rx.isEmpty()) return false;

    if (_protocol==nullptr) return false;

    QByteArray decoded  = _protocol->decode_rx(_last_serial_rx);

    if (decoded.isEmpty()) return false;

    for (auto & param : _params)
        if (param!=nullptr)
        {
            int expected_command_length = param->get_command_group().length();

            QByteArray received_command = decoded.left(expected_command_length);

            if (received_command!=param->get_command_group())
                continue;


            QByteArray payload  = decoded.right(decoded.length()-expected_command_length);

            // If we sent a command, we need to consider this
            if (payload.isEmpty())
            {
                if ((param->get_io_type()==RPT_IO_INPUT)||(param->get_type()==RPT_VOID))
                {
                    param_is_updated(param);
                    param->set_status(RPS_RECEIVED);

                }
                return true;
            }

            if (param->is_command_group())
            {
                QVector<radarParamPointer> params = get_param_group(param);
                int np = 0;
                for (const auto& param_of_group: params)
                {
                    param_of_group->set_status(RPS_RECEIVED);
                    np++;
                    int expected_size = param_of_group->get_expected_payload_size();
                    if (expected_size == 0) continue;

                    if (np < params.count())
                    {
                        expected_size = param_of_group->get_expected_payload_size();

                        if (!param_of_group->split_values(payload.left(expected_size)))
                        { // Clean up the status param group, if something wrong
                            for ( auto& pg: params)
                                if (pg!=nullptr) pg->set_status(RPS_IDLE);

                            return false;
                        }
                        payload = payload.right(payload.length()-expected_size);

                        param_is_updated(param_of_group);
                        update_param_workspace_gui(param_of_group);

                        continue;
                    }

                    // This is variable payload: it must be at the end of the packet.
                    if(!param_of_group->split_values(payload))
                    { // Clean up the status param group, if something wrong
                        for ( auto& pg: params)
                            if (pg!=nullptr) pg->set_status(RPS_IDLE);

                        return false;
                    }

                    param_is_updated(param_of_group);
                    update_param_workspace_gui(param_of_group);
                    param_of_group->set_status(RPS_IDLE);

                }

                _last_sent_command = nullptr;
                return true;
            }
            else
            {
                param->set_status(RPS_RECEIVED);
                int expected_size = param->get_expected_payload_size();
                if (expected_size == 0)
                    continue;

                if (!param->split_values(payload))
                {
                    param->set_status(RPS_IDLE);
                    return false;
                }

                param_is_updated(param);
                update_param_workspace_gui(param);

                _last_sent_command = nullptr;
                param->set_status(RPS_IDLE);
                return true;
            }
        }

    return false;
}


void radarInstance::update_param_workspace_gui(radarParamPointer param/*, bool update_workspace*/)
{
    if (param==nullptr) return;
    if (param->get_type()==RPT_VOID)
        return;
    if (param->get_io_type()==RPT_IO_INPUT) return;
    // To be done
    if (_workspace!=nullptr)
    {
        if (param->get_type() != RPT_VOID)
            if (param->is_linked_to_octave())
            {
                _workspace->set_variable(get_mapped_name(param).toStdString(),   param->get_value());
            }
    }

    // This signal should be linked to GUI
    emit param_updated(param);

    param->set_status(RPS_IDLE);
}
