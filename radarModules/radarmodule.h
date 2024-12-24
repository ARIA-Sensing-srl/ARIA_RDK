/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef RADARMODULE_H
#define RADARMODULE_H

#include <octave.h>
#include <ovl.h>
#include <ov.h>

#include <QVector>
#include "antennainstance.h"

#include "radarproject.h"
#include "radarparameter.h"

#include <QVariant>
#include <QFile>
#include <QtXml>

#include <memory>

#include "../serialcomm/interfaceprotocol.h"
#include "octavescript.h"
#include "QSerialPort"
#include "interfaceprotocol.h"

#define  XML_RADARFILE_MAJOR  0
#define  XML_RADARFILE_MINOR  1
#define  XML_RADARFILE_SUBVER 0

enum SerialStatus{SS_IDLE, SS_TRANSMITTING, SS_RECEIVING, SS_RECEIVEDONE, SS_ERROR, SS_TIMEOUT};

struct SerialSettings {
    qint32 baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity parity;
    QSerialPort::StopBits stopBits;
    QSerialPort::FlowControl flowControl;
    bool operator == (SerialSettings serial)
    {
        return (dataBits==serial.dataBits)&&
               (parity==serial.parity)&&
               (stopBits==serial.stopBits)&&
               (flowControl==serial.flowControl);
    }
};

class radarModule : public projectItem
{

protected:
    QString                     _module_name;
    antenna_array               _ant;
    QByteArray                  _inquiry_moduleid_command;
    QByteArray                  _inquiry_moduleid_expected_value;
    QByteArray                  _inquiry_instanceid_command;


    QVector<radarParamPointer>   _params;
    QFile                       _rfile;
    QVector<QPair<int,int>>    _txrx_scheme;

    QVector<double>             _phases;

    NDArray                    _freqs;
    NDArray                    _azimuth;
    NDArray                    _zenith;
    antennaInstance            _equivalent_array_antenna;
    int                        _freq_of_interest;

    std::shared_ptr<interfaceProtocol>          _protocol;

    QVector<int>                _init_commands;
    QVector<int>                _postacquisition_commands;

    bool                        save_xml();
    bool                        load_xml();
    bool                        save_xml(QDomDocument& document);

    QVector<octaveScript_ptr>       _init_scripts;
    QVector<octaveScript_ptr>       _post_acquisition_scripts;

    void                        copy_scripts(QVector<octaveScript_ptr>& dest, const QVector<octaveScript_ptr>& source);


protected:
    radarModule(QString name, dataType type);
    SerialSettings             _serial_port_info;
    QString                    _expected_serial_description;
    QByteArray                 _expected_serial_manufacturer;
    QByteArray                 _expected_serial_number;
    qint32                     _expected_vendorid;
    qint32                     _expected_productid;
    radarParamPointer           _last_sent_command;


public:
    radarModule(QString filename="", projectItem *parent = nullptr);
    radarModule(radarModule& module);
    ~radarModule();

    bool           load_file(QString filename="");
    bool           save_file(QString filename="");
    bool           require_normalization();
    radarModule&   operator = (radarModule& m2);
    void                        set_filename(QString filename);
    void                        set_name(QString name);
    int                         has_param(QString parname);
    int                         get_param_count();
    radarParamPointer           get_param(QString name);
    radarParamPointer           get_param(int id);
    void                        set_param(int id, radarParamPointer& param);
    void                        set_param_copy(int id, radarParamPointer& param);
    void                        set_clear_and_copy(int id, radarParamPointer& param);
    void                        append_param(radarParamPointer par);
    void                        remove_param(int id);
    void                        remove_param(const QString& param_name);
    QVector<radarParamPointer>  get_param_table() {return _params;}
    void                        set_param_table(QVector<radarParamPointer> ptable) {_params = ptable;}
    void                        clear_param_table();
    void                        clear_antenna_table();
    void                        append_copy_of_param(radarParamPointer& param);
    antennaInstance             calculate_focusing_direction(double target_theta, double target_phi, QVector<QPair<int,int>> txrx);
    void                        equalize_antenna_supports();
    antenna_array               get_antenna_array();
    int                         get_antenna_count();
    antenna_pointer             get_antenna(int antennaid);
    void                        set_antenna_model(int antennaid, antenna_model_pointer model);

    QString                     absolute_path(QString basepath);

    void                        append_antenna(antenna_model_pointer model, QString name, double pos_x=0, double pos_y=0, double pos_z=0, double rot_theta=0, double rot_phi=0, double loss = 0, double delay=0);
    void                        append_antenna(antenna_pointer antenna);


    void                        set_txrx_pairs_count(int n);
    void                        set_txrx_pair(int npair, int ant1, int ant2);
    void                        append_txrx_pair(int ant1, int ant2);
    void                        clear_txrx_pairs();
    QPair<int,int>              get_txrx_pair(int npair);
    int                         get_txrx_pairs_count();
    QVector<QPair<int,int>>     get_txrx_pairs();
    void                        set_txrx_pairs(QVector<QPair<int,int>> txrx);
    void                        remove_txrx_pair(int pair);

    bool                        set_support(NDArray freqs, NDArray azimuths, NDArray zeniths);

    NDArray                     get_freq()      {return _freqs;}
    NDArray                     get_azimuth()   {return _azimuth;}
    NDArray                     get_zenith()    {return _zenith;}

    void                        calculate_direction_focusing(double a_ref, double z_ref, int nf, bool compensate_delay=false);
    void                        calc_gain_equivalent();
    NDArray                     gain_equivalent();
    NDArray                     freqs_equivalent()          {return _freqs;}
    NDArray                     azimuth_equivalent()        {return _azimuth;}
    NDArray                     zenith_equivalent()         {return _zenith;}
    void                        set_freq_of_interest(int nf=-1);
    int                         freq_of_interest()              {return _freq_of_interest;}

    void                        calculate_antenna_planar_waves();


    QVector<int>                get_init_commands() {return _init_commands;}
    QVector<int>                get_postacquisition_commands() {return _postacquisition_commands;}
    void                        append_init_command(int param)              {if ((param>=0)&&(param<=_params.count())) _init_commands.append(param);}
    void                        append_postacquisition_command(int param)   {if ((param>=0)&&(param<=_params.count())) _postacquisition_commands.append(param);}

    void                        set_init_commands(QVector<int> commands) {_init_commands = commands;}
    void                        set_postacquisition_commands(QVector<int> commands) {_postacquisition_commands = commands;}
    void                        remove_init_command(int param)              {if ((param>=0)&&(param<=_init_commands.count())) _init_commands.remove(param);}
    void                        remove_postacquisition_command(int param)   {if ((param>=0)&&(param<=_postacquisition_commands.count())) _postacquisition_commands.remove(param);}

    void                        set_init_command(int command_order, int command_id);
    void                        set_postacquisition_command(int command_order, int command_id);

    int                         get_init_command(int command_order);
    int                         get_postacquisition_command(int command_order);

    QVector<octaveScript_ptr>       get_init_scripts()                          {return _init_scripts;}
    QVector<octaveScript_ptr>       get_postacquisition_scripts()               {return _post_acquisition_scripts;}

    void                        set_init_scripts(QVector<octaveScript_ptr> scripts) ;
    void                        set_postacquisition_scripts(QVector<octaveScript_ptr> scripts);

    void                        append_init_script(octaveScript_ptr script);
    void                        append_postacquisition_script(octaveScript_ptr script);

    void                        remove_init_script(int nscript)                     {if ((nscript>=0)&&(nscript<=_init_scripts.count()))        _init_scripts.remove(nscript);}
    void                        remove_postacquisition_script(int nscript)          {if ((nscript>=0)&&(nscript<=_post_acquisition_scripts.count())) _post_acquisition_scripts.remove(nscript);}

    void                        set_init_script(int script_order, octaveScript_ptr script);
    void                        set_postacquisition_script(int script_order, octaveScript_ptr script);

    octaveScript_ptr         get_init_script(int script_order);
    octaveScript_ptr         get_postacquisition_script(int script_order);

    void                        clear_commands_tables();
    void                        clear_scripts_tables();

    bool                     copy_antenna_models_to_folder(QString base_dir, QString newfolder);
    bool                     copy_scripts_to_folder(QString base_dir, QString newfolder);
    bool                     copy_to_new_folder(QString dest_base_folder, QString module_folder, QString scripts_folder, QString antenna_model_folder);

    QVector<antenna_model_pointer>      get_antenna_models();

    void                    set_serial_port_configuration(
        qint32  baud_rate,
        QSerialPort::DataBits data_bits,
        QSerialPort::Parity   parity,
        QSerialPort::StopBits stop_bits,
        QSerialPort::FlowControl flow_control
        );


    SerialSettings  get_serial_port_configuration();

    void    set_expected_serial_port_data(QString description, QString manufacturer, QString serialnumber, QString vendorid, QString productid);
    QString get_serial_description();
    QString get_serial_manufacturer();
    QString get_serial_number();
    QString  get_serial_vendorid();
    QString  get_serial_productid();
    QVector<radarParamPointer>      get_param_group(radarParamPointer param);
    QVector<radarParamPointer>      get_param_group(const QByteArray& command_string);
    QByteArray                      get_current_value_group(const QByteArray& command_string);
    QByteArray                      get_current_value_group(radarParamPointer command_param);

    QByteArray      get_inquiry_moduleid_command();
    QString         get_inquiry_moduleid_command_as_string();
    void            set_inquiry_moduleid_command(const QByteArray& pname);
    void            set_inquiry_moduleid_command_as_string(const QString& pname);

    QByteArray      get_inquiry_moduleid_expectedvalue();
    void            set_inquiry_moduleid_expectedvalue(const QByteArray& data);
    void            set_inquiry_moduleid_expectedvalue_as_string(const QString& data);
    QString         get_inquiry_moduleid_expectedvalue_as_string();

    QByteArray      get_inquiry_instanceid_command();
    void            set_inquiry_instanceid_command(const QByteArray& data);
    void            set_inquiry_instanceid_command_as_string(const QString& data);
    QString         get_inquiry_instanceid_command_as_string();

    void            set_comm_protocol( std::shared_ptr<interfaceProtocol>  protocol);
    std::shared_ptr<interfaceProtocol>  get_comm_protocol();


    QByteArray      encode_param_for_transmission(radarParamPointer param);
    QByteArray      encode_param_for_transmission(const QVector<radarParamPointer>& params, bool inquiry=false);
    bool            decode_data_from_radar(const QByteArray& data);
    bool            operator == (const radarModule& module2);
};





#endif // RADARMODULE_H
