// Copyright 2019-2022 The Hush developers
// Released under the GPLv3
#ifndef SENTTXSTORE_H
#define SENTTXSTORE_H

#include "precompiled.h"
#include "mainwindow.h"
#include "rpc.h"

class SentTxStore {
public:
    static void deleteHistory();

    static QList<TransactionItem> readSentTxFile();
    static void                   addToSentTx(Tx tx, QString txid);

private:
    static QString writeableFile();
    
};

#endif // SENTTXSTORE_H
