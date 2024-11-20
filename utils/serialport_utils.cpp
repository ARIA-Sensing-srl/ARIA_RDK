#include <serialport_utils.h>


QMap<QSerialPort::Parity,QString> serial_port_parity_descriptor{ {QSerialPort::NoParity,"No Parity"},
                                                                 {QSerialPort::EvenParity,"Even parity"},
                                                                 {QSerialPort::OddParity, "Odd parity"},
                                                                 {QSerialPort::SpaceParity,"Space parity"},
                                                                 {QSerialPort::MarkParity, "Mark parity"}};

QMap<QSerialPort::StopBits,QString> serial_port_stopbits_descriptor{ {QSerialPort::OneStop,"1 Stopbit"},
                                                                     {QSerialPort::OneAndHalfStop,"1.5 Stopbits"},
                                                                     {QSerialPort::TwoStop, "2 Stopbits"}};

QMap<QSerialPort::FlowControl,QString> serial_port_flowcontrol_descriptor{ {QSerialPort::NoFlowControl,"No Flow Control"},
                                                                           {QSerialPort::HardwareControl,"RTS/CTS"},
                                                                           {QSerialPort::SoftwareControl, "XON/XOFF"}};
