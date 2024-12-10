/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef WNDRADARINSTANCEEDITOR_H
#define WNDRADARINSTANCEEDITOR_H

#include "qcombobox.h"
#include "qtablewidget.h"
#include <QDialog>
#include <radarinstance.h>
#include "../scheduler/opscheduler.h"

namespace Ui {
class wndRadarInstanceEditor;
}

class wndRadarInstanceEditor : public QDialog
{
    Q_OBJECT

public:
    explicit wndRadarInstanceEditor(radarInstance* radar,
                                    QVector<radarModule*> available_models = QVector<radarModule*>(),                                    
                                    QWidget *parent = nullptr);
    ~wndRadarInstanceEditor();

    radarInstance*  getRadarInstance() {return _radar_instance;}
protected:
    void closeEvent(QCloseEvent *event);
private:
    bool        _b_handling_script;
    Ui::wndRadarInstanceEditor *ui;

    QString                     _last_operation_result;
    radarProject*               _project;

    radarInstance*              _radar_instance;
    radarInstance*              _backup;
    QVector<radarModule*>       _available_models;
    QVector<octaveScript*>      _available_scripts;
    void                        init_script_tables();
    void                        init_script_table(QTableWidget* table, QVector<octaveScript_ptr> list);
    void                        fill_script_table_widget(QComboBox* cb, octaveScript_ptr script = nullptr);
    void                        add_script_table_empty_row(QTableWidget* table);
    void                        init_parameter_table();
    void                        param_to_table_row(int row, radarParamPointer current_param, radarParamPointer default_value);
    void                        current_param_to_table(int row, radarParamPointer current_value);
    //void                        update_serial_output();
    void                        update_read_value(int row);
    bool                        _b_transmitting;
    void                        connect_serial_updates();
    octaveScript*               load_new_script();

    opScheduler*                _scheduler;
public slots:
    void                        confirm();
    void                        cancel();
    void                        connect_radar();
    void                        update_connect_btn();
    void                        scan();
    void                        send_custom_string();
    void                        paramSliderReleased();
    void                        cbEnumIndexChange(int);
    void                        linkToOctaveChanged(Qt::CheckState);
    void                        compoundNameChanged(Qt::CheckState);
//    void                        currentPlotChanged(int);
    void                        itemChanged(QTableWidgetItem* item);
    void                        cbInitScriptChanged(int index);
    void                        cbPreacqScriptChanged(int index);
    void                        cbPostacqScriptChanged(int index);
    void                        clearSerialOutput();
    void                        init();
    void                        run();
    void                        exportToOctave();
    void                        importFromOctave();
    void                        variable_updated(const std::string& varname);
    void                        variables_updated(const std::set<std::string>& varlist);
    void                        send_command();
    void                        inquiry_parameter();
    void                        tx_updated(const QByteArray& tx);
    void                        rx_updated(const QByteArray& rx);
    void                        tx_timeout(const QString& strerror);
    void                        rx_timeout(const QString& strerror);
    void                        serial_error(const QString& strerror);
    void                        radar_param_update_byserial(radarParamPointer ptr);
    void                        scheduler_running();
    void                        scheduler_halted();
    void                        connection_done(radarInstance* device);
    void                        connection_done_all();
    void                        connection_error(radarInstance* device);
    void                        init_done(radarInstance* device);
    void                        init_done_all();
    void                        init_error(radarInstance* device);
    void                        postacquisition_error(radarInstance* device);
    void                        postacquisition_done_all();
    void                        preacquisition_done_all();
    void                        preacquisition_error(radarInstance* device);
    void                        scheduler_timing_error();

};

#endif // WNDRADARINSTANCEEDITOR_H
