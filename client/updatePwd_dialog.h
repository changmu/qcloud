#ifndef UPDATEPWD_DIALOG_H
#define UPDATEPWD_DIALOG_H

#include <QDialog>

namespace Ui {
class updatePwd_dialog;
}

class UpdatePwd_dialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdatePwd_dialog(QWidget *parent = 0);
    ~UpdatePwd_dialog();

    void setText(const QString &txt, const QString &color = "red");
    void closeEvent(QCloseEvent *) override { exit(1); }

signals:
    void updatePwd(QString name, QString oldPwd, QString newPwd);

private slots:
    void on_btnSubmit_clicked();

private:
    Ui::updatePwd_dialog *ui;
};

#endif // UPDATEPWD_DIALOG_H
