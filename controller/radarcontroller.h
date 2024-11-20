/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef RADARCONTROLLER_H
#define RADARCONTROLLER_H

#include "radarproject.h"
#include <QObject>
#include <QtSerialPort>
#include <QQueue>




class radarController : public projectItem, public QObject
{

private:
    QSerialPort             *_serial_port;
    QQueue<QByteArray>      _data_tx_fifo;
    QQueue<QByteArray>      _data_rx_fifo;
    QTimer                  *_timer;


public:
    radarController(QObject* owner = nullptr, QString controllerName="newController");

    void            start_serial();
    void            append_new_string(const QByteArray& newdata);
    int             to_transmit_count();
    int             received_count();
    QByteArray      get_next_rx_data();
    void            transmit_data(const QByteArray& new_tx_data);
signals:
    void            tx_fifo_empty();
    void            task_complete();
public slots:
    void            data_from_serial_ready();
    void            data_to_serial_complete();
    void            serial_error();
    void            serial_timeout();
};

#endif // RADARCONTROLLER_H
