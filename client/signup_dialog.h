#ifndef SIGNUP_DIALOG_H
#define SIGNUP_DIALOG_H

#include <QDialog>

namespace Ui {
class SignupDialog;
}

class SignupDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SignupDialog(QWidget *parent = 0);
    ~SignupDialog();

    void closeEvent(QCloseEvent *) override { exit(1); }

signals:
    void signup(QString name, QString pwd);

public slots:
    void onSignupFailed(QString msg);

private slots:
    void on_signupButton_clicked();

private:
    void setText(const QString &txt, const QString &color = "red");

private:
    Ui::SignupDialog *ui;
};

#endif // SIGNUP_DIALOG_H
