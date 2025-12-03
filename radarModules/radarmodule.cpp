/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "radarmodule.h"
#include "qserialportinfo.h"
#include "serialport_utils.h"
#include <octave.h>
#include <ovl.h>
#include <ov.h>
#include <QFileInfo>
#include <octavews.h>
#include <radarparameter.h>
#include <radarinstance.h>
#include <ariautils.h>
#include <QSerialPort>
/*
 * Radar Module
 * */

extern QString cstr_radar_devices;
extern QString cstr_scripts;
extern QString cstr_handler;
extern QString cstr_radar_modules;
extern QString cstr_antenna_models;
extern QString cstr_comm_protocol;

//---------------------------------------------
radarModule::radarModule(QString filename,projectItem *parent ) : projectItem("newRadarModule",DT_RADARMODULE, parent),
    _module_name("newRadarModule"),
    _ant(),
    _params(),
    _rfile(),
    _txrx_scheme(),
    _phases(),
    _freqs(),
    _azimuth(),
    _zenith(),
    _equivalent_array_antenna(),
    _freq_of_interest(-1),
    _protocol(nullptr),
    _init_commands(),    
    _postacquisition_commands()

{
    _serial_port_info ={
        115200,
        QSerialPort::Data8,
        QSerialPort::NoParity,
        QSerialPort::OneStop,
        QSerialPort::NoFlowControl
    };
    _expected_serial_description = "";
    _expected_serial_manufacturer= "";
    _expected_serial_number      = "";
    _expected_vendorid           = 0;
    _expected_productid          = 0;
    set_filename(filename);

    _inquiry_moduleid_command           = "";
    _inquiry_moduleid_expected_value= "";
    // Create a default protocol
    if (_protocol==nullptr)
    {
        _protocol = std::make_shared<interfaceProtocol>();
        _protocol->set_stuffing();
        _protocol->set_start(0xFF);
        _protocol->set_stop(0x00);
        _protocol->set_alternate_start_char(0xFE);
        _protocol->set_alternate_stuff_char(0x81);
        _protocol->set_alternate_stop_char(0x01);
        _protocol->set_stuff(0x80);
    }

}

radarModule::radarModule(radarModule& module) :
    _module_name("newRadarModule"),
    _ant(),
    _params(),
    _rfile(),
    _txrx_scheme(),
    _phases(),
    _freqs(),
    _azimuth(),
    _zenith(),
    _equivalent_array_antenna(),
    _freq_of_interest(-1),
    _protocol(module._protocol),
    _init_commands(),
    _postacquisition_commands()
{
    operator = (module);
    if (_protocol==nullptr)
    {
        _protocol = std::make_shared<interfaceProtocol>();
        _protocol->set_stuffing();
        _protocol->set_start(0xFF);
        _protocol->set_stop(0x00);
        _protocol->set_stuff(0x80);
        _protocol->set_alternate_start_char(0xFE);
        _protocol->set_alternate_stuff_char(0x81);
        _protocol->set_alternate_stop_char(0x01);
    }
}



void radarModule::set_filename(QString filename)
{
    QFileInfo fi(filename);
    _filename = fi.fileName();
    _rfile.setFileName(filename);
    if (filename.isEmpty())
        set_name("[noname]");
    else
        set_name(_filename);
}
//---------------------------------------------
radarModule::radarModule(QString name, dataType type) : projectItem(name,type)
{
}

radarModule::~radarModule()
{
    clear_antenna_table();
    clear_param_table();
    clear_txrx_pairs();
    clear_commands_tables();
    clear_scripts_tables();
}
//---------------------------------------------
void radarModule::set_name(QString name)
{
    projectItem::set_name(name);
    _module_name = name;
}
//---------------------------------------------
bool radarModule::load_file(QString filename)
{
    if (!filename.isEmpty())
        set_filename(filename);

    if (!load_xml()) return false;

    return true;
}
//---------------------------------------------
bool radarModule::save_file(QString filename)
{
    if (filename.isEmpty() && _rfile.fileName().isEmpty()) return false;
    if (filename.isEmpty())
        filename = _rfile.fileName();
    else
        set_filename(filename);

    _filename = QFileInfo(filename).fileName();
    return save_xml();
}

//---------------------------------------------
int radarModule::has_param(QString parname)
{
    for (int n=0; n < _params.count(); n++)
        if (_params.at(n)->get_name()==parname) return n;

    return -1;
}
//---------------------------------------------
radarModule&   radarModule::operator = (radarModule& m2)
{
    projectItem::operator=(m2);
    _module_name = m2._module_name;

    _ant.resize(m2._ant.count());
    for (int n=0; n < m2._ant.count(); n++)
    {
        _ant[n] = new antennaInstance( *(m2._ant[n]));
    }
    //_ant         = m2._ant;
    //_params      = m2._params;

    int nparams = 0;
    for (const auto& param : m2._params)
        if (param!=nullptr)
            nparams++;

    _params.resize(nparams);

    int n=0;
    for (const auto& param : m2._params)
        if (param!=nullptr)
            _params[n++] = param->clone();

    _protocol    = m2._protocol;
    _txrx_scheme = m2._txrx_scheme;
    _phases     = m2._phases;
    _freqs      = m2._freqs;
    _azimuth    = m2._azimuth;
    _zenith     = m2._zenith;
    _equivalent_array_antenna = m2._equivalent_array_antenna;
    _freq_of_interest = m2._freq_of_interest;
    _init_commands = m2._init_commands;
    _postacquisition_commands = m2._postacquisition_commands;
    _init_commands        = m2._init_commands;
    _postacquisition_commands = m2._postacquisition_commands;
    copy_scripts(_init_scripts, m2._init_scripts);
    copy_scripts(_post_acquisition_scripts, m2._post_acquisition_scripts);

    set_filename(m2.get_full_filepath());

    _serial_port_info = m2._serial_port_info;
    _expected_serial_description = m2._expected_serial_description;
    _expected_serial_manufacturer= m2._expected_serial_manufacturer;
    _expected_serial_number      = m2._expected_serial_number;
    _expected_vendorid           = m2._expected_vendorid;
    _expected_productid          = m2._expected_productid;

    _inquiry_moduleid_command           = m2._inquiry_moduleid_command;
    _inquiry_moduleid_expected_value= m2._inquiry_moduleid_expected_value;
    _inquiry_instanceid_command = m2._inquiry_instanceid_command;
    return *this;
}
//---------------------------------------------
int            radarModule::get_param_count()
{
    return _params.count();
}
//---------------------------------------------
radarParamPointer  radarModule::get_param(QString name)
{
    for (int n=0; n < _params.count(); n++)
        if (_params.at(n)->get_name()==name)
            return _params.at(n);

    return nullptr;
}
//---------------------------------------------
radarParamPointer      radarModule::get_param(int id)
{
    if ((id<0)||(id>=_params.count()))
        return nullptr;

    return _params.at(id);
}

//---------------------------------------------
void  radarModule::set_param(int id, radarParamPointer& param)
{
    if ((id<0)||(id>=_params.count())) return;
    _params[id]=param;

}
//---------------------------------------------
void radarModule::append_param(radarParamPointer par)
{
    if (par!=nullptr)
        _params.append(par);
}
//---------------------------------------------
void radarModule::remove_param(int id)
{
    if (_params[id]!=nullptr) _params[id].reset();
    _params.remove(id);
}
//---------------------------------------------
void radarModule::remove_param(const QString& param_name)
{
    radarParamPointer ptr = get_param(param_name);
    if (ptr == nullptr) return;
    _params.removeAll(ptr);
    ptr.reset();

}
//---------------------------------------------
void radarModule::clear_param_table()
{
    for (int n=0; n<_params.count(); n++)
        _params[n].reset();

    _params.clear();
    set_inquiry_moduleid_command("");
    set_inquiry_moduleid_expectedvalue_as_string("");
}
//---------------------------------------------
void radarModule::clear_antenna_table()
{
    for (int n=0; n<_ant.count(); n++)
        if (_ant[n] != nullptr) delete _ant[n];

    _ant.clear();
}
//---------------------------------------------
void radarModule::clear_commands_tables()
{
    _init_commands.clear();
    _postacquisition_commands.clear();
}
//---------------------------------------------
void radarModule::clear_scripts_tables()
{
    _init_scripts.clear();
    _post_acquisition_scripts.clear();

}
//---------------------------------------------
void radarModule::copy_scripts(QVector<octaveScript_ptr>& dest, const QVector<octaveScript_ptr>& source)
{
    /*
    for (int n=0; n < dest.count(); n++)
        delete dest[n];*/
    dest.clear();

    dest.resize(source.count());

    for (int n=0; n < source.count(); n++)
        dest[n] =source[n];

}
//---------------------------------------------
void radarModule::append_copy_of_param(radarParamPointer& param)
{
    _params.append(param->clone());
}
//---------------------------------------------
void radarModule::set_param_copy(int id, radarParamPointer& param)
{
    if ((id<0)||(id>=_params.count())) return;
    _params[id] = param->clone();
}

//---------------------------------------------
void radarModule::set_clear_and_copy(int id, radarParamPointer& param)
{
    if ((id<0)||(id>=_params.count())) return;
    _params[id].reset();
    _params[id] = param->clone();
}

//---------------------------------------------
antennaInstance radarModule::calculate_focusing_direction(double target_theta, double target_phi, QVector<QPair<int,int>> txrx)
{
    antennaInstance out;
    equalize_antenna_supports();
    NDArray direction_versor_cartesian = polar_to_cartesian(1.0, target_phi, target_theta);


    for (int pair = 0; pair < txrx.count(); pair++)
    {
        NDArray position = _ant[txrx[pair].first]->get_phase_center().array_value();

    }

    return out;
}

//---------------------------------------------
void radarModule::equalize_antenna_supports()
{
    if (!require_normalization()) return;
    for (int t=1; t < _ant.count(); t++)
        _ant[t]->resample_antennas(*(_ant[0]), true);
}

//---------------------------------------------
bool radarModule::require_normalization()
{

    for (int r = 0; r < _ant.count(); r++)
        if (!_ant[0]->has_same_support(*(_ant[r]))) return true;

    return false;
}

//---------------------------------------------
antenna_array   radarModule::get_antenna_array()
{
    return _ant;
}
//---------------------------------------------
int             radarModule::get_antenna_count()
{
    return _ant.count();
}
//---------------------------------------------
antenna_pointer radarModule::get_antenna(int antennaid)
{
    antenna_pointer out = nullptr;
    if ((antennaid < 0)||(antennaid>=_ant.count()))
        return out;

    out = _ant[antennaid];
    return out;
}
//---------------------------------------------
void radarModule::set_antenna_model(int antennaid, antenna_model_pointer model)
{
    if ((antennaid < 0)||(antennaid>=_ant.count())) return;
    _ant[antennaid]->set_antenna_model(model);

}

//---------------------------------------------
bool radarModule::save_xml()
{
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

//----------------------------------------------
// This function returns the absolute path
// given a new basepath
QString radarModule::absolute_path(QString basepath)
{
    QFileInfo fi(get_name());
    QDir      current_rel_path(fi.path());

    return current_rel_path.cleanPath(current_rel_path.absoluteFilePath(basepath));
}

//---------------------------------------------
bool radarModule::operator == (const radarModule& module2)
{
    if (_module_name != module2._module_name) return false;

    // Strong comparison
    //if (_ant == module2._ant) return false;

    // Let's enforce a weak comparison where pointers must be the same to assert the equivalence
    if (_ant.count() != module2._ant.count()) return false;
    for (int j = 0; j < _ant.count(); j++)
        if (!(_ant[j]==module2._ant[j])) return false;

    if (_inquiry_moduleid_command != module2._inquiry_moduleid_command) return false;
    if (_inquiry_moduleid_expected_value != module2._inquiry_moduleid_expected_value) return false;
    if (_inquiry_instanceid_command != module2._inquiry_instanceid_command) return false;

    // Weak comparison
    if (_params.count() != module2._params.count()) return false;

    for (int j=0; j < _params.count(); j++)
        if (!(*(_params[j])==(*(module2._params[j]))))
            return false;

    if (_txrx_scheme!=module2._txrx_scheme) return false;
    if (_phases != module2._phases) return false;

    if (!is_equal(_freqs, module2._freqs)) return false;
    if (!is_equal(_azimuth, module2._azimuth)) return false;
    if (!is_equal(_zenith, module2._zenith)) return false;

    if (_init_commands != module2._init_commands) return false;
    if (_postacquisition_commands != module2._postacquisition_commands) return false;


    if (_init_scripts.count() !=module2._init_scripts.count()) return false;

    for (int j=0; j < _init_scripts.count(); j++)
        if (!((*(_init_scripts[j]))==(*(module2._init_scripts[j]))))
            return false;


    if (_post_acquisition_scripts.count() !=module2._post_acquisition_scripts.count()) return false;

    for (int j=0; j < _post_acquisition_scripts.count(); j++)
        if (!((*(_post_acquisition_scripts[j]))==(*(module2._post_acquisition_scripts[j]))))
            return false;


    if (!(_serial_port_info==module2._serial_port_info)) return false;

    if (_expected_serial_description!=module2._expected_serial_description) return false;

    if (_expected_serial_manufacturer!=module2._expected_serial_manufacturer) return false;

    if (_expected_serial_number != module2._expected_serial_number) return false;

    if (_expected_vendorid != module2._expected_vendorid) return false;

    if (_expected_productid != module2._expected_productid) return false;

    return true;
}
//---------------------------------------------
bool radarModule::save_xml(QDomDocument& document)
{
    QDomElement root = document.createElement("ARIA_Radar_Module");
    QVersionNumber xml_ver({XML_RADARFILE_MAJOR,XML_RADARFILE_MINOR,XML_RADARFILE_SUBVER});
    root.setAttribute("xml_dataformat", xml_ver.toString());
    root.setAttribute("radar_module_name",_module_name);


    document.appendChild(root);

    QDomElement parameter_list = document.createElement("parameter_list");
    parameter_list.setAttribute("count",_params.count());

    parameter_list.setAttribute("inquiry_moduleid_command",get_inquiry_moduleid_command_as_string());
    parameter_list.setAttribute("expected_moduleid_value",get_inquiry_moduleid_expectedvalue_as_string());
    parameter_list.setAttribute("inquiry_instanceid_command",get_inquiry_instanceid_command_as_string());

    for (int n=0; n < _params.count(); n++)
        _params[n]->save_xml(document, parameter_list);

    root.appendChild(parameter_list);

    QDomElement antenna_root = document.createElement("antennas");
    antenna_root.setAttribute("count",_ant.count());

    for (int i = 0; i < _ant.count(); i++)
    {
        antenna_pointer ant = _ant[i];
//        ant->rebase_dir_samedest(get_base_dir());
        ant->save_xml(document, antenna_root);
    }
    root.appendChild(antenna_root);

    if (_txrx_scheme.count()>0)
    {
        QDomElement txrx_list = document.createElement("txrx_pairs");
        txrx_list.setAttribute("count", _txrx_scheme.count());
        for (int n=0; n < _txrx_scheme.count(); n++)
        {
            QDomElement txrx_pair = document.createElement("txrx_pair");
            txrx_pair.setAttribute("ant1",_txrx_scheme[n].first);
            txrx_pair.setAttribute("ant2",_txrx_scheme[n].second);
            txrx_list.appendChild(txrx_pair);
        }
        root.appendChild(txrx_list);
    }

    QDomElement init_params = document.createElement("init_parameters");
    init_params.setAttribute("count",_init_commands.count());

    for (int n=0; n < _init_commands.count(); n++)
    {
        QDomElement command = document.createElement("command");
        command.setAttribute("order", n);
        command.setAttribute("param_id", _init_commands[n]);

        init_params.appendChild(command);
    }

    root.appendChild(init_params);

    QDomElement postacquisition_params = document.createElement("post_acquisition_parameters");
    postacquisition_params.setAttribute("count",_postacquisition_commands.count());

    for (int n=0; n < _postacquisition_commands.count(); n++)
    {
        QDomElement command = document.createElement("command");
        command.setAttribute("order", n);
        command.setAttribute("param_id", _postacquisition_commands[n]);

        postacquisition_params.appendChild(command);
    }

    root.appendChild(postacquisition_params);

    QDomElement init_scripts_root = document.createElement("init_scripts");
    init_scripts_root.setAttribute("count",_init_scripts.count());

    for (int i = 0; i < _init_scripts.count(); i++)
    {
//        _init_scripts[i]->rebase_dir_samedest(get_base_dir());
        _init_scripts[i]->save_xml(document,init_scripts_root);
    }

    root.appendChild(init_scripts_root);

    QDomElement post_acq_scripts_root = document.createElement("post_acquisition_scripts");
    post_acq_scripts_root.setAttribute("count",_post_acquisition_scripts.count());

    for (int i = 0; i < _post_acquisition_scripts.count(); i++)
    {
//        _acquisition_scripts[i]->rebase_dir_samedest(get_base_dir());
        _post_acquisition_scripts[i]->save_xml(document,post_acq_scripts_root);
    }

    root.appendChild(post_acq_scripts_root);

    QDomElement serial_port = document.createElement("serial_port_configuration");
    serial_port.setAttribute("baud",QString::number(_serial_port_info.baudRate));
    serial_port.setAttribute("parity",serial_port_parity_descriptor.value(_serial_port_info.parity));
    serial_port.setAttribute("databits",QString::number(_serial_port_info.dataBits));
    serial_port.setAttribute("stopbits",serial_port_stopbits_descriptor.value(_serial_port_info.stopBits));
    serial_port.setAttribute("flowcontrol",serial_port_flowcontrol_descriptor.value(_serial_port_info.flowControl));

    serial_port.setAttribute("description",_expected_serial_description);
    serial_port.setAttribute("manufacturer",get_serial_manufacturer());
    serial_port.setAttribute("serial_number",get_serial_number());
    serial_port.setAttribute("vendorid", get_serial_vendorid());
    serial_port.setAttribute("productid", get_serial_productid());

    root.appendChild(serial_port);
    return true;
}
//--------------------------------------------
void radarModule::append_antenna(antenna_pointer antenna)
{
    //((radarProject*)(get_root()))->add_antenna()
    _ant.append(antenna);
}
//--------------------------------------------
void radarModule::append_antenna(antenna_model_pointer model, QString name, double pos_x, double pos_y, double pos_z, double rot_theta, double rot_phi, double loss, double delay)
{
    antenna_pointer new_antenna = new antennaInstance();
    new_antenna->set_name(name);
    new_antenna->set_antenna_model(model);
    new_antenna->set_position(0, pos_x);
    new_antenna->set_position(1, pos_y);
    new_antenna->set_position(2, pos_z);
    new_antenna->set_rotation(true, rot_phi);
    new_antenna->set_rotation(false, rot_theta);
    new_antenna->set_loss(loss);
    new_antenna->set_delay(delay);
    _ant.append(new_antenna);
}

//---------------------------------------------
bool radarModule::load_xml()
{
    QDomDocument document;

    if (!_rfile.open(QIODevice::ReadOnly ))
    {
        return false;
    }
    document.setContent(&_rfile);
    _rfile.close();
    QDomElement root = document.firstChildElement("ARIA_Radar_Module");
    if (root.isNull()) return false;

    QString xml_ver_string = root.attribute("xml_dataformat");

    QVersionNumber xml_ver = QVersionNumber::fromString(xml_ver_string);

    QVersionNumber current_xml({XML_RADARFILE_MAJOR,XML_RADARFILE_MINOR, XML_RADARFILE_SUBVER});

    if (xml_ver > current_xml)
        return false;

    _module_name = root.attribute("radar_module_name");

    clear_param_table();

    QDomElement param_root = root.firstChildElement("parameter_list");

    if (!param_root.isNull())
    {
        bool bOk = false;
        int param_count = param_root.attribute("count").toInt(&bOk);
        set_inquiry_moduleid_command_as_string(param_root.attribute("inquiry_moduleid_command",""));
        set_inquiry_moduleid_expectedvalue_as_string(param_root.attribute("expected_moduleid_value",""));
        set_inquiry_instanceid_command_as_string(param_root.attribute("inquiry_instanceid_command",""));

        if (!bOk) return false;

        QDomElement param_elem = param_root.firstChildElement("parameter");

        while (!param_elem.isNull())
        {
            radarParamPointer p_param = radarParamBase::create_from_xml_node(document,param_elem);
            if (p_param == nullptr)
                break;
             _params.append(p_param);
        }
        if (_params.count()!=param_count) return false;
    }

    clear_antenna_table();

    QDomElement antenna_root = root.firstChildElement("antennas");

    if (!antenna_root.isNull())
    {
        //bool bOk = false;
        //int antenna_count = antenna_root.attribu  te("count").toInt(&bOk);
        //if (!bOk) return false;

        QDomElement antenna_elem = antenna_root.firstChildElement("antenna");
        int nnoname= 0;
        while (!antenna_elem.isNull())
        {
            // Model file is the relative?
            QString model_file = antenna_elem.attribute("model_file");

            antenna_pointer new_antenna =antennaInstance::load_xml(document, antenna_elem);
            if (new_antenna == nullptr)
                return false;

            antenna* model = (antenna*)(get_root()->get_child(model_file,DT_ANTENNA));
            new_antenna->set_antenna_model(model);
            new_antenna->set_antenna_name(antenna_elem.attribute("name",QString("noname")+QString::number(nnoname++)));

            _ant.append(new_antenna);
            //antenna_elem = antenna_elem.nextSiblingElement("antenna");
        }

        //if (_ant.count()!=antenna_count) return false;
    }

    _txrx_scheme.clear();
    QDomElement txrx_pairs = root.firstChildElement("txrx_pairs");

    if (!txrx_pairs.isNull())
    {
        bool bOk;
        int expected_pair_count = txrx_pairs.attribute("count").toInt(&bOk);
        if (!bOk) return false;
        QDomElement txrx = txrx_pairs.firstChildElement("txrx_pair");

        while (!txrx.isNull())
        {
            int ant1 = txrx.attribute("ant1").toInt(&bOk);
            if (!bOk)
                return false;
            int ant2 = txrx.attribute("ant2").toInt(&bOk);
            if (!bOk)
                return false;

            _txrx_scheme.append(QPair<int,int>(ant1,ant2));

            txrx = txrx.nextSiblingElement("txrx_pair");
        }

        if (_txrx_scheme.count()!=expected_pair_count)
            return false;
    }

    // Load params (init, acq, post_acq)
    clear_commands_tables();
    bool bOk;
    QDomElement params = root.firstChildElement("init_parameters");

    if (!params.isNull())
    {
        int num_init_params = params.attribute("count","0").toInt(&bOk);

        if (!bOk) return false;

        QDomElement command_elem = params.firstChildElement("command");
        _init_commands.resize(0);
        int n_good_params = 0;
        while (!command_elem.isNull())
        {
            int command_order = command_elem.attribute("order","-1").toInt(&bOk);
            if (!bOk) return false;
            int command_id = command_elem.attribute("param_id","-1").toInt(&bOk);

            if ((command_order<0)||(command_order>=num_init_params)) return false;
            if ((command_id<0)||(command_id>=_params.count())) return false;

            _init_commands.append(command_id);

            command_elem = command_elem.nextSiblingElement("command");
            n_good_params++;
        }

        if (n_good_params!=num_init_params) return false;
    }

    params = root.firstChildElement("post_acquisition_parameters");

    if (!params.isNull())
    {
        int num_params = params.attribute("count","0").toInt(&bOk);

        if (!bOk) return false;

        QDomElement command_elem = params.firstChildElement("command");
        _postacquisition_commands.resize(0);
        int n_good_params = 0;
        while (!command_elem.isNull())
        {
            int command_order = command_elem.attribute("order","-1").toInt(&bOk);
            if (!bOk) return false;
            int command_id = command_elem.attribute("param_id","-1").toInt(&bOk);

            if ((command_order<0)||(command_order>=num_params)) return false;
            if ((command_id<0)||(command_id>=_params.count())) return false;
            _postacquisition_commands.append(command_id);

            command_elem = command_elem.nextSiblingElement("command");
            n_good_params++;
        }

        if (n_good_params!=num_params) return false;
    }

    // and scripts

    QDomElement scripts = root.firstChildElement("init_scripts");

    if (!scripts.isNull())
    {
        //int num_scripts = scripts.attribute("count","0").toInt(&bOk);
        QDomElement script = scripts.firstChildElement("script");
        _init_scripts.resize(0);

        do
        {
            QString script_file = script.attribute("filename");
            if (!script_file.isEmpty())
            {
                octaveScript_ptr octave_script = (octaveScript*)(get_root()->get_child(QFileInfo(script_file).fileName(),DT_SCRIPT));
                if (octave_script!=nullptr)
                    _init_scripts.append(octave_script);
                else
                {   // Create a new placeholder script
                    octaveScript* newScript = new octaveScript (get_root()->get_folder(cstr_scripts)->get_full_path()
                                                                +QFileInfo(script_file).fileName(),get_root()->get_folder(QString(cstr_scripts)));
                    get_root()->add_script(newScript);
                    _init_scripts.append(newScript);
                }
            }

            script = script.nextSiblingElement("script");
        }
        while(!script.isNull());
    }


    scripts = root.firstChildElement("post_acquisition_scripts");

    if (!scripts.isNull())
    {
        //int num_scripts = scripts.attribute("count","0").toInt(&bOk);
        QDomElement script = scripts.firstChildElement("script");
        _post_acquisition_scripts.resize(0);
        do
        {
            QString script_file = script.attribute("filename");
            if (!script_file.isEmpty())
            {

                octaveScript_ptr octave_script = (octaveScript*)(get_root()->get_child(QFileInfo(script_file).fileName(),DT_SCRIPT));
                if (octave_script!=nullptr)
                    _post_acquisition_scripts.append(octave_script);
                else
                {   // Create a new placeholder script
                    octaveScript* newScript = new octaveScript (get_root()->get_folder(cstr_scripts)->get_full_path()
                                                               +QFileInfo(script_file).fileName(),get_root()->get_folder(QString(cstr_scripts)));
                    get_root()->add_script(newScript);
                    _post_acquisition_scripts.append(newScript);
                }

            }

            script = script.nextSiblingElement("script");
        }
        while(!script.isNull());

       // if (_post_acquisition_scripts.count()!=num_scripts) return false;
    }


    QDomElement serial_port = root.firstChildElement("serial_port_configuration");

    if (!serial_port.isNull())
    {

        _serial_port_info.baudRate = serial_port.attribute("baud","115200").toInt();

        _serial_port_info.parity   = serial_port_parity_descriptor.key
                                   (serial_port.attribute("parity"), QSerialPort::NoParity);
        _serial_port_info.stopBits = serial_port_stopbits_descriptor.key
                                     (serial_port.attribute("stopbits"), QSerialPort::OneStop);
        _serial_port_info.flowControl = serial_port_flowcontrol_descriptor.key
                                (serial_port.attribute("flowcontrol"), QSerialPort::NoFlowControl);
        int databits = serial_port.attribute("databits","8").toInt();
        if ((databits < 5)||(databits>8)) databits = 8;
        _serial_port_info.dataBits = QSerialPort::DataBits(databits);
        _expected_serial_description = serial_port.attribute("description","");
        QString manufacturer_string = serial_port.attribute("manufacturer","");
        QString serialnumber_string = serial_port.attribute("serial_number","");
        QString vendorid_string = serial_port.attribute("vendorid","");
        QString productid_string = serial_port.attribute("productid","");
        set_expected_serial_port_data(_expected_serial_description, manufacturer_string, serialnumber_string, vendorid_string, productid_string);

    }
    else
    {
        _serial_port_info ={
            115200,
            QSerialPort::Data8,
            QSerialPort::NoParity,
            QSerialPort::OneStop,
            QSerialPort::NoFlowControl
        };
        _expected_serial_description = "";
        _expected_serial_manufacturer= "";
        _expected_serial_number      = "";
        _expected_productid          = -1;
        _expected_vendorid           = -1;
    }

    root.appendChild(serial_port);

    return true;
}

//---------------------------------------------
void    radarModule::set_txrx_pairs_count(int n)
{
    _txrx_scheme.resize(n);
}
//---------------------------------------------
void  radarModule::set_txrx_pair(int npair, int ant1, int ant2)
{
    if ((npair<0)||(npair>=_txrx_scheme.count())) return;
    if ((ant1 < 0)||(ant1>=_ant.count())) return;
    if ((ant2 < 0)||(ant2>=_ant.count())) return;

    _txrx_scheme[npair] = QPair<int,int>(ant1,ant2);

}
//---------------------------------------------
void    radarModule::append_txrx_pair(int ant1, int ant2)
{
    if ((ant1 < 0)||(ant1>=_ant.count())) return;
    if ((ant2 < 0)||(ant2>=_ant.count())) return;

    _txrx_scheme.append(QPair<int,int>(ant1,ant2));
}
//---------------------------------------------
void    radarModule::clear_txrx_pairs()
{
    _txrx_scheme.clear();
}
//---------------------------------------------
QPair<int,int>  radarModule::get_txrx_pair(int npair)
{
    if ((npair<0)||(npair>=_txrx_scheme.count())) return QPair<int,int>(-1,-1);
    return _txrx_scheme[npair];
}
//---------------------------------------------
int radarModule::get_txrx_pairs_count()
{
    return _txrx_scheme.count();
}

//---------------------------------------------
QVector<QPair<int,int>>     radarModule::get_txrx_pairs()
{
    return _txrx_scheme;
}

//---------------------------------------------
void radarModule::set_txrx_pairs(QVector<QPair<int,int>> txrx)
{
    _txrx_scheme = txrx;
}
//---------------------------------------------
void radarModule::remove_txrx_pair(int pair)
{
    if ((pair < 0)||(pair>=_txrx_scheme.count())) return;
    _txrx_scheme.removeAt(pair);
}

//---------------------------------------------
void radarModule::calculate_antenna_planar_waves()
{
    for (int a=0; a < _ant.count(); a++)
        _ant[a]->calculate_plane_wave();
}

//---------------------------------------------
bool radarModule::set_support(NDArray freqs, NDArray azimuths, NDArray zeniths)
{
    if (_ant.count()==0) return false;

    // Resample each antenna
    _freqs      = freqs;
    _azimuth    = azimuths;
    _zenith     = zeniths;
    for (int a=0; a < _ant.count(); a++)
        if (!_ant[a]->calculate_over_freq_azimuth_zenith_support(_freqs, _azimuth, _zenith))
            return false;

    _equivalent_array_antenna.resize_as(*(_ant[0]));

    return true;
}
void radarModule::set_freq_of_interest(int nf)
{
    _freq_of_interest = nf;

    for (int a = 0; a < _ant.count(); a++)
        _ant[a]->set_freq_of_interest(nf);

    _equivalent_array_antenna.set_freq_of_interest(nf);
}
//---------------------------------------------
// This function assume that antenna instances have
// been already calculated. If not, use "set_support" first
void radarModule::calculate_direction_focusing(double a_ref, double z_ref, int nf, bool compensate_delay)
{
    if (_txrx_scheme.count()==0)
        return;

    set_freq_of_interest(nf);

    NDArray vector = polar_to_cartesian(1.0, a_ref, z_ref);

    double k = 2.0*M_PI*_freqs(nf)/_C0;

    antennaInstance ant_i;
    antennaInstance ant_j;
    for (int npair = 0; npair<_txrx_scheme.count(); npair++)
    {
        int a1 = _txrx_scheme[npair].first;
        int a2 = _txrx_scheme[npair].second;
        if ((a1 <0)||(a1>=_ant.count())) return;
        if ((a2 <0)||(a2>=_ant.count())) return;
        antenna_pointer ant1 = _ant[a1];
        antenna_pointer ant2 = _ant[a2];

        if ((ant1==nullptr)||(ant2==nullptr)) return;
        // 1. Ep, Et are calculated at 1m
        double phase_i = k*(dot(ant1->get_phase_center().array_value(), vector)-1);
        double phase_j = k*(dot(ant2->get_phase_center().array_value(), vector)-1);
#ifdef DEBUG_FOI
        qDebug() << "Phase i:" << QString::number(phase_i);
        qDebug() << "Phase j:" << QString::number(phase_j);
#endif

        //ant1->copy_from((*ant1)[phase_i]);

        ant_i.copy_from((*ant1));
        ant_j.copy_from((*ant2));
        ant_i.apply_phase(phase_i);
        ant_j.apply_phase(phase_j);
        ant_i.mult(ant_j);

        if (npair==0)
            _equivalent_array_antenna.copy_from( ant_i);
        else
             _equivalent_array_antenna.add(ant_i);
    }
    calc_gain_equivalent();
}

//---------------------------------------------
void radarModule::calc_gain_equivalent()
{
    _equivalent_array_antenna.calc_gain_equivalent();
}
//---------------------------------------------
NDArray radarModule::gain_equivalent()
{
#ifdef DEBUG_FOI
    qDebug() << "Radar module geq : " << QString::number(_freq_of_interest);
#endif

    NDArray out(
        _freq_of_interest >= 0 ? dim_vector(_azimuth.numel(), _zenith.numel()) :
                                 dim_vector(_azimuth.numel(), _zenith.numel(), _freqs.numel()));

    NDArray gain_eq(_equivalent_array_antenna.gain_equivalent());

    if (_freq_of_interest <0)
    {
        for (int f=0; f < _freqs.numel(); f++)
             for (int a=0; a < _azimuth.numel(); a++)
                for (int z=0; z < _zenith.numel(); z++)
                    out(a,z,f) = (gain_eq(a,z,f)/(double)(_txrx_scheme.count()));
    }
    else
    {
        for (int a=0; a < _azimuth.numel(); a++)
             for (int z=0; z < _zenith.numel(); z++)
                out(a,z) = (gain_eq(a,z,_freq_of_interest)/(double)(_txrx_scheme.count()));
    }

    return out;
}
//---------------------------------------------
void        radarModule::set_init_scripts(QVector<octaveScript_ptr> scripts)
{
    QString base_dir = get_full_path();
    _init_scripts = scripts;
    for (auto &script: _init_scripts)
        if (script!=nullptr)
        {
            script->move_to_new_basedir(base_dir);
        }
}
//---------------------------------------------
void        radarModule:: set_postacquisition_scripts(QVector<octaveScript_ptr> scripts)
{
    QString base_dir = get_full_path();
    _post_acquisition_scripts = scripts;
    for (auto &script: _post_acquisition_scripts)
        if (script!=nullptr)
        {
            script->move_to_new_basedir(base_dir);
        }
}
//---------------------------------------------
void        radarModule::append_init_script(octaveScript_ptr script)
{
    if (script==nullptr) return;
    script->move_to_new_basedir(get_full_path());
    _init_scripts.append(script);

}

//---------------------------------------------
void    radarModule::set_init_command(int command_order, int command_id)
{
    if ((command_order < 0)||(command_order>=_init_commands.count()))
        return;
    if ((command_id<0)||(command_id>=_params.count()))
        return;
}
//---------------------------------------------
void    radarModule::set_postacquisition_command(int command_order, int command_id)
{
    if ((command_order < 0)||(command_order>=_postacquisition_commands.count()))
        return;
    if ((command_id<0)||(command_id>=_params.count()))
        return;

}
//---------------------------------------------
int     radarModule::get_init_command(int command_order)
{
    if ((command_order < 0)||(command_order>=_init_commands.count()))
        return -1;
    return _init_commands[command_order];
}
//---------------------------------------------
int     radarModule::get_postacquisition_command(int command_order)
{
    if ((command_order < 0)||(command_order>=_postacquisition_commands.count()))
        return -1;

    return _postacquisition_commands[command_order];
}

//---------------------------------------------
void    radarModule::set_init_script(int script_order, octaveScript_ptr script)
{
    if (script==nullptr) return;
	if ((script_order<0)||(script_order>=_init_scripts.count()))
        return;
    script->move_to_new_basedir(get_full_path());
    _init_scripts[script_order] = script;

}
//---------------------------------------------
void    radarModule::set_postacquisition_script(int script_order, octaveScript_ptr script)
{
    if (script==nullptr) return;
	if ((script_order<0)||(script_order>=_post_acquisition_scripts.count()))
        return;
    script->move_to_new_basedir(get_full_path());
    _post_acquisition_scripts[script_order] = script;

}
//---------------------------------------------
octaveScript_ptr    radarModule::get_init_script(int script_order)
{
    if ((script_order<=0)||(script_order>=_init_scripts.count()))
        return nullptr;

    return (_init_scripts[script_order]);
}
//---------------------------------------------
octaveScript_ptr     radarModule::get_postacquisition_script(int script_order)
{
    if ((script_order<=0)||(script_order>=_post_acquisition_scripts.count()))
        return nullptr;

    return (_post_acquisition_scripts[script_order]);
}
//---------------------------------------------
bool    radarModule::copy_scripts_to_folder(QString base_dir, QString newfolder)
{
    const QVector<octaveScript_ptr>& init_scripts = _init_scripts;
    for (const auto& script: init_scripts)
        if (script!=nullptr)
        {        
            script->move_to_new_basedir(base_dir,newfolder,false,true);
        }
    const QVector<octaveScript_ptr>& postacq_scripts = _post_acquisition_scripts;
    for (const auto& script: postacq_scripts)
        if (script!=nullptr)
        {
            script->move_to_new_basedir(base_dir,newfolder,false,true);
        }

    return true;
}
//---------------------------------------------
bool    radarModule::copy_antenna_models_to_folder(QString base_dir, QString newfolder)
{
    const QVector<antenna_model_pointer>& _available_antennas = get_antenna_models();

    for (const auto& model: _available_antennas)
        if (model!=nullptr)
            {
                model->move_to_new_basedir(base_dir, newfolder, false, true);
            }

    const antenna_array& ants = _ant;
    for (const auto& ant:ants)
    {
        ant->move_to_new_basedir(base_dir, newfolder, false, true);
    }

    for (const auto& model: _available_antennas)
            if (model!=nullptr)
                model->save();
    return true;
}
//---------------------------------------------
bool    radarModule::copy_to_new_folder(QString dest_base_folder, QString module_folder, QString scripts_folder, QString antenna_model_folder)
{
    if (!copy_scripts_to_folder(dest_base_folder,scripts_folder)) return false;
    if (!copy_antenna_models_to_folder(dest_base_folder,antenna_model_folder)) return false;
    // To be checked
    move_to_new_basedir(dest_base_folder,module_folder,false,false);
    return save_file(get_full_filepath());
}
//---------------------------------------------
QVector<antenna_model_pointer>      radarModule::get_antenna_models()
{
    QVector<antenna_model_pointer> out;
    const antenna_array& ants = _ant;
    for (const auto& ant:ants)
    {
        if (!out.contains(ant->get_antenna_model()))
            out.append(ant->get_antenna_model());
    }
    return out;
}
//---------------------------------------------
void    radarModule::set_serial_port_configuration(
    qint32  baud_rate,
    QSerialPort::DataBits data_bits,
    QSerialPort::Parity   parity,
    QSerialPort::StopBits stop_bits,
    QSerialPort::FlowControl flow_control
    )
{
    _serial_port_info.baudRate = baud_rate;
    _serial_port_info.dataBits = data_bits;
    _serial_port_info.parity   = parity;
    _serial_port_info.stopBits = stop_bits;
    _serial_port_info.flowControl = flow_control;
}
//---------------------------------------------
SerialSettings  radarModule::get_serial_port_configuration()
{
    return _serial_port_info;
}
//---------------------------------------------
void    radarModule::set_expected_serial_port_data(QString description, QString manufacturer, QString serialnumber, QString vendorid, QString productid)
{
    _expected_serial_description = description;
    _expected_serial_manufacturer= QByteArray::fromHex(manufacturer.toLatin1());
    _expected_serial_number      = QByteArray::fromHex(serialnumber.toLatin1());
    //QByteArray vendor = QByteArray::fromHex(vendorid.toLatin1());
    //QByteArray product = QByteArray::fromHex(productid.toLatin1());

    _expected_vendorid           = QByteArray(vendorid.toLatin1()).toInt(nullptr,16);
    _expected_productid          = QByteArray(productid.toLatin1()).toInt(nullptr,16);
}
//---------------------------------------------
QString radarModule::get_serial_description()
{
    return _expected_serial_description;
}
//---------------------------------------------
QString radarModule::get_serial_manufacturer()
{
    return _expected_serial_manufacturer.toHex();
}
//---------------------------------------------
QString radarModule::get_serial_number()
{
    return _expected_serial_number.toHex();
}
//---------------------------------------------
QString  radarModule::get_serial_vendorid()
{
    return QByteArray::number(_expected_vendorid,16);
}
//---------------------------------------------
QString  radarModule::get_serial_productid()
{
    return QByteArray::number(_expected_productid,16);
}
//---------------------------------------------
QVector<radarParamPointer>      radarModule::get_param_group(radarParamPointer param)
{
    QVector<radarParamPointer> out;

    if (param==nullptr) return out;
    if (!param->is_command_group()) {out.push_back(param); return out;}

    return get_param_group(param->get_command_group());
}
//---------------------------------------------
QVector<radarParamPointer>      radarModule::get_param_group(const QByteArray& command_string)
{
    QVector<radarParamPointer> out;

    const QVector<radarParamPointer>& px = _params;

    int n_max_order = -1;

    for (const auto& param:px)
    {
        if (param->get_command_group() == command_string)
            if (param->get_command_order_group()>n_max_order)
                n_max_order = param->get_command_order_group();
    }

    out.resize(n_max_order+1);

    for (const auto& param:px)
    {
        if (param->get_command_group() == command_string)
        {
            int n=param->get_command_order_group();
            if ((n<0)||(n>n_max_order))
                continue;

            out[n] = param;
        }
    }

    return out;
}
//---------------------------------------------
QByteArray  radarModule::get_current_value_group(const QByteArray& command_string)
{
    QByteArray out;
    const auto& params = get_param_group(command_string);

    for (const auto& param: params)
        if (param!=nullptr)
            out.append(param->chain_values());

    return out;
}
//---------------------------------------------
QByteArray  radarModule::get_current_value_group(radarParamPointer command_param)
{
    QByteArray out;
    const auto& params = get_param_group(command_param);

    for (const auto& param: params)
        if (param!=nullptr)
            out.append(param->chain_values());

    return out;
}
//-----------------------------------------------------
// Set and retrieve the command for the inquiry of the module id

QByteArray         radarModule::get_inquiry_moduleid_command()
{
    return _inquiry_moduleid_command;
}
//---------------------------------------------
QString            radarModule::get_inquiry_moduleid_command_as_string()
{
    return _inquiry_moduleid_command.toHex(0);
}
//---------------------------------------------
void            radarModule::set_inquiry_moduleid_command_as_string(const QString& pname)
{
    QByteArray str_hex = QByteArray::fromHex(pname.toLatin1());
    _inquiry_moduleid_command = str_hex;
}
//---------------------------------------------
void    radarModule::set_inquiry_moduleid_command(const QByteArray& pname)
{
    _inquiry_moduleid_command = pname;
}
//-----------------------------------------------------
// Set and retrieve the command for the inquiry of the expected value of module id
QByteArray      radarModule::get_inquiry_moduleid_expectedvalue()
{
    return _inquiry_moduleid_expected_value;
}
//---------------------------------------------
void            radarModule::set_inquiry_moduleid_expectedvalue(const QByteArray& data)
{
    _inquiry_moduleid_expected_value = data;
}
//---------------------------------------------
void            radarModule::set_inquiry_moduleid_expectedvalue_as_string(const QString& data)
{
    _inquiry_moduleid_expected_value = QByteArray::fromHex(data.toLatin1());
}
//---------------------------------------------
QString         radarModule::get_inquiry_moduleid_expectedvalue_as_string()
{
    return QString(_inquiry_moduleid_expected_value.toHex(0));
}
//-----------------------------------------------------
// Set and retrieve the command for the inquiry of the instance id
QByteArray      radarModule::get_inquiry_instanceid_command()
{
    return _inquiry_instanceid_command;
}
//---------------------------------------------
void            radarModule::set_inquiry_instanceid_command(const QByteArray& data)
{
    _inquiry_instanceid_command = data;
}
//---------------------------------------------
void            radarModule::set_inquiry_instanceid_command_as_string(const QString& data)
{
    _inquiry_instanceid_command = QByteArray::fromHex(data.toLatin1());
}
//---------------------------------------------
QString         radarModule::get_inquiry_instanceid_command_as_string()
{
    return _inquiry_instanceid_command.toHex(0);
}

//---------------------------------------------
void    radarModule::set_comm_protocol( std::shared_ptr<interfaceProtocol>  protocol)
{
    _protocol = protocol;
}
//---------------------------------------------
std::shared_ptr<interfaceProtocol>  radarModule::get_comm_protocol()
{
    return _protocol;
}
//---------------------------------------------
QByteArray      radarModule::encode_param_for_transmission(const QVector<radarParamPointer>& params, bool inquiry)
{
    QByteArray out;
    if (params.count()==0) return out;
    if (_protocol==nullptr) return out;

    QByteArray command_string = params[0]->get_command_group();

    RADARPARAMIOTYPE rpt_io = params[0]->get_io_type();

    // 1. if parameter is a radar module Output (SDK Input only):
    // ---> send the command with NO payload. Check for return
    // 2. if the parameter is a radar module Input (SKD Output only)
    // ---> send the command with Paylod. Don't check for return payload
    // 3. if the parameter is I/O. Send a command encoding current_value, check for return
    // ---> if the inquiry flag is on, send inquiry-value

    if (rpt_io==RPT_IO_OUTPUT)
    {
        out.append(_protocol->get_start());
        out.append(_protocol->encode_tx(command_string));
        out.append(_protocol->get_stop());
        return out;
    }

    if (rpt_io==RPT_IO_INPUT)
    {
        out.append(_protocol->get_start());
        out.append(_protocol->encode_tx(command_string));
        // Append payload
        int exp_size = 0;
        QByteArray payload;
        for (auto param: params)
            if (param!=nullptr)
            {
                exp_size+=param->value_bytes_count();
                payload.append(param->chain_values());
            }
        if (exp_size!=payload.length()) return QByteArray();
        out.append(_protocol->encode_tx(payload));
        return out.append(_protocol->get_stop());
    }
    if (rpt_io==RPT_IO_IO)
    {
        out.append(_protocol->get_start());
        out.append(_protocol->encode_tx(command_string));
        QByteArray payload;
        if (inquiry==false)
        {
            // Append payload
            int exp_size = 0;
            for (auto param: params)
                if (param!=nullptr)
                {
                    exp_size+=param->value_bytes_count();
                    payload.append(param->chain_values());
                }
            if (exp_size!=payload.length()) return QByteArray();
            out.append(_protocol->encode_tx(payload));
            return out.append(_protocol->get_stop());
        }
        else
        {
            int exp_size = 0;
            QByteArray payload;
            payload.append(params[0]->get_inquiry_value());

            bool any_is_vector = false;
            for (auto param:params)
                if (param!=nullptr)
                {
                    if (!param->is_scalar()) any_is_vector = true;
                    exp_size+=param->value_bytes_count();
                }

            if ((!any_is_vector)&&(exp_size != payload.length()))
                return QByteArray();

            out.append(_protocol->encode_tx(payload));
            return out.append(_protocol->get_stop());
        }
    }

    return QByteArray();
}
//---------------------------------------------


bool  radarModule::decode_data_from_radar(const QByteArray& data)
{
    QByteArray out;
    if (data.length()==0) return false;
    if (_protocol==nullptr) return false;

    out = _protocol->decode_rx(data);

    if (out.isEmpty()) return false;

    // Remove command id
    int expected_command_length = _last_sent_command->get_command_group().length();
    QByteArray received_command = out.left(expected_command_length);
    if (received_command!=_last_sent_command->get_command_group())
        return false;

    QByteArray payload          = out.right(out.length()-expected_command_length);

    if (_last_sent_command->is_command_group())
    {

        QVector<radarParamPointer> params = get_param_group(_last_sent_command);
        const QVector<radarParamPointer>& px = params;

        for (const auto& param: px)
        {
            int expected_size = 0;
            if (param->is_scalar()) expected_size = param->get_expected_payload_size();

            if (expected_size==0) continue;
            if (expected_size==-1)
                // This is variable payload: it must be at the end of the packet.
                return param->split_values(payload);

            if (!param->split_values(payload.left(expected_size)))
                return false;

            payload = payload.right(payload.length()-expected_size);
        }
    }
    else
    {
        int expected_size = 0;
        if (_last_sent_command->is_scalar()) expected_size = _last_sent_command->get_expected_payload_size();

        if ((expected_size==-1)||(expected_size==payload.length()))
            // This is variable payload: it must be at the end of the packet.
            return _last_sent_command->split_values(payload);

        return false;
    }
    return true;
}



//-----------------------------------------------------------------------------------------
bool radarModule::contain_script(octaveScript* script)
{
    bool bfound = false;
    if (!bfound)
    {
        for (auto& mod_script : _init_scripts)
        {
            if (mod_script!=nullptr)
                if (mod_script->get_name()==script->get_name())
                {
                    bfound = true; break;
                }
        }
    }

    if (!bfound)
    {
        for (auto& mod_script : _post_acquisition_scripts)
        {
            if (mod_script!=nullptr)
                if (mod_script->get_name()==script->get_name())
                {
                    bfound = true; break;
                }
        }
    }

    return bfound;
}

//-----------------------------------------------------------------------------------------
void radarModule::remove_script(octaveScript* script)
{
    if (script==nullptr) return;
    auto iter = _init_scripts.begin();
    while (iter!=_init_scripts.end())
    {
        if ((*iter)->get_name()==script->get_name())
            _init_scripts.erase(iter);
        else
            iter++;
    }

    iter = _post_acquisition_scripts.begin();
    while (iter!=_post_acquisition_scripts.end())
    {
        if ((*iter)->get_name()==script->get_name())
            _post_acquisition_scripts.erase(iter);
        else
            iter++;
    }


}
