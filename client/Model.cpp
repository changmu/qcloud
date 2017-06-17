#include "Model.h"
#include "global.h"
#include <QFileInfo>
#include <assert.h>

int FileListModel::rowCount(const QModelIndex &) const
{
    return g_fileList->size();
}

QVariant FileListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Qt::DisplayRole) {
        int ncol = index.column();
        int nrow =  index.row();
        RemoteFile rFile;
        {
            // QMutexLocker locker(&g_mutexFileList);
            rFile = g_fileList->at(nrow);
        }
        if (ncol == 0)
            return QFileInfo(rFile.name).fileName();
        else if (ncol == 1)
            return rFile.size;
    }

    return QVariant();
}

QVariant FileListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return horizontalHeader_.at(section);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int StatusModel::rowCount(const QModelIndex &) const
{
    return g_statusList->size();
}

static QString formatRate(double rate)
{
    const static size_t GB = 1 << 30;
    const static size_t MB = 1 << 20;
    const static size_t KB = 1 << 10;

    if (rate >= GB) {
        return QString::number(rate / GB, 'f', 2) + "GB/s";
    } else if (rate >= MB) {
        return QString::number(rate / MB, 'f', 2) + "MB/s";
    } else if (rate >= KB) {
        return QString::number(rate / KB, 'f', 2) + "KB/s";
    }

    return QString::number(rate, 'f', 2) + " B/s";
}

QVariant StatusModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Qt::DisplayRole) {
        int ncol = index.column();
        int nrow =  index.row();
        FileStatus sFile;
        {
            assert(g_statusList->size() > nrow);
            // QMutexLocker locker(&g_mutexFileStatus);
            sFile = g_statusList->at(nrow);
        }

        qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]"
                 << QThread::currentThread();

        if (ncol == 0)
            return sFile.name;
        else if (ncol == 1)
            return QStringList() << QString::number(sFile.rcvLen)
                                 << QString::number(sFile.totLen);
        else if (ncol == 2)
            return formatRate(sFile.rate);
        else if (ncol == 3)
            return QString(g_stateStr[sFile.status]);
    }

    return QVariant();
}

QVariant StatusModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return horizontalHeader_.at(section);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
