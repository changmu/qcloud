#include "signup_dialog.h"
#include "ui_signup_dialog.h"

#include <QMessageBox>

SignupDialog::SignupDialog(QWidget *parent)
  : QDialog(parent),
    ui(new Ui::SignupDialog)
{
    ui->setupUi(this);
    setFixedSize(width(), height());
    setWindowTitle("QCloud");

    QRegExp re(R"(\w{1,20})");
    ui->usernameLineEdit->setValidator(new QRegExpValidator(re, this));
    QRegExp rePwd(R"(.{1,20})");
    ui->passwordLineEdit->setValidator(new QRegExpValidator(rePwd, this));
    ui->password2LineEdit->setValidator(new QRegExpValidator(rePwd, this));
}

SignupDialog::~SignupDialog()
{
    delete ui;
}

void SignupDialog::onSignupFailed(QString msg)
{
    setText(msg);
}

void SignupDialog::setText(const QString &txt, const QString &color)
{
    ui->statusLabel->setText(QString("<font color='%1'>%2</font>")
                             .arg(color).arg(txt));
}

void SignupDialog::on_signupButton_clicked()
{
    QString name = ui->usernameLineEdit->text();
    QString pwd = ui->passwordLineEdit->text();
    QString pwd2 = ui->password2LineEdit->text();

    if (name.isEmpty() || pwd.isEmpty() || pwd2.isEmpty()) {
        setText("字段不能为空");
        return;
    }
    if (pwd != pwd2) {
        setText("两次密码输入不一致");
        return;
    }

    setText("注册中 ...", "green");
    emit signup(name, pwd);
}
