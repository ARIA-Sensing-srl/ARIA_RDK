#ifndef WNDFWUPLOAD_H
#define WNDFWUPLOAD_H

#include <QDialog>
#include <QProcess>
#include <QSerialPortInfo>
namespace Ui {
class wndFWUpload;
}

enum FW_STATUS{FW_IDLE, FW_STARTED_FWUPLOAD, FW_STARTED_RUN, FW_ERROR};

class wndFWUpload : public QDialog
{
    Q_OBJECT

public:
    explicit wndFWUpload(QWidget *parent = nullptr);
    ~wndFWUpload();
    QProcess    _upload_process;

    QString     _last_fw_program;
    QString     _last_bin_file;
    QString     _last_write_flag;
    QString     _last_verify_flag;
    QString     _last_run_flag;
    QString     _last_run_address;
    QString     _last_serial_name;
    QStringList options_write;
    QStringList options_run;
    QSerialPortInfo _desired_port_info;
    FW_STATUS   _status;

private:
    Ui::wndFWUpload *ui;
public slots:
    void    select_uploader();
    void    select_bin_file();
    void    updload_fw();
    void    close_wnd();

    void    proc_started();
    void    proc_done(int exitCode, QProcess::ExitStatus exitStatus = QProcess::NormalExit);
    void    proc_error(QProcess::ProcessError error);
    void    data_received();
};

#endif // WNDFWUPLOAD_H
