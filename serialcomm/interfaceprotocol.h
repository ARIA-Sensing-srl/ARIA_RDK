/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef INTERFACEPROTOCOL_H
#define INTERFACEPROTOCOL_H
#include <QByteArray>


class interfaceProtocol
{
    bool                        _b_stuffing;
    char                        _start_command;
    char                        _stop_command;
    char                        _stuff_char;
    char                        _alternate_start;
    char                        _alternate_stop;
    char                        _alternate_stuff;

    QByteArray                  _stuffed_start_seq;
    QByteArray                  _stuffed_stop_seq;
    QByteArray                  _stuffed_stuff_seq;


public:
    interfaceProtocol();
    ~interfaceProtocol();

    void         set_stuffing(bool bstuffing=true);
    void         set_start(char start);
    void         set_stop(char stop);
    void         set_stuff(char stuff);
    void         set_alternate_start_char(char alternate);
    void         set_alternate_stop_char(char alternate);
    void         set_alternate_stuff_char(char alternate);

    char         get_start(void);
    char         get_stop(void);
    char         get_stuff(void);
    char         get_alternate_start_char(void);
    char         get_alternate_stop_char(void);
    char         get_alternate_stuff_char(void);

    QByteArray    encode_tx(const QByteArray& tx_data);
    QByteArray    decode_rx(const QByteArray& rx_data);

};

#endif // INTERFACEPROTOCOL_H
