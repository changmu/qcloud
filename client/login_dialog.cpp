#include "login_dialog.h"
#include "ui_login_dialog.h"
#include "signup_dialog.h"
#include "protocol.h"
#include "updatePwd_dialog.h"
#include "global.h"

#include <stdlib.h>
#include <QDebug>
#include <QMessageBox>

#define NAME_OR_PWD_WRONG "wrong username or password"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setFixedSize(width(), height());
    setWindowTitle("QCloud");

    ui->usernameLineEdit->setFocus();
    //ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    //ui->loginButton->setDefault(true);
    QRegExp reName(R"(\w{1,20})");
    ui->usernameLineEdit->setValidator(new QRegExpValidator(reName, this));
    QRegExp rePwd(R"(.{1,20})");
    ui->passwordLineEdit->setValidator(new QRegExpValidator(rePwd, this));
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::setText(const QString &txt, const QString &color)
{
    ui->statusLabel->setText(QString("<font color='%1'>%2</font>")
                             .arg(color).arg(txt));
}

void LoginDialog::on_wrongPwd()
{
    setText(NAME_OR_PWD_WRONG, "red");
}

void LoginDialog::on_loginButton_clicked()
{
    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]";

    setText("logining..");

//    ui->statusLabel->clear();
    username_ = ui->usernameLineEdit->text();
    password_ = ui->passwordLineEdit->text();

    qDebug() << "username: " << username_;
    qDebug() << "password: " << password_;
    if (username_.isEmpty() || password_.isEmpty()) {
        setText(NAME_OR_PWD_WRONG, "red");
        return;
    }

    //accept();
    emit checkUsernamePassword(username_, password_);
}

void LoginDialog::on_signupButton_clicked()
{
    emit singupBtnClicekd();
}

void LoginDialog::on_btnUpdatePwd_clicked()
{
    this->hide();
    g_updateDialog->show();
}
