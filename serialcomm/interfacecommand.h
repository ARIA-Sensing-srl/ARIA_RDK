/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef INTERFACECOMMAND_H
#define INTERFACECOMMAND_H

#include "radarparameter.h"
#include <QtXml>


class interfaceCommand
{
private:
    radarParamPointer   _param;
    QString             _command_string;
    QString             _param_id;

public:
    interfaceCommand();
    interfaceCommand(QString param_name, QString command_string);
    interfaceCommand(const interfaceCommand& command);
    ~interfaceCommand();
    QByteArray               compute_string_to_radar();
    bool                     eval_string_from_radar(QString command_resp, QByteArray& str_response);
    void                     save_xml(QDomDocument& owner, QDomElement& root);
    static std::shared_ptr<interfaceCommand> load_xml(QDomDocument& owner, QDomElement& root);

    interfaceCommand&        operator = (const interfaceCommand& command);

    QString                  parameter_name() {return has_param_assigned()? _param->get_name():_param_id;}
    QString                  command_string() {return _command_string;}
    void                     set_param_name(QString param_name);
    void                     set_param(radarParamPointer param_ptr);
    void                     set_command_string(QString command_string);
    bool                     has_param_assigned() {return _param!=nullptr;}
};

#endif // INTERFACECOMMAND_H
