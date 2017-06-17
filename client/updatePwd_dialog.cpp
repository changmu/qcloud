#include "updatePwd_dialog.h"
#include "ui_updatePwd_dialog.h"
#include "global.h"

UpdatePwd_dialog::UpdatePwd_dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::updatePwd_dialog)
{
    ui->setupUi(this);
    setFixedSize(width(), height());
    setWindowTitle("QCloud Update");

    ui->leUsername->setFocus();
    // ui->btnSubmit->setShortcut(Qt::Key_Enter);
    QRegExp reName(R"(\w{1,20})");
    ui->leUsername->setValidator(new QRegExpValidator(reName, this));
    QRegExp rePwd(R"(.{1,20})");
    ui->leOldPwd->setValidator(new QRegExpValidator(rePwd, this));
    ui->leNewPwd->setValidator(new QRegExpValidator(rePwd, this));
    ui->leNewPwd2->setValidator(new QRegExpValidator(rePwd, this));
}

UpdatePwd_dialog::~UpdatePwd_dialog()
{
    delete ui;
}

void UpdatePwd_dialog::setText(const QString &txt, const QString &color)
{
    ui->lbStatus->setText(QString("<font color='%1'>%2</font>")
                             .arg(color).arg(txt));
}

void UpdatePwd_dialog::on_btnSubmit_clicked()
{
    QString name = ui->leUsername->text();
    QString oldPwd = ui->leOldPwd->text();
    QString newPwd = ui->leNewPwd->text();
    QString newPwd2 = ui->leNewPwd2->text();

    if (name.isEmpty() || oldPwd.isEmpty() || newPwd.isEmpty() || newPwd2.isEmpty()) {
        setText("字段不能为空!");
        return;
    }
    if (newPwd != newPwd2) {
        setText("两次新密码输入不一致!");
        return;
    }

    emit updatePwd(name, oldPwd, newPwd);
}
