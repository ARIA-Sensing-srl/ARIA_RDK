/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef OCTAVEWS_H
#define OCTAVEWS_H

#include "ov.h"
#include "ovl.h"
#include "ov-struct.h"
#include <map>
#include <set>
#include <QObject>


bool ov_equal(const octave_value& a,const octave_value &b);

typedef std::map<std::string,octave_value>   oct_dataset;
typedef std::pair<std::string, octave_value> variable;
class octavews : public QObject
{
private:
    oct_dataset             _octave_variables;
    oct_dataset             _internal_variables;
    octave::interpreter*    _oct_int;
    octave_value            dumb;
    std::set<class wndPlot2d*>   _graphs2d;
    std::set<std::string>        _updated_vars;
    bool                         _b_deleting; // Used to avoid circular deleting
    class octaveInterface*       _data_interface;
public:
    octavews(octave::interpreter* interpreter = nullptr);
    ~octavews();

    class octaveInterface*      data_interface() {return _data_interface;}
    void                        data_interface(class octaveInterface* data_interface) {_data_interface = data_interface;}

    void            merge_value_list(const string_vector& vars, bool octave_generated = true, const octave_value_list& values = octave_value_list());
    void            remove_value_list(const string_vector& vars);
    void            add_variable(const std::string& varname, bool octave_generated = true, const octave_value& value = octave_value());
    void            clear();
    void            clear_internal();
    void            clear_octave();
    octave_value&   var_value(const std::string& varname);
    bool            is_internal(const std::string& varname);
    bool            is_octave(const std::string& varname);
    void            workspace_to_interpreter_noautolist();
    void            workspace_to_interpreter();
    void            interpreter_to_workspace();
    void              set_variable(const std::string& varname, const octave_value& value);
    void              set_variable_no_immediate_update(const std::string& varname, const octave_value& value);
    void              update_after_set_variables();
    octave_value_list get_var_values(string_vector names);
    void              set_var_values(string_vector names, octave_value_list values);
    string_vector     get_var_names(bool internal);
    int               get_variable_count(bool internal);
    variable          get_variable(int id, bool binternal);

// Graph update methods. We maintain a list of graphs depending on this
// workspace. We also build a list of updated variables
    void              add_graph(class wndPlot2d* graph);
    void              remove_graph(class wndPlot2d* graph);
    void              update_graphs();
    void              add_variable_to_updatelist(std::string variable);
    std::set<std::string>& get_updated_variables();

    void              save_to_file(std::string filename);



};

#endif // OCTAVEWS_H
