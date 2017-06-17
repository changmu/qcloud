#ifndef PROGRESSBARDELEGATE_H
#define PROGRESSBARDELEGATE_H

#include <QItemDelegate>
#include <QPainter>
#include <QApplication>
#include <QDebug>
#include "global.h"

class ProgressBarDelegate : public QItemDelegate {
    Q_OBJECT
public:
    ProgressBarDelegate(QObject *parent = NULL)
      : QItemDelegate(parent)
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        if(index.column() == 1) {
            int row = index.row();
            if (row >= g_statusList->size()) {
                return QItemDelegate::paint(painter, option, index);
            }

            FileStatus fs = g_statusList->at(row);

            uint64_t rcvLen = fs.rcvLen;
            uint64_t totLen = fs.totLen;
            if (totLen == 0) {
                totLen = rcvLen = 100;
            }
            QStyleOptionProgressBarV2 progressBarOption;
            progressBarOption.rect = option.rect.adjusted(4, 4, -4, -4);
            progressBarOption.minimum = 0;
            progressBarOption.maximum = totLen;
            qDebug() << "maximum = " << totLen;
            qDebug() << "progress = " << rcvLen;
            progressBarOption.textAlignment = Qt::AlignRight;
            progressBarOption.textVisible = true;
            progressBarOption.progress = rcvLen;
            progressBarOption.text = tr("%1%").arg(rcvLen * 100 / totLen);

            painter->save();
            if (option.state & QStyle::State_Selected) {
                painter->fillRect(option.rect, option.palette.highlight());
                        painter->setBrush(option.palette.highlightedText());
            }
                    QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);

            painter->restore();

        } else {
            return QItemDelegate::paint (painter, option, index);
        }
    }
};

#endif // PROGRESSBARDELEGATE_H
