#ifndef FINDSUCCESSDIALOG_H
#define FINDSUCCESSDIALOG_H

#include <QDialog>
#include <memory>
#include "userdata.h"

namespace Ui {
class FindSuccessDialog;
}

class FindSuccessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindSuccessDialog(QWidget *parent = nullptr);
    ~FindSuccessDialog();
    void SetSearchInfo(std::shared_ptr<SearchInfo> si);
private slots:

private:
    Ui::FindSuccessDialog *ui;
    QWidget * _parent;
    std::shared_ptr<SearchInfo> _si;
};

#endif // FINDSUCCESSDIALOG_H
