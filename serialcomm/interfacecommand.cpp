/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "interfacecommand.h"
//---------------------------------------
interfaceCommand::interfaceCommand() :
    _param(nullptr),
    _command_string("__unknown__"),
    _param_id("__unkown__")
{

}
//---------------------------------------
interfaceCommand::interfaceCommand(const interfaceCommand& command) :
    _param(command._param),
    _command_string(command._command_string),
    _param_id(command._param_id)

{

}
//---------------------------------------
interfaceCommand::interfaceCommand(QString param_name, QString command_string) :
    _param(nullptr),
    _command_string(command_string),
    _param_id(param_name)
{

}
//---------------------------------------
interfaceCommand::~interfaceCommand()
{

}
//---------------------------------------
QByteArray interfaceCommand::compute_string_to_radar()
{
    QByteArray out;
    if (_param==nullptr)
        return out;

    out.append(_command_string.toLocal8Bit());

    // OUTPUT params do not see active payload so we may close the param string
    if (_param->get_io_type()==RPT_IO_OUTPUT)
        return out;

    out.append(_param->chain_values());

    return out;
}


//---------------------------------------
bool interfaceCommand::eval_string_from_radar(QString command_resp, QByteArray& str_response)
{
    if (command_resp != _command_string)
        return false;

    if (_param==nullptr)
        return false;

    return _param->split_values(str_response);
}

//---------------------------------------
void    interfaceCommand::save_xml(QDomDocument& owner, QDomElement& root)
{
    QDomElement command_node = owner.createElement("command");
    command_node.setAttribute("param_name", parameter_name());
    command_node.setAttribute("command_string",_command_string);
    root.appendChild(command_node);
}
//---------------------------------------
std::shared_ptr<interfaceCommand>  interfaceCommand::load_xml(QDomDocument& owner, QDomElement& root)
{
    std::shared_ptr<interfaceCommand> out = nullptr;
    if (root.isNull())
        root = owner.firstChildElement("command");

    if (root.isNull())
        return out;


    if (!root.hasAttribute("param_name")) return out;
    if (!root.hasAttribute("command_string")) return out;


    QString str_param = root.attribute("param_name");
    QString str_command=root.attribute("command_string");
    out = std::make_shared<interfaceCommand>(str_param, str_command);

    return out;
}
//---------------------------------------
void    interfaceCommand::set_param_name(QString param_name)
{
    if (_param->get_name()!=param_name)
    {
        _param = nullptr;
        _param_id = param_name;
    }
}
//---------------------------------------
void    interfaceCommand::set_param(radarParamPointer param_ptr)
{
    _param = param_ptr;
    _param_id = "";
}
//---------------------------------------
void    interfaceCommand::set_command_string(QString command_string)
{
    _command_string = command_string;
}
//---------------------------------------
interfaceCommand&   interfaceCommand::operator = (const interfaceCommand& command)
{
    _param = command._param;
    _param_id = command._param_id;
    _command_string = command._command_string;
    return *this;
}
