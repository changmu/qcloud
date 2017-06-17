#pragma once
#include "login_dialog.h"
#include "signup_dialog.h"
#include "mainwindow.h"
#include "global.h"
#include "tcpsocket.h"
#include <string>

class App : QObject {
    Q_OBJECT
public:
    explicit App(const std::string& configPath);

    void start();

signals:

private slots:
    void on_checkUsernamePassword(QString name, QString pwd);
    void on_signup(QString name, QString pwd);
    void on_updatePwd(QString name, QString oldPwd, QString newPwd);

};
