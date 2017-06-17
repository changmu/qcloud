#include "mainwindow.h"
#include "ProgressBarDelegate.h"
#include "ui_mainwindow.h"
#include "Task.h"
#include "global.h"
#include "Model.h"
#include <QThreadPool>
#include <QFileInfo>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
  : QWidget(parent),
    ui(new Ui::MainWindow),
    viewFile_(new QTableView(this)),
    viewStatus_(new QTableView(this))
{
    ui->setupUi(this);
    setWindowTitle("QCloud");
    setFixedSize(800, 600);
    startTimer(1000);

    viewFile_->setModel(g_modelFiles.get());
    viewFile_->setColumnWidth(0, 600);
    viewFile_->horizontalHeader()->setStretchLastSection(true);
    viewFile_->horizontalHeader()->setHighlightSections(true);
    viewFile_->verticalHeader()->setVisible(true);
    viewFile_->setShowGrid(false);
    viewFile_->setSelectionBehavior(QAbstractItemView::SelectRows);

    viewStatus_->setModel(g_modelStatus.get());
    viewStatus_->horizontalHeader()->setStretchLastSection(true);
    viewStatus_->horizontalHeader()->setHighlightSections(true);
    viewStatus_->verticalHeader()->setVisible(true);
    viewStatus_->setShowGrid(false);
    viewStatus_->setSelectionBehavior(QAbstractItemView::SelectRows);
    viewStatus_->setColumnWidth(1, 400);
    viewStatus_->setItemDelegateForColumn(1, new ProgressBarDelegate);

    ui->tabWidget->clear();
    ui->tabWidget->setGeometry(0, 50, 800, 550);
    ui->tabWidget->addTab(viewFile_, "文件列表");
    ui->tabWidget->addTab(viewStatus_, "传输状态");

    connect(this, SIGNAL(statusChanged(FileStatus)), this, SLOT(on_statusChanged(FileStatus)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::timerEvent(QTimerEvent *)
{
    // emit g_modelStatus->layoutChanged();
#if 0
    for (FileStatus &fs : *g_statusList) {
        fs.rate = 0;
    }
#endif
}


void MainWindow::on_btnUpload_clicked()
{
    QFileDialog fdia;
    QStringList filepaths = fdia.getOpenFileNames(NULL, "Upload", "~");

    for (const auto& name : filepaths) {
        qDebug() << "on_btnUpload_clicked, file: " << name;
        QThreadPool::globalInstance()->start(new TaskUpload(name));
    }
}

void MainWindow::on_btnDownload_clicked()
{
    // get selected files
    QModelIndexList indexList = viewFile_->selectionModel()->selectedRows();
    qDebug() << indexList;
    QStringList files;
    for (const auto& idx : indexList) {
        RemoteFile rf = g_fileList->at(idx.row());
        files << rf.name;
    }
    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]" << "to download:" << files;

    // get dir
    QString dir = QFileDialog().getExistingDirectory(NULL, "Save Dir", *g_localDir);
    if (dir.isEmpty())
        return;
    *g_localDir = dir + "/";
    qDebug() << "g_localDir:" << *g_localDir;

    // download it
    for (const auto& filename : files) {
        // filename: 'qiu/xxx.mkv'
        QThreadPool::globalInstance()->start(new TaskDownload(filename, *g_localDir));
    }
}

void MainWindow::on_btnFlush_clicked()
{
    g_controlSocket->requestFileList();
}

void MainWindow::on_btnDelete_clicked()
{
    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]";
    QModelIndexList indexList = viewFile_->selectionModel()->selectedRows();
    qDebug() << indexList;
    QStringList files;
    for (const auto& idx : indexList) {
        RemoteFile rf = g_fileList->at(idx.row());
        files << rf.name;
    }
    qDebug() << __func__  << __LINE__ << files;
    for (const auto &filename : files) {
        g_controlSocket->deleteFileRequest(filename);
    }
    g_controlSocket->requestFileList();
    emit g_modelFiles->layoutChanged();
}

void MainWindow::on_statusChanged(FileStatus fStatus)
{
    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]"
             << QThread::currentThread();

#if 1
    int i = 0;
    for ( ; i < g_statusList->size(); ++i) {
        if (g_statusList->at(i).name == fStatus.name)
            break;
    }
    if (i == g_statusList->size()) {
        g_statusList->push_back(fStatus);
    } else {
        g_statusList->operator [](i) = fStatus;
    }
#endif

    for (const FileStatus& it : *g_statusList) {
        qDebug() << "FileStatus:" << "name:" << it.name << "totLen:" << it.totLen;
    }
    emit g_modelStatus->layoutChanged();
}
