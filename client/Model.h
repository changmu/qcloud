#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QDebug>
#include <utility>
#include <QThread>
#include "global.h"

class FileListModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit FileListModel(QObject *parent = NULL)
      : QAbstractTableModel(parent),
        horizontalHeader_(QStringList() << "File Name" << "Size(Byte)")
    {}

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override { return horizontalHeader_.size(); }

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setHorizontalHeader(const QStringList& header) { horizontalHeader_ = header; }

private:
    QStringList horizontalHeader_;
};



class StatusModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit StatusModel(QObject *parent = NULL)
      : QAbstractTableModel(parent),
        horizontalHeader_(QStringList() << "File Name" << "Progress" << "Rate" << "Status")
    {
    }

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override { return horizontalHeader_.size(); }

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setHorizontalHeader(const QStringList& header) { horizontalHeader_ = header; }
public slots:

private:
    QStringList horizontalHeader_;
};
