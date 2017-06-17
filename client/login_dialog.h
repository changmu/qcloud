#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();

    void setText(const QString& txt, const QString &color = "green");
    void closeEvent(QCloseEvent *) override { exit(1); }

signals:
    void singupBtnClicekd();
    void checkUsernamePassword(QString name, QString pwd);

private slots:
    void on_wrongPwd();

    void on_loginButton_clicked();

    void on_signupButton_clicked();

    void on_btnUpdatePwd_clicked();

private:
    bool checkUsernamePassword() const;

public:
    Ui::LoginDialog *ui;
private:
    QString username_;
    QString password_;
};

#endif // DIALOG_H
