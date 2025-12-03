/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef RADARINSTANCE_H
#define RADARINSTANCE_H
#include "octavews.h"
#include "radarmodule.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <serialport_utils.h>
#include <QtXml>
//class radarModule;

/*
 * This class contains the definition of a radar module instance, referring
 * to a pre-defined radar module. (e.g. LT103, LT102, Hydrogen-based...)
 */
enum OPERATION {RADAROP_NONE, RADAROP_INIT_PARAMS, RADAROP_INIT_SCRIPTS, RADAROP_POSTACQ_PARAMS, RADAROP_POSTACQ_SCRIPTS};

class radarInstance : public QObject, public radarModule
{
    Q_OBJECT
private:
    std::shared_ptr<interfaceProtocol>      _protocol;
    radarModule*                 _ref_module;
    QMap<QString,radarParamPointer>         _map_octws_param_names; // each pair is "workspace name" (e.g. LT102_ABCDEF01_parameter) to parameter
    octavews*                    _workspace;
    QByteArray                   _radar_uid;                    // This is the expected _radar_uid
    QByteArray                   _actual_radar_uid;             // This is the actual _radar_uid
    bool                         _fixed_serial_port;            // If true connect only to devices of given serial port (basically: PC do not change)
    bool                         _fixed_id;                     // If true connect only to devices of a given UID (basically, radar module do not change)
    QString                      _device_name;
    QString                      _expected_portname;            // Expected portname (used when _fixed_serial_port = true)

    QString                      build_param_name(bool complete_name);
    bool                         transmit_param(radarParamPointer param);
#ifdef INTERFACE_STRUCT
    octave_map                   _radar_cell;
#endif
/*
 * Serial Port Stuff
 * */
    QByteArray                  _last_serial_tx;
    QByteArray                  _last_serial_rx;
    QString                     _last_error_serial;
    QByteArray                  _left_rx;
    int                         kWriteTimeoutTx = 150;
    int                         kWriteTimeoutRx = 800;

    qint64                      _bytesToWrite;
    volatile SerialStatus       _serial_status;
    QSerialPort*                _serialport;                    // This is the actual SerialPort object used for data com with physical device
    QTimer*                     _serial_timer;
    QVector<radarParamPointer>  _params_to_modify;              // List of params that requires link to octave during init/preacq/postacq and update
    QVector<radarParamPointer>  _params_updated;                // List of params currently updated
    QVector<radarParamPointer>  _params_to_inquiry;             // List of params that requires link to octave during init/preacq/postacq and inquiry
    int                         _n_params_to_receive;
    void                        clear_params_update_lists();
    OPERATION                   _radar_operation;

    void                        update_param_internal_copy(radarParamPointer param, octave_value val);
public:
    radarInstance(radarModule* module=nullptr);
    radarInstance(radarInstance& radar);
    ~radarInstance();

    OPERATION       get_current_operation() {return _radar_operation;}

    QString get_item_descriptor();

    radarInstance& operator = (radarInstance& radar);

    void            attach_to_workspace(octavews *dws);
    octavews*       get_workspace(void);
    QString         get_mapped_name(radarParamPointer param);
    QVector<radarParamPointer> get_cmdgrp_varlist(radarParamPointer param);

    void            create_variable(QString parameterName);
    void            create_all_variables();
    void            remove_variable(QString parameterName);
    void            remove_variable(radarParamPointer param);
//---------------------------------------------------------------
    void            immediate_set_param_value(radarParamPointer param, const octave_value& val, bool updateLocalOnly = false);
    void            immediate_inquiry_value(radarParamPointer param);
    void            immediate_set_command(radarParamPointer param);
//---------------------------------------------------------------
    void            set_param_value(radarParamPointer param, octave_value val, bool transmit = false, bool update_workspace = false);
    void            set_param_value(radarParamPointer param, const QVector<QVariant>& val, bool transmit = false, bool update_workspace = false);
    void            set_param_value(QString name, octave_value val, bool transmit = false, bool update_workspace = false);
    void            set_param_value_default(int param_index, bool transmit= false);
    void            set_param_value(int param_index, octave_value val, bool transmit= false, bool update_workspace = false);
    bool            read_param_value(QString name, bool use_alias = false);
    bool            read_param_value(int param_index);

    void            workspace_to_params();
    void            params_to_workspace();

    QString         get_uid_string();
    QByteArray      get_uid();
    void            set_uid_string(const QString& uid);
    void            set_uid(const QByteArray& uid);
    radarModule*    get_module();
    const QByteArray& get_actual_uid() {return _actual_radar_uid;}
    QString           get_actual_uid_string() {return _actual_radar_uid.toHex(0);}

    void            set_fixed_id(bool fixed_id);
    void            set_fixed_port(bool fixed_port);
    bool            fixed_id();
    bool            fixed_port();
    bool            scan();
    QByteArray      query_serial_id(bool& bok);
    bool            query_module_id();
    void            set_ports(const QList<QSerialPortInfo>& available_ports);
    bool            set_port(const QSerialPortInfo& port, bool force_connection = false);
    bool            set_port(const QSerialPortInfo& port, const SerialSettings& settings, bool force_connection = false);
    void            startup();
    void            acquisition();
    void            postprocess();

    void            set_device_name(const QString& device_name);
    QString         get_device_name();
    void            save_extra_project_item_data(QDomDocument& doc,QDomElement& node);
    void            load_extra_project_item_data(QDomElement& node);

    void            set_expected_portname(const QString& portname);
    QString         get_expected_portname();

    bool            can_differentiate();

    bool            save_xml(QDomDocument& document);
    bool            save_xml();
    bool            load_xml();

    void            init();
    void            run();
    // Serial port stuff
    //void            set_serial_port_settings(SerialSettings settings);
    //void            open_serial_port();

    bool            transmit_custom(const QByteArray& tx);
    void            set_module(radarModule *ref);
    void            update_module();
    void            export_to_octave();
    void            import_from_octave();
//---------------------------------------------------------------
    void            immediate_update_variable(const std::string& varname);
    void            immediate_inquiry_variable(const std::string& varname);
    void            immediate_command(const std::string& varname);
//---------------------------------------------------------------
    void            update_variable(const std::string& varname);
    void            update_variables(const std::set<std::string>& varlist);
    bool            transmit_command(radarParamPointer param);
    bool            inquiry_parameter(radarParamPointer param);

    const QByteArray& last_serial_tx() {return _last_serial_tx;}
    const QByteArray& last_serial_rx()  {return _last_serial_rx;}
    void              clear_serial_buffer() {_last_error_serial.clear(); _last_serial_rx.clear(); _last_serial_tx.clear();}
    SerialStatus      get_serial_status() {return _serial_status;}
    void              initSerialPortTimer();
    void              clearSerialPortTimer();

    bool              is_connected();
    void              disconnect();
    void              port_connect();
    void              auto_connect();
    bool              transmit_and_wait_answer(QByteArray& datatx);
    void              transmit_data     (const QByteArray& datatx);
    void              wait_for_answer   ();
    bool              transmit_param_blocking(const QVector<radarParamPointer>& params, bool inquiry=false);
    bool              transmit_command_blocking(radarParamPointer param);
    bool              transmit_param_non_blocking(const QVector<radarParamPointer>& params, bool inquiry=false);
    void              serial_chain_data(const QByteArray& data);
    bool              update_command_from_last_rx();
    bool              update_param_from_last_rx();
    void              update_param_workspace_gui(radarParamPointer param/*, bool update_workspace*/);
    void              param_is_updated(radarParamPointer param);

    void              emit_operation_done();

    bool            init_pre();
    bool            init_scripts();
    bool            postacquisition_pre();
    bool            postacquisition_scripts();

    void            set_serial_timeout(int timeout) {if (timeout>0) kWriteTimeoutRx=timeout;}
    int             get_serial_timeout()            {return kWriteTimeoutRx;}

    QByteArray      set_port_no_module_check(char module_id_inquiry_command, const QSerialPortInfo& port, const SerialSettings& settings);

    bool            contain_script(octaveScript* script);
    void            remove_script(octaveScript* script);

public slots:
    //void            data_received();
    void            serial_error(QSerialPort::SerialPortError error);
    void            serial_timeout();
    void            bytes_written(qint64 bytes);
    void            read_data();

signals:
    void            new_read_completed(const QByteArray& rx);
    void            new_write_completed(const QByteArray& tx);
    void            tx_timeout(const QString& strerror);
    void            rx_timeout(const QString& strerror);
    void            serial_error_out(const QString& strerror);
    void            param_updated(radarParamPointer& param);
    void            connection_done();
    void            disconnection();
    void            all_params_updated();
    void            init_params_done();
    void            init_scripts_done();
    void            preacq_params_done();
    void            preacq_scripts_done();
    void            postacq_params_done();
    void            postacq_scripts_done();
};

#endif // RADARINSTANCE_H
