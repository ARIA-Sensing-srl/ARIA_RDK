/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef WNDRADARMODULEEDITOR_H
#define WNDRADARMODULEEDITOR_H

#include <QDialog>
#include "octavescript.h"
#include "radarparameter.h"
#include <QVariant>
#include <QTableWidgetItem>
#include <QComboBox>
#include <antenna.h>
#include <interfacecommand.h>

namespace Ui {
class wndRadarModuleEditor;
}
class radarModule;

class wndRadarModuleEditor : public QDialog
{
    Q_OBJECT

public:
    explicit wndRadarModuleEditor(radarModule* module=nullptr, radarProject* project = nullptr, QWidget *parent = nullptr);
    ~wndRadarModuleEditor();

    int _prevLine = -1;
    radarModule*            get_radar_module() {return _radar_module;}

private:
    radarProject*           _project;
    Ui::wndRadarModuleEditor *ui;
    bool                            _bAntennaNeedValid;
    bool                            _bNewModule;
    radarModule               *_radar_module;
    radarModule               *_source;
//------------------------------
// Temp data
    QVector<radarParamPointer>      _ptable;
    antenna_array                   _antenna_table;
    QVector<antenna*>            _available_antennas;
    bool                            _uploading_antenna_cb;
//------------------------------
    RADARPARAMSIZE                  get_size(int row);
    RADARPARAMTYPE                  get_rpt(int row);
    RADARPARAMIOTYPE                get_rpt_io(int row);
    QString                         get_name(int row);
    void                            init_param_table();
    void                            init_antenna_table();
    void                            clear_ptable();
    void                            copy_radar_params_to_temp();
    void                            copy_temp_params_to_radar();
    void                            copy_radar_array_to_temp();
    void                            copy_temp_array_to_module();
    void                            set_param_row(radarParamPointer param, int rownumber);
    int                             validate_param(radarParamPointer &dest, int row); // Return -1 on success, the ID of the column showing an error
    radarParamPointer               recreate_param(radarParamPointer pbase, QString name, RADARPARAMTYPE rt);
    void                            update_row_param(int row);
    void                            fill_cb_with_enums(QComboBox* cb,radarParamPointer rp);
    void                            fill_cb_with_available(QComboBox* cb,radarParamPointer rp);
    void                            update_default_widget(radarParamPointer param, int row, bool scalar = true, bool force_scalar = false);

    bool                            antenna_table_row_to_antenna(antenna_pointer ant, int row);
    bool                            validate_antenna_row(int row);
    void                            init_txrx_pairs_table();
    void                            init_txrx_antenna_combobox(QComboBox* cb);
    void                            remove_wrong_txrx_pairs();
    int                             validate_antennas();

    void                            set_param_row_void(radarParamPointer param,int row);
    bool                            validate_param_groups();

    void fill_antenna_combobox(QComboBox* cb);
    void set_antennacombobox_index(QComboBox* cb, antenna_model_pointer ant);

    void set_table_antennarow(antenna_pointer ant, int row);

    void init_tables();
    void fill_param_combobox(QComboBox* cb, int current = -1);
    void update_commands(QString old_param_name, QString new_param_name);
    void update_commands(QVector<radarParamPointer> previous_table, QVector<radarParamPointer> new_table);
    QVector<int> update_commands(QVector<int> paramlist, QVector<radarParamPointer> previous_table, QVector<radarParamPointer> new_table);

    void    command_tables_to_radar();
    void    script_tables_to_radar();

    octaveScript*        add_new_script();
    void    renumber_table(QTableWidget* table);

    QVector<octaveScript_ptr>   _init_scripts;
    QVector<octaveScript_ptr>   _acq_scripts;
    QVector<octaveScript_ptr>   _postacq_scripts;

    void        update_all_txrx_combos(QString oldname, QString newname);

    void        init_serial_port_combos();
    void        update_cb_inquiry();

public slots:

    void cbType_index_changed(int index=0);
    void cbSize_index_changed(int index=0);
    void cbIO_index_changed(int index=0);

    void btn_edit_limits_clicked();
    void btn_edit_default_clicked();
    void current_cell_change(int row=0, int col=0, int oldrow = 0, int oldcol =0 );
    void add_param_clicked();
    void param_selection_changed();
    void edit_parameter_default(radarParamPointer& param);
    bool edit_parameter_limits(radarParamPointer& param);

    void add_parameter();
    void remove_parameter();
    void save_parameters();
    void undo_parameters();
    void copy_parameter();

    void add_antenna();
    void remove_antenna();
    void clear_antenna_table();
    void save_antennas();
    void revert_antennas();

    void cb_antenna_model_changed(int index = 0);

    void save_radar_module();
    void save_radar_module_as();
    void load_radar_module();

    void add_txrx_pair();
    void add_txrx_pair(bool create_new);
    void remove_txrx_pair();

    void current_txrx_pair_changed(int current =0);

    void start_direction_focusing();
    void start_point_focusing();
/*
    void add_command();
    void add_interface_command_row(int nrow);
    void remove_interface_command();
    void confirm_commands_table();
*/

    void initParams_RightClick(QPoint pos);
    void acqParams_RightClick(QPoint pos);
    void postacqParams_RightClick(QPoint pos);
    void initScripts_RightClick(QPoint pos);
    void acqScripts_RightClick(QPoint pos);
    void postacqScripts_RightClick(QPoint pos);

    void add_initParam( );
    void remove_initParam( );
    void add_acquisitionParam( );
    void remove_acquisitionParam( );
    void add_postAcquisitionParam( );
    void remove_postAcquisitionParam( );


    void add_initScript( );
    void remove_initScript( );
    void add_acquisitionScript( );
    void remove_acquisitionScript( );
    void add_postAcquisitionScript( );
    void remove_postAcquisitionScript( );

    void update_antenna_data(int row, int column);
    void antenna_name_changed(QWidget *item);

    void scanRadarDevices();
    void createDevice();
signals:
    void radar_saved(radarModule* );


};
#endif // WNDRADARMODULEEDITOR_H
