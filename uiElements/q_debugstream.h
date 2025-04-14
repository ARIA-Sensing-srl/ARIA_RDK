/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#ifndef Q_DEBUGSTREAM_H
#define Q_DEBUGSTREAM_H
#ifndef ThreadLogStream_H
#define ThreadLogStream_H
#include <iostream>
#include <streambuf>
#include <string>
#include <QScrollBar>
#include "QTextEdit"
#include "QDateTime"
#include "QFile"
#include "QFileInfo"
#include <aria_rdk_interface_messages.h>
#include <octaveinterface.h>

// MessageHandler
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
class MessageHandler : public QObject
{
    Q_OBJECT
public :
    MessageHandler(QTextEdit *textEdit, QObject * parent = Q_NULLPTR) : QObject(parent), m_textEdit(textEdit){}


public slots:
    void catchMessage(QString msg)
    {
        if (this->m_textEdit == nullptr) return;
        this->m_textEdit->append(msg);
        this->m_textEdit->moveCursor(QTextCursor::End);
    }
    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
private:
    QTextEdit * m_textEdit;
};
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/


class ThreadLogStream : public QObject, public std::basic_streambuf<char>
{


    Q_OBJECT
public:
	ThreadLogStream(std::ostream &stream, QObject * parent = Q_NULLPTR) :QObject(parent), m_stream(stream), octInt(nullptr), m_interface_file("")
    {
        m_old_buf = stream.rdbuf();
        stream.rdbuf(this);
    }
    ~ThreadLogStream()
    {
        // output anything that is left
        if (!m_string.empty())
        {
            emit sendLogString(QString::fromStdString(m_string));
        }
        m_stream.rdbuf(m_old_buf);
		cleanUpFile();
    }
	void setInterface(octaveInterface* ptr_interface) {octInt = ptr_interface;}
	void cleanUpFile() {if (m_interface_file.empty()) return;
						QFileInfo fi(QString::fromStdString(m_interface_file));
						if (fi.fileName()!=".rdk_tmp.atp") return;
						std::remove(m_interface_file.c_str());
						m_interface_file.clear();
		}
protected:
    virtual int_type overflow(int_type v)
    {
        if (v == '\n')
        {
            emit sendLogString(QString::fromStdString(m_string));
            m_string.erase(m_string.begin(), m_string.end());
        }
        else
            m_string += v;
        return v;
    }
    virtual std::streamsize xsputn(const char *p, std::streamsize n)
    {
        m_string.append(p, p + n);
        QString msg = QString::fromStdString(m_string);
        QString varname="";
        QString filepath="";

        int n_update = msg.lastIndexOf(QString::fromStdString(str_message_immediate_update));
        int n_inquiry= msg.lastIndexOf(QString::fromStdString(str_message_immediate_inquiry));
        int n_command= msg.lastIndexOf(QString::fromStdString(str_message_immediate_command));


        bool b_immediate_update = false;
        bool b_immediate_inquiry= false;
        bool b_immediate_command= false;

        bool b_any_command = (n_update >= 0) || (n_inquiry >=0) || (n_command>=0);

        if (b_any_command)
        {
            int ns = n_update >= 0 ? n_update : (n_inquiry >=0 ? n_inquiry : n_command);
            int nfixed_length = n_update >= 0 ? str_message_immediate_update.length() : (n_inquiry >= 0 ? str_message_immediate_inquiry.length() : str_message_immediate_command.length());
            int right_n =  msg.length() - ( ns + nfixed_length);
            QString var_file = msg.right(right_n).simplified();
            int comma   = var_file.indexOf(',',0);
            if (comma >=0)
            {
                varname = var_file.left(comma).simplified();
                filepath= var_file.right(var_file.length()-comma-1);
                // Check that file exists
                if (!QFile(filepath).exists())
                    filepath.clear();

                if ((!varname.isEmpty()&&(!filepath.isEmpty())))
                {
                    b_immediate_update = n_update >= 0;
                    b_immediate_inquiry= n_inquiry>= 0;
                    b_immediate_command= n_command>= 0;
                }
            }
        }

        bool b_any_plot = false;
        // Check for plots
        int n_plot =  msg.lastIndexOf(QString::fromStdString(str_message_plot));
        b_any_plot |= (n_plot >=0);

        if (b_any_plot)
        {

        }

        long pos = 0;
        while (pos != static_cast<long>(std::string::npos))
        {
            pos = static_cast<long>(m_string.find('\n'));
            if (pos != static_cast<long>(std::string::npos))
            {
                std::string tmp(m_string.begin(), m_string.begin() + pos);
                QString msg = QString::fromStdString(tmp);
                if (b_any_command) msg.clear();
                if ((!b_any_command)&&(!b_any_plot))
                    emit sendLogString(msg);
                m_string.erase(m_string.begin(), m_string.begin() + pos + 1);
            }
        }
        if ((octInt!=nullptr)&&(!varname.isEmpty())&&(!filepath.isEmpty()))
        {
			m_interface_file = filepath.toStdString();
            if (b_immediate_update)
				octInt->immediate_update_of_radar_var(varname.toStdString(),m_interface_file);
            if (b_immediate_inquiry)
				octInt->immediate_inquiry_of_radar_var(varname.toStdString(),m_interface_file);
            if (b_immediate_command)
				octInt->immediate_command(varname.toStdString(),m_interface_file);
        }

        return n;
    }
private:
    std::ostream &m_stream;
    std::streambuf *m_old_buf;
    std::string m_string;
    octaveInterface*        octInt;
	std::string	m_interface_file;
    //mdiOctaveInterface*     _wndOctave;
signals:
    void sendLogString(const QString& str);

};
#endif // ThreadLogStream_H

#endif // Q_DEBUGSTREAM_H
