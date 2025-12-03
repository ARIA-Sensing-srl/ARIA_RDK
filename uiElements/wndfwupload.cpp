#include "wndfwupload.h"
#include "ui_wndfwupload.h"
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QSerialPortInfo>


extern QString          ariasdk_fw_uploader;
extern QString          ariasdk_fw_bin;
extern QString          ariasdk_flag_write;
extern QString          ariasdk_flag_start;
extern QString          ariasdk_flag_verify;
extern QString          ariasdk_bin_start_address;
extern QDir             ariasdk_uploader_path;
extern QDir             ariasdk_bin_path;
extern QString          ariasdk_serial_name;


wndFWUpload::wndFWUpload(QWidget *parent)
    : QDialog(parent)
    , _status(FW_IDLE)
    , ui(new Ui::wndFWUpload)
{
    ui->setupUi(this);
    ui->leUploaderSelection->setText(ariasdk_fw_uploader);
    ui->leBinSelection->setText(ariasdk_fw_bin);
    ui->leFlagStart->setText(ariasdk_flag_start);
    ui->leFlagVerify->setText(ariasdk_flag_verify);
    ui->leFlagWrite->setText(ariasdk_flag_write);
    ui->leStartAddress->setText(ariasdk_bin_start_address);

    for (auto& sp : QSerialPortInfo::availablePorts())
    {
        QString sName = sp.portName();
        ui->cbSerialPort->addItem(sName);
    }

    ui->cbSerialPort->setCurrentText(ariasdk_serial_name);

    connect(ui->btnUploaderSelection,   &QPushButton::clicked, this, &wndFWUpload::select_uploader);
    connect(ui->btnBinSelection,        &QPushButton::clicked, this, &wndFWUpload::select_bin_file);
    connect(ui->btnUpload,              &QPushButton::clicked, this, &wndFWUpload::updload_fw);
    connect(ui->btnCancel,              &QPushButton::clicked, this, &wndFWUpload::close_wnd);

    connect(&_upload_process,           &QProcess::started,    this, &wndFWUpload::proc_started);
    connect(&_upload_process,           &QProcess::finished,   this, &wndFWUpload::proc_done);
    connect(&_upload_process,           &QProcess::errorOccurred, this, &wndFWUpload::proc_error);
    connect(&_upload_process,           &QProcess::readyReadStandardError, this, &wndFWUpload::data_received);
    connect(&_upload_process,           &QProcess::readyReadStandardOutput, this, &wndFWUpload::data_received);
    _upload_process.blockSignals(false);
}

wndFWUpload::~wndFWUpload()
{
    delete ui;
}


void    wndFWUpload::select_uploader()
{
    QString uploader_file = QFileDialog::getOpenFileName(this, "Select program file", ariasdk_uploader_path.absolutePath(), tr("All files (*.*)" ));
    if (uploader_file.isEmpty()) return;
    ui->leUploaderSelection->setText(uploader_file);

}

void    wndFWUpload::select_bin_file()
{
    QString bin_file = QFileDialog::getOpenFileName(this, "Select binary file", ariasdk_uploader_path.absolutePath(), tr("All files (*.*);;Bin FW (*.bin *.hex)" ));
    if (bin_file.isEmpty()) return;
    ui->leBinSelection->setText(bin_file);
}

void    wndFWUpload::updload_fw()
{

    if ((_status != FW_IDLE)&&(_status != FW_ERROR))
    {
        QMessageBox::critical(this, "Error", tr("Process already running"));
    }
    QString uploader_command = ui->leUploaderSelection->text();
    QString fw_bin_file      = ui->leBinSelection->text();
    QString write_flag       = ui->leFlagWrite->text();
    QString verify_flag      = ui->leFlagVerify->text();
    QString run_flag         = ui->leFlagStart->text();
    QString run_address      = ui->leStartAddress->text();
    _last_serial_name        = ui->cbSerialPort->currentText();

    if (!QFileInfo::exists(uploader_command))
    {
        QMessageBox::critical(this, "Error", "Select a valid program");
        return;
    }

    if (!QFileInfo::exists(fw_bin_file))
    {
        QMessageBox::critical(this, "Error", "Select a valid binary file");
        return;
    }

    /*QByteArray temp = QByteArray::fromHex(run_address.toLatin1());
    if (temp.size()!=8)
    {
        QMessageBox::critical(this,"Error", tr("Please provide a 32bit address"));
        return;
    }*/

    QString serial_system_name("");


    for (auto& sp: QSerialPortInfo::availablePorts())
    {
        if (sp.portName() == _last_serial_name)
        {
            serial_system_name = sp.systemLocation();
        }
    }

    if (serial_system_name.isEmpty())
    {
        QMessageBox::critical(this,"Error",tr("Selected port has been disconnected!"));
        return;
    }

    _last_fw_program = uploader_command;
    _last_bin_file   = fw_bin_file;
    _last_write_flag = write_flag;
    _last_verify_flag= verify_flag;
    _last_run_flag   = run_flag;
    _last_run_address= run_address;
    _last_serial_name = ui->cbSerialPort->currentText();

    options_write.clear();
    if (!_last_write_flag.isEmpty()) options_write << ( QString("-") + _last_write_flag);
    options_write << _last_bin_file;

    if (!_last_verify_flag.isEmpty()) options_write << ( QString("-") + _last_verify_flag);

    options_write << serial_system_name;


    options_run.clear();
    if (!_last_run_flag.isEmpty()) options_run << (QString("-") + _last_run_flag);

    options_run << (QString("0x")+_last_run_address);
    options_run << serial_system_name;

    _status = FW_STARTED_FWUPLOAD;
    _upload_process.start(_last_fw_program, options_write);


}

void    wndFWUpload::close_wnd()
{
    if ((_status!=FW_ERROR)&&(_status!=FW_IDLE))
    {
        if (QMessageBox::warning(this, tr("Warning"), tr("Do you want to stop the FW Upload process?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
            return;

        _upload_process.kill();
    }
    done(QMessageBox::Ok);
}


void   wndFWUpload::proc_started()
{
    ui->teOutput->setTextColor( QColor( "green" ) );
    QString data = _status == FW_STARTED_FWUPLOAD ? tr("Firmware upload") : tr("Firmware start");
    ui->teOutput->append( data );
    // restore
    ui->teOutput->setTextColor( QColor("grey") );
}
void    wndFWUpload::proc_done(int exitCode, QProcess::ExitStatus exitStatus)
{

    if ((exitStatus == QProcess::NormalExit)&&(exitCode==0))
    {
        /*if (_status == FW_STARTED_FWUPLOAD)
        {
            _upload_process.start(_last_fw_program,options_run);
            _status = FW_STARTED_RUN;

            return;
        }

        if (_status == FW_STARTED_RUN)
        {
            _status = FW_IDLE;
            QMessageBox::information(this, tr("Successful"), tr("FW Upload & Started Succesfully"));
            ariasdk_fw_uploader = _last_fw_program;
            ariasdk_uploader_path = QFileInfo(_last_fw_program).absolutePath();
            ariasdk_fw_bin = _last_bin_file;
            ariasdk_bin_path = QFileInfo(_last_bin_file).absolutePath();
            ariasdk_flag_start = _last_run_flag;
            ariasdk_flag_verify= _last_verify_flag;
            ariasdk_flag_write = _last_write_flag;
            ariasdk_serial_name= _last_serial_name;
            ariasdk_bin_start_address  = _last_run_address;
        }*/
        if (_status == FW_STARTED_FWUPLOAD){
            _status = FW_IDLE;
            QMessageBox::information(this, tr("Successful"), tr("FW Upload & Started Succesfully"));
            ariasdk_fw_uploader = _last_fw_program;
            ariasdk_uploader_path = QFileInfo(_last_fw_program).absolutePath();
            ariasdk_fw_bin = _last_bin_file;
            ariasdk_bin_path = QFileInfo(_last_bin_file).absolutePath();
            ariasdk_flag_start = _last_run_flag;
            ariasdk_flag_verify= _last_verify_flag;
            ariasdk_flag_write = _last_write_flag;
            ariasdk_serial_name= _last_serial_name;
            ariasdk_bin_start_address  = _last_run_address;
        }
    }
    else
    {
        QMessageBox::critical(this,tr("Error"),tr("Error while ")+(_status==FW_STARTED_FWUPLOAD? tr("uploading"):tr("starting")));
        _status = FW_ERROR;
        return;
    }

}
void    wndFWUpload::proc_error(QProcess::ProcessError error)
{
    if (_status == FW_STARTED_FWUPLOAD)
    {
        QMessageBox::critical(this, tr("Error"), tr("Error while uploading fw"));
        _status = FW_ERROR;
        return;
    }
    if (_status == FW_STARTED_RUN)
    {
        QMessageBox::critical(this, tr("Error"), tr("Error while uploading fw"));
        _status = FW_ERROR;
        return;
    }
}



void wndFWUpload::data_received()
{
    QByteArray data = _upload_process.readAllStandardError();

    ui->teOutput->setTextColor( QColor( "red" ) );
    ui->teOutput->append( data );
    // restore
    ui->teOutput->setTextColor( QColor("grey") );

    data = _upload_process.readAllStandardOutput();
    ui->teOutput->append( data );
}
