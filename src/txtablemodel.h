// Copyright 2019-2022 The Hush developers
// Released under the GPLv3
#ifndef STRINGSTABLEMODEL_H
#define STRINGSTABLEMODEL_H

#include "precompiled.h"

struct TransactionItem;

class TxTableModel: public QAbstractTableModel
{
public:
    TxTableModel(QObject* parent);
    ~TxTableModel();

    void addTData    (const QList<TransactionItem>& data);
    void addZSentData(const QList<TransactionItem>& data);
    void addZRecvData(const QList<TransactionItem>& data);

    QString  getTxId(int row) const;
    QString  getMemo(int row) const;
    QString  getAddr(int row) const;
    QString  getFromAddr(int row) const;
    qint64   getDate(int row) const;
    QString  getType(int row) const;
    qint64   getConfirmations(int row) const;
    QString  getAmt (int row) const;

    bool     exportToCsv(QString fileName) const;

    int      rowCount(const QModelIndex &parent) const;
    int      columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QImage colorizeIcon(const QIcon icon, const QColor color) const;

private:
    void updateAllData();

    QList<TransactionItem>*  tTrans      = nullptr;
    QList<TransactionItem>*  zrTrans     = nullptr;     // Z received
    QList<TransactionItem>*  zsTrans     = nullptr;     // Z sent

    QList<TransactionItem>* modeldata    = nullptr;

    QList<QString>           headers;
};


#endif // STRINGSTABLEMODEL_H
