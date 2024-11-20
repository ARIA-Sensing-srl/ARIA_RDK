/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef SERIALPORT_UTILS_H
#define SERIALPORT_UTILS_H
#include <QPair>
#include <QMap>
#include <QString>
#include <QSerialPort>



extern QMap<QSerialPort::Parity,QString> serial_port_parity_descriptor;

extern QMap<QSerialPort::StopBits,QString> serial_port_stopbits_descriptor;

extern QMap<QSerialPort::FlowControl,QString> serial_port_flowcontrol_descriptor;




#endif // SERIALPORT_UTILS_H
