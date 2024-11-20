/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "octavews.h"
#include "interpreter.h"
#include "../uiElements/wndplot2d.h"
#include "octaveinterface.h"
using namespace std;

bool ov_equal(const octave_value& a,const octave_value &b)
{
    if (a.isstruct())
    {
        if (b.isstruct())
        {
            octave_map am = a.map_value();
            octave_map bm = b.map_value();


            if (am.nfields()!=bm.nfields()) return false;

            for (auto& afield: am)
            {
                if (!bm.isfield(afield.first)) return false;
                octave_value aval = afield.second;
                octave_value bval = bm.getfield(afield.first).elem(0);
                if (aval.isempty()!=bval.isempty()) return false;
                if (!bval.is_equal(aval)) return false;
            }
            return true;
        }
        else
            return false;
    }
    return a.is_equal(b);
}

octavews::octavews(octave::interpreter* interpreter) : _octave_variables(), _internal_variables(), _oct_int(interpreter), _data_interface(nullptr)
{
    _b_deleting = false;
}

octavews::~octavews()
{
    _b_deleting = true;
    for (const auto& graph:_graphs2d)
    {
        if (graph!=nullptr)
            graph->remove_workspace();
    }
    _b_deleting = false;
    clear();
}


void octavews::merge_value_list(const string_vector& vars, bool octave_generated, const octave_value_list& values)
{
    if (octave_generated)
    {
        if ((_oct_int==nullptr)&&(values.length()!=vars.numel()))
            return;

        for (int s = 0; s < vars.numel(); s++)
        {
            string vname = vars(s);
            oct_dataset::iterator vfound = _internal_variables.find(vname);
            if (vfound != _internal_variables.end())
                _internal_variables.erase(vfound);
            // when octave generated, ignore the values (unless _oct_int==null)
            if (_oct_int==nullptr)
                _octave_variables[vname] = values(s);
            else
                _octave_variables[vname] = _oct_int->varval(vname);

        }
        return;
    }

    if (!octave_generated)
    {
        if (values.length()!=vars.numel())
            return;

        for (int s = 0; s < vars.numel(); s++)
        {
            string vname = vars(s);
            oct_dataset::iterator vfound = _octave_variables.find(vname);
            if (vfound != _octave_variables.end())
                _octave_variables.erase(vfound);
            // internal values requires a set of values
            _internal_variables[vname] = values(s);

        }
        return;
    }
}
void octavews::remove_value_list(const string_vector& vars)
{
    for (int n=0; n < vars.numel(); n++)
    {
        string vname = vars(n);
        oct_dataset::iterator vfound = _internal_variables.find(vname);
        if (vfound != _internal_variables.end())
            _internal_variables.erase(vfound);

        vfound = _octave_variables.find(vname);
        if (vfound != _octave_variables.end())
            _octave_variables.erase(vfound);
    }
}

void octavews::add_variable(const string& varname, bool octave_generated, const octave_value& value)
{

    oct_dataset::iterator vfound;
    if (octave_generated)
    {
        vfound = _internal_variables.find(varname);
        if (vfound != _internal_variables.end())
            _internal_variables.erase(vfound);
    }

    if (!octave_generated)
    {
        vfound = _octave_variables.find(varname);
        if (vfound != _octave_variables.end())
            _octave_variables.erase(vfound);
    }

    if (octave_generated)
        _octave_variables[varname] = _oct_int == nullptr ? value : _oct_int->varval(varname);
    else
        _internal_variables[varname] = value;
}

void octavews::clear()
{
    clear_internal();
    clear_octave();
}

void octavews::clear_internal()
{
    _internal_variables.clear();
}


void    octavews::clear_octave()
{
    _octave_variables.clear();
}


octave_value&   octavews::var_value(const std::string& varname)
{
    oct_dataset::iterator vfound = _internal_variables.find(varname);
    if (vfound != _internal_variables.end())
        return vfound->second;

    vfound = _octave_variables.find(varname);
    if (vfound != _octave_variables.end())
        return vfound->second;

    return dumb;
}

bool            octavews::is_internal(const std::string& varname)
{
    oct_dataset::iterator vfound = _internal_variables.find(varname);
    if (vfound != _internal_variables.end())
        return true;
    return false;
}


bool            octavews::is_octave(const std::string& varname)
{
    oct_dataset::iterator vfound = _octave_variables.find(varname);
    if (vfound != _octave_variables.end())
        return true;
    return false;
}

void            octavews::workspace_to_interpreter_noautolist()
{
    // Install only internal variables
    if (_oct_int==nullptr) return;
    const oct_dataset& varx = _internal_variables;
    for (const auto& var : varx)
    {
        if (_oct_int->is_variable(var.first))
            _oct_int->assign(var.first, var.second);
        else
            _oct_int->install_variable(var.first, var.second, true);
    }
}

void            octavews::workspace_to_interpreter()
{
    // Install only internal variables
    if (_oct_int==nullptr) return;
    const oct_dataset& varx = _internal_variables;
    for (const auto& var : varx)
    {
        if (_oct_int->is_variable(var.first))
            _oct_int->assign(var.first, var.second);
        else
            _oct_int->install_variable(var.first, var.second, true);

        add_variable_to_updatelist(var.first);
    }

}

void            octavews::interpreter_to_workspace()
{
    // get list of variables
    if (_oct_int==nullptr)
        return;
    clear_octave();
    _updated_vars.clear();

    const std::list<std::string>& varnames = _oct_int->variable_names();
    for (const auto& varname : varnames)
    {
        octave_value val = _oct_int->varval(varname);
        oct_dataset::iterator vfound_int = _internal_variables.find(varname);
        oct_dataset::iterator vfound_oct = _octave_variables.find(varname);

        if (vfound_int != _internal_variables.end())
        {
            octave_value prev = _internal_variables[varname];
            if (!(ov_equal(prev,val)))
            {
                add_variable_to_updatelist(varname);
                _internal_variables[varname] = val;
            }
            continue;
        }
        if (vfound_oct != _internal_variables.end())
        {
            octave_value prev = _octave_variables[varname];
            if (!(ov_equal(prev,val)))
            {
                add_variable_to_updatelist(varname);
                _octave_variables[varname] = val;
            }
            continue;
        }

        _octave_variables[varname] = val;
        add_variable_to_updatelist(varname);
    }

    if (_data_interface!=nullptr)
        emit _data_interface->updatedVariables(_updated_vars);
    update_graphs();
}


octave_value_list octavews::get_var_values(string_vector names)
{
    octave_value_list out;

    for (int s=0; s < names.numel(); s++)
    {
        string vname = names[s];
        if (vname.empty()) {out.append(octave_value()); continue;}
        out.append(var_value(vname));
    }
    return out;
}

void            octavews::set_var_values(string_vector names, octave_value_list values)
{
    for (int s=0; s < names.numel(); s++)
    {
        string varname = names[s];
        if (varname.empty()) continue;
        _oct_int->assign(varname,values(s));
        oct_dataset::iterator vfound = _internal_variables.find(varname);
        if (vfound != _internal_variables.end())
            {vfound->second = values(s); continue;}

        vfound = _octave_variables.find(varname);
        if (vfound != _octave_variables.end())
            {vfound->second = values(s); continue;}

        add_variable(varname,true, values(s));

        add_variable_to_updatelist(varname);
    }
    if (_data_interface!=nullptr)
        emit _data_interface->updatedVariables(_updated_vars);
    update_graphs();

    return;
}


void        octavews::update_after_set_variables()
{
    if (_data_interface!=nullptr)
        emit _data_interface->updatedVariables(_updated_vars);

    update_graphs();

    _updated_vars.clear();
}

void        octavews::set_variable_no_immediate_update(const std::string& varname, const octave_value& value)
{
    oct_dataset::iterator vfound = _internal_variables.find(varname);
    if (vfound != _internal_variables.end())
        vfound->second = value;
    else
    {
        vfound = _octave_variables.find(varname);
        if (vfound != _octave_variables.end())
            vfound->second = value;
        else
            add_variable(varname,false, value);

    }
    _oct_int->assign(varname,value);
}


void        octavews::set_variable(const std::string& varname, const octave_value& value)
{
    oct_dataset::iterator vfound = _internal_variables.find(varname);
    if (vfound != _internal_variables.end())
        vfound->second = value;
    else
    {
        vfound = _octave_variables.find(varname);
        if (vfound != _octave_variables.end())
            vfound->second = value;
        else
        {
            add_variable(varname,false, value);
            add_variable_to_updatelist(varname);
        }
    }
    _oct_int->assign(varname,value);
    if (_data_interface!=nullptr)
        emit _data_interface->updatedVariable(varname);

    update_graphs();
}


string_vector     octavews::get_var_names(bool internal)
{
    const oct_dataset& refds = internal ? _internal_variables : _octave_variables;
    string_vector out;
    for (const auto& var: refds)
        out.append(var.first);
    return out;
}

int               octavews::get_variable_count(bool internal)
{
    return internal? _internal_variables.size() : _octave_variables.size();
}

variable    octavews::get_variable(int id, bool binternal)
{
    variable out;

    const oct_dataset& refds = binternal ? _internal_variables : _octave_variables;

    if ((id<0)||(id>=refds.size()))
        return out;
    oct_dataset::const_iterator var = refds.begin();
    var = std::next(var,id);
    out.first = var->first;
    out.second= var->second;
    return out;
}



void octavews::add_graph(class wndPlot2d* graph)
{
    if (graph!=nullptr)
        _graphs2d.insert(graph);
}

void octavews::remove_graph(class wndPlot2d* graph)
{
    if (_b_deleting) return;
    if (graph!=nullptr)
        _graphs2d.erase(graph);

}

void octavews::update_graphs()
{
    for (const auto& graph: _graphs2d)
        graph->update_workspace(this);
}

void octavews::add_variable_to_updatelist(std::string variable)
{
    if (!variable.empty())
        _updated_vars.insert(variable);
}

std::set<std::string>& octavews::get_updated_variables()
{
    return _updated_vars;
}



void    octavews::save_to_file(std::string filename)
{
    if (filename.empty()) return;
    int parse_status;
    std::string command = "save ( \"-mat\" , \""+ filename + "\")";


    _oct_int->eval_string(command,true,parse_status);

}


