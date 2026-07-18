#ifndef SEARCHLIST_H
#define SEARCHLIST_H
#include <QListWidget>
#include <QWheelEvent>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>
#include "loadingdialog.h"
#include <memory>
#include "global.h"
#include "userdata.h"

class SearchList: public QListWidget
{
    Q_OBJECT
public:
    SearchList(QWidget *parent = nullptr);
    void CloseFindDlg();
    void SetSearchEdit(QWidget* edit);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override ;
private:
    void waitPending(bool pending = true);
    bool _send_pending;
    void addTipItem();
    QDialog* _find_dlg = nullptr;
    QWidget* _search_edit;
    LoadingDialog * _loadingDialog;
private slots:
    void slot_item_clicked(QListWidgetItem *item);
    void slot_user_search(std::shared_ptr<SearchInfo> si);
signals:

};
#endif // SEARCHLIST_H
