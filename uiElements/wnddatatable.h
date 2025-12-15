#ifndef WNDDATATABLE_H
#define WNDDATATABLE_H

#include <QDialog>
#include <octaveinterface.h>
namespace Ui {
class wnddatatable;
}

class wnddatatable : public QDialog
{
    Q_OBJECT

public:
    explicit wnddatatable(octaveInterface* datainterface, QString var, QWidget *parent = nullptr);
    ~wnddatatable();

    QString getVariable() {return QString::fromStdString(varName);}

    void updateTable();

private:
    Ui::wnddatatable *ui;

    octaveInterface* dataInterface;
    std::string      varName;
public slots:
    void             workSpaceModified(const std::set<std::string>& vars);
    void             exportToCSV();
    void             exportToMat();
};

#endif // WNDDATATABLE_H
