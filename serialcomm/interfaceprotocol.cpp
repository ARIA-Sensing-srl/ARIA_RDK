/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "interfaceprotocol.h"

interfaceProtocol::interfaceProtocol() :
        _b_stuffing(false)
{
    set_stuffing(true);
    set_start(0xFF);
    set_stop(0x00);
    set_alternate_start_char(0xFE);
    set_alternate_stop_char(0x01);
    set_alternate_stuff_char(0x81);
    set_stuff(0x80);
}

interfaceProtocol::~interfaceProtocol()
{

}


void         interfaceProtocol::set_stuffing(bool bstuffing)
{
    _b_stuffing = bstuffing;
}
void         interfaceProtocol::set_start(char start)
{
    _start_command = start;
}

void         interfaceProtocol::set_stop(char stop)
{
    _stop_command = stop;
}
void         interfaceProtocol::set_stuff(char stuff)
{
    _stuff_char = stuff;
    char stuff_seq[] = {_stuff_char, _alternate_stuff};
    char start_seq[] = {_stuff_char, _alternate_start};
    char stop_seq[] = {_stuff_char, _alternate_stop};
    _stuffed_stuff_seq = QByteArray(stuff_seq,2);
    _stuffed_start_seq = QByteArray(start_seq,2);
    _stuffed_stop_seq = QByteArray(stop_seq,2);
}
void         interfaceProtocol::set_alternate_start_char(char alternate)
{
    _alternate_start = alternate;
    char start_seq[] ={_stuff_char, _alternate_start};
    _stuffed_start_seq = QByteArray(start_seq,2);

}
void         interfaceProtocol::set_alternate_stop_char(char alternate)
{
    _alternate_stop = alternate;
    char stop_seq[] = {_stuff_char, _alternate_stop};
    _stuffed_stop_seq = QByteArray(stop_seq,2);;
}
void         interfaceProtocol::set_alternate_stuff_char(char alternate)
{
    _alternate_stuff = alternate;
    char stuff_seq[] = {_stuff_char, _alternate_stuff};
    _stuffed_stuff_seq = QByteArray(stuff_seq,2);
}

char         interfaceProtocol::get_start(void) {return _start_command;}
char         interfaceProtocol::get_stop(void)  {return _stop_command;}
char         interfaceProtocol::get_stuff(void) {return _stuff_char;}
char         interfaceProtocol::get_alternate_start_char(void) {return _alternate_start;}
char         interfaceProtocol::get_alternate_stop_char(void)  {return _alternate_stop;}
char         interfaceProtocol::get_alternate_stuff_char(void) {return _alternate_stuff;}

QByteArray    interfaceProtocol::encode_tx(const QByteArray& tx_data)
{
    if (!_b_stuffing)
        return tx_data;

    QByteArray out;

    for (const auto& byte : tx_data)
    {
        if (byte==_start_command)
        {
            out.push_back(_stuff_char);
            out.push_back(_alternate_start);
            continue;
        }
        if (byte==_stop_command)
        {
            out.push_back(_stuff_char);
            out.push_back(_alternate_stop);

            continue;
        }
        if (byte==_stuff_char)
        {
            out.push_back(_stuff_char);
            out.push_back(_alternate_stuff);
            continue;
        }
        out.push_back(byte);
    }
    return out;
}


QByteArray    interfaceProtocol::decode_rx(const QByteArray& rx_data)
{
    if (!_b_stuffing)
        return rx_data;

    QByteArray out;

    if (rx_data.size()<2)
        return out;

    if (!rx_data.startsWith(_start_command))
        return out;

    if (!rx_data.endsWith(_stop_command))
        return out;

    out.reserve(rx_data.length());
    bool bcontrol = false;

    QByteArray data = rx_data.mid(1,rx_data.size()-2);

    for (const auto& byte : data)
    {
        if (byte==_stuff_char)
        {
            bcontrol = true;
            continue;
        }

        if (bcontrol)
        {
            if (byte == _alternate_start)
            {
                out.push_back(_start_command);
                bcontrol = false;
                continue;
            }
            if (byte == _alternate_stop)
            {
                out.push_back(_stop_command);
                bcontrol = false;
                continue;
            }
            if (byte == _alternate_stuff)
            {
                out.push_back(_stuff_char);
                bcontrol = false;
                continue;
            }
            // wrong decoded sequence
            out.clear();
            return out;
        } // bcontrol

        out.push_back(byte);
    }

    return out;
}
