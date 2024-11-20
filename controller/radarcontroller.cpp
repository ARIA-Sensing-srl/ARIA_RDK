/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "radarcontroller.h"
#include "radarproject.h"

//-----------------------------------------------------------------------
radarController::radarController(QObject* owner, QString controllerName) : projectItem(controllerName, DT_SCHEDULER)
{
    _serial_port = new QSerialPort(owner);
    _timer       = new QTimer(this);
}
//-----------------------------------------------------------------------
void    radarController::start_serial()
{
    connect(_serial_port, &QSerialPort::readyRead,      this, &radarController::data_from_serial_ready);
    connect(_serial_port, &QSerialPort::bytesWritten,   this, &radarController::data_to_serial_complete);
}
//-----------------------------------------------------------------------
void    radarController::data_from_serial_ready()
{
    const QByteArray data = _serial_port->readAll();
    _data_rx_fifo.enqueue(data);
}
//-----------------------------------------------------------------------
void    radarController::data_to_serial_complete()
{
    if (!_data_tx_fifo.isEmpty())
    {

    }
    else
    {
        emit tx_fifo_empty();
    }
}
//-----------------------------------------------------------------------
void    radarController::tx_fifo_empty()
{

}
//-----------------------------------------------------------------------
void    radarController::append_new_string(const QByteArray& newdata)
{

}
//-----------------------------------------------------------------------
int     radarController::to_transmit_count()
{
    return _data_tx_fifo.count();
}
//-----------------------------------------------------------------------
int     radarController::received_count()
{
    return _data_rx_fifo.count();
}
//-----------------------------------------------------------------------
QByteArray  radarController::get_next_rx_data()
{
    return _data_rx_fifo.dequeue();
}
//-----------------------------------------------------------------------
void    radarController::task_complete()
{

}
