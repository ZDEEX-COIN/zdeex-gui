// Copyright 2019-2022 The Hush developers
// Released under the GPLv3
#ifndef VIEWALLADDRESSES_H
#define VIEWALLADDRESSES_H

#include "precompiled.h"
#include "rpc.h"

class ViewAllAddressesModel : public QAbstractTableModel {

	Q_OBJECT

public:
    ViewAllAddressesModel(QTableView* parent, QList<QString> taddrs, RPC* rpc);
    ~ViewAllAddressesModel() = default;

    int      rowCount(const QModelIndex &parent) const;
    int      columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<QString> addresses;
    QStringList headers;    
    RPC* rpc;
};

#endif
