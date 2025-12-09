#ifndef WNDDATATABLE_H
#define WNDDATATABLE_H

#include <QDialog>

namespace Ui {
class wnddatatable;
}

class wnddatatable : public QDialog
{
    Q_OBJECT

public:
    explicit wnddatatable(QWidget *parent = nullptr);
    ~wnddatatable();

private:
    Ui::wnddatatable *ui;
};

#endif // WNDDATATABLE_H
