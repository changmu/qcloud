#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QTableView>
#include <QAbstractTableModel>
#include "global.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void timerEvent(QTimerEvent *) override;
    void closeEvent(QCloseEvent *) override { exit(1); }

signals:
    void statusChanged(FileStatus fStatus);

private slots:
    void on_btnUpload_clicked();

    void on_btnDownload_clicked();

    void on_btnFlush_clicked();

    void on_btnDelete_clicked();

    void on_statusChanged(FileStatus fStatus);

private:
    Ui::MainWindow *ui;

    QTableView *viewFile_;
    QTableView *viewStatus_;
};

#endif // MAINWINDOW_H
