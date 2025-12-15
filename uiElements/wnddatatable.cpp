#include "wnddatatable.h"
#include "ui_wnddatatable.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>

extern QDir             ariasdk_data_path;

wnddatatable::wnddatatable(octaveInterface* datainterface, QString var, QWidget *parent)
    : QDialog(parent)
    , dataInterface(datainterface)
    , ui(new Ui::wnddatatable)
{
    ui->setupUi(this);
    varName = var.toStdString();
    ui->leVarName->setText(var);
    this->setWindowTitle(QString("Data view : ")+var);
    ui->dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    updateTable();
}

wnddatatable::~wnddatatable()
{
    delete ui;
}

void wnddatatable::exportToCSV()
{
    QDateTime date = QDateTime::currentDateTime();
    QString formatted_date_tme = date.toString("yyyy_MM_dd_hh_mm_ss");

    QString dataFile = QFileDialog::getSaveFileName(this,"Comma Separated Values file",
                                                    ariasdk_data_path.absolutePath()+QDir::separator()+QString("data_")+formatted_date_tme+QString(".csv"),
                                                    tr("Data workspace(*.mat);;All files (*.*)"),
                                                    nullptr,
                                                    QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));
    if (dataFile.isEmpty())
        return;

    ariasdk_data_path = QFileInfo(dataFile).absolutePath()+QDir::separator();

    QFile file(dataFile);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "# Data exported from ARIA-RDK : " << formatted_date_tme << "\n";
        out << "# Var name : " << QString::fromStdString(varName) << "\n";
        out << "# n Rows : " << ui->dataTable->rowCount() << " ; n Cols : " << ui->dataTable->columnCount() << "\n";
        for (int r = 0; r < ui->dataTable->rowCount(); r++)
            for (int c = 0; c < ui->dataTable->columnCount(); c++)
            {
                out << ui->dataTable->item(r,c)->text();
                out << ((c < ui->dataTable->columnCount() - 1) ? " , " : "\n");
            }
    }
    file.close();

}
void wnddatatable::exportToMat()
{
    if ((dataInterface == nullptr)||(dataInterface->get_workspace()==nullptr))
    {
        QMessageBox::warning(this, "Warning", "No available workspace");
        return;
    }

    QDateTime date = QDateTime::currentDateTime();
    QString formatted_date_tme = date.toString("yyyy_MM_dd_hh_mm_ss");

    QString dataFile = QFileDialog::getSaveFileName(this,"Workspace file",
                                                       ariasdk_data_path.absolutePath()+QDir::separator()+QString("data_")+formatted_date_tme+QString(".mat"),
                                                       tr("Data workspace(*.mat);;All files (*.*)"),
                                                       nullptr,
                                                       QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));
    if (dataFile.isEmpty())
        return;

    ariasdk_data_path = QFileInfo(dataFile).absolutePath()+QDir::separator();

    dataInterface->get_workspace()->save_to_file(dataFile.toStdString());

}


void wnddatatable::workSpaceModified(const std::set<std::string>& vars)
{

    updateTable();
}

void wnddatatable::updateTable()
{
    if (dataInterface==nullptr)
    {
        ui->dataTable->clear();
        return;
    }

    if ((!dataInterface->get_workspace()->is_internal(varName))&&(!dataInterface->get_workspace()->is_octave(varName)))
    {
        ui->dataTable->clear();
        QPalette palette = ui->label->palette();
        palette.setColor(QPalette::WindowText, Qt::red);
        ui->leWarning->setPalette(palette);
        ui->leWarning->setText("Variable not found");
        ui->leWarning->setVisible(true);

        return;
    }
    ui->leWarning->setVisible(false);
    octave_value& val = dataInterface->get_workspace()->var_value(varName);

    if ((val.ndims()>2)||(val.ndims()==0))
    {
        QPalette palette = ui->label->palette();
        palette.setColor(QPalette::WindowText, Qt::red);
        ui->leWarning->setPalette(palette);
        ui->leWarning->setText(val.ndims()==0 ? "Empty data": "Up to 2D data can be shown here");
        ui->leWarning->setVisible(true);
        ui->dataTable->clear();

        return;
    }

    ui->leWarning->setVisible(false);
    int nrows = val.dims()(0);

    int ncols = val.ndims()>1 ? val.dims()(1) : 1;

    ui->dataTable->setColumnCount(ncols);
    ui->dataTable->setRowCount(nrows);

    if (val.is_complex_matrix())
    {
        ComplexNDArray cdata = val.complex_array_value();
        for (int r=0; r < nrows; r++)
            for (int c=0; c < ncols; c++)
            {
                Complex cval = cdata(r,c);
                QString sign = cval.imag() > 0 ? "+i" : "-i";
                ui->dataTable->setItem(r,c,new QTableWidgetItem(
                                    QString::number(cval.real(),'e',5)+sign+
                                    QString::number(fabs(cval.imag()),'e',5)));
            }

    }

    if (val.is_bool_matrix())
    {
        boolNDArray bdata = val.bool_array_value();
        for (int r=0; r < nrows; r++)
            for (int c=0; c < ncols; c++)
            {
                bool bval = bdata(r,c);
                ui->dataTable->setItem(r,c,new QTableWidgetItem(bval?"true":"false"));
            }

    }

    if (val.is_double_type())
    {
        NDArray ddata = val.array_value();
        for (int r=0; r < nrows; r++)
            for (int c=0; c < ncols; c++)
            {
                double dval = ddata(r,c);
                ui->dataTable->setItem(r,c,new QTableWidgetItem(QString::number(dval,'e',5)));
            }
    }

    if (val.is_int16_type())
    {
        int16NDArray idata = val.int16_array_value();
        for (int r=0; r < nrows; r++)
            for (int c=0; c < ncols; c++)
            {
                int16_t dval = idata(r,c);
                ui->dataTable->setItem(r,c,new QTableWidgetItem(QString::number(dval)));
            }
    }


    if (val.is_int32_type())
    {
        int32NDArray idata = val.int32_array_value();
        for (int r=0; r < nrows; r++)
            for (int c=0; c < ncols; c++)
            {
                int32_t dval = idata(r,c);
                ui->dataTable->setItem(r,c,new QTableWidgetItem(QString::number(dval)));
            }
    }

    if (val.is_int64_type())
    {
        int64NDArray idata = val.int64_array_value();
        for (int r=0; r < nrows; r++)
            for (int c=0; c < ncols; c++)
            {
                int64_t dval = idata(r,c);
                ui->dataTable->setItem(r,c,new QTableWidgetItem(QString::number(dval)));
            }
    }

    if (val.is_uint16_type())
    {
        uint16NDArray idata = val.uint16_array_value();
        for (int r=0; r < nrows; r++)
            for (int c=0; c < ncols; c++)
            {
                uint16_t dval = idata(r,c);
                ui->dataTable->setItem(r,c,new QTableWidgetItem(QString::number(dval)));
            }
    }


    if (val.is_uint32_type())
    {
        uint32NDArray idata = val.uint32_array_value();
        for (int r=0; r < nrows; r++)
            for (int c=0; c < ncols; c++)
            {
                uint32_t dval = idata(r,c);
                ui->dataTable->setItem(r,c,new QTableWidgetItem(QString::number(dval)));
            }
    }

    if (val.is_uint64_type())
    {
        uint64NDArray idata = val.uint64_array_value();
        for (int r=0; r < nrows; r++)
            for (int c=0; c < ncols; c++)
            {
                uint64_t dval = idata(r,c);
                ui->dataTable->setItem(r,c,new QTableWidgetItem(QString::number(dval)));
            }
    }
}

