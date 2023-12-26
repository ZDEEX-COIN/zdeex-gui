// Copyright 2019-2022 The Hush developers
// Released under the GPLv3
#ifndef RPCCLIENT_H
#define RPCCLIENT_H

#include "precompiled.h"
#include "balancestablemodel.h"
#include "txtablemodel.h"
#include "peerstablemodel.h"
#include "bannedpeerstablemodel.h"
#include "ui_mainwindow.h"
#include "mainwindow.h"
#include "connection.h"

struct TransactionItem {
    QString         type;
    qint64          datetime;
    QString         address;
    QString         txid;
    double          amount;
    unsigned long   confirmations;
    QString         fromAddr;
    QString         memo;
};

struct BannedPeerItem {
    QString     address;
    QString     subnet;
    qint64      banned_until;
    qint64      asn;
};

struct PeerItem {
    qint64          peerid;
    QString         type;
    qint64          conntime;
    QString         address;
    qint64          asn;
    QString         tls_cipher;
    bool            tls_verified;
    qint64          banscore;
    qint64          protocolversion;
    QString         subver;
    qint64          bytes_received;
    qint64          bytes_sent;
    double          pingtime;
};


struct WatchedTx {
    QString opid;
    Tx tx;
    std::function<void(QString, QString)> completed;
    std::function<void(QString, QString)> error;
};

class RPC
{
public:
    RPC(MainWindow* main);
    ~RPC();

    void setConnection(Connection* c);
    void setEHushd(std::shared_ptr<QProcess> p);
    const QProcess* getEHushD() { return ehushd.get(); }

    void refresh(bool force = false);
    void pauseTimer();
    void restartTimer();

    void refreshAddresses();    
    void refreshRescan();
    void refreshPeers();
    void setban(QString ip, QString command, const std::function<void(QJsonValue)>& cb);
    void clearBanned(const std::function<void(QJsonValue)>& cb);

    void checkForUpdate(bool silent = true);
    void refreshPrice();

    void executeTransaction(Tx tx, 
        const std::function<void(QString opid)> submitted,
        const std::function<void(QString opid, QString txid)> computed,
        const std::function<void(QString opid, QString errStr)> error);

    void fillTxJsonParams(QJsonArray& params, Tx tx);
    void sendZTransaction(QJsonValue params, const std::function<void(QJsonValue)>& cb, const std::function<void(QString)>& err);
    void shieldCoinbase(QJsonArray& params, const std::function<void(QJsonValue)>& cb, const std::function<void(QString)>& err);
    void mergeToAddress(QJsonArray& params, const std::function<void(QJsonValue)>& cb, const std::function<void(QString)>& err);
    void watchTxStatus();

    const QMap<QString, WatchedTx> getWatchingTxns() { return watchingOps; }
    void addNewTxToWatch(const QString& newOpid, WatchedTx wtx); 

    const TxTableModel*               getTransactionsModel() { return transactionsTableModel; }
    const PeersTableModel*            getPeersModel()        { return peersTableModel; }
    const QList<QString>*             getAllZAddresses()     { return zaddresses; }
    const QList<QString>*             getAllTAddresses()     { return taddresses; }
    const QList<UnspentOutput>*       getUTXOs()             { return utxos; }
    const QMap<QString, double>*      getAllBalances()       { return allBalances; }
    const QMap<QString, bool>*        getUsedAddresses()     { return usedAddresses; }

    void newZaddr(const std::function<void(QJsonValue)>& cb);
    void newTaddr(const std::function<void(QJsonValue)>& cb);

    void setGenerate(int proclimit, const std::function<void(QJsonValue)>& cb);
    void stopGenerate(int proclimit, const std::function<void(QJsonValue)>& cb);
    void getZPrivKey(QString addr, const std::function<void(QJsonValue)>& cb);
    void getZViewKey(QString addr, const std::function<void(QJsonValue)>& cb);
    void getTPrivKey(QString addr, const std::function<void(QJsonValue)>& cb);
    void importZPrivKey(QString addr, bool rescan, const std::function<void(QJsonValue)>& cb);
    void importTPrivKey(QString addr, bool rescan, const std::function<void(QJsonValue)>& cb);
    void validateAddress(QString address, const std::function<void(QJsonValue)>& cb);
    void getBlock(QString height, const std::function<void(QJsonValue)>& cb);

    void shutdownHushd();
    void noConnection();
    bool isEmbedded() { return ehushd != nullptr; }

    QString getDefaultSaplingAddress();
    QString getDefaultTAddress();

    void getAllPrivKeys(const std::function<void(QList<QPair<QString, QString>>)>);

    Connection* getConnection() { return conn; }

    void rescan(qint64 height, const std::function<void(QJsonValue)>& cb);
    void getRescanInfo(const std::function<void(QJsonValue)>& cb);
    void help(const std::function<void(QJsonValue)>& cb);
    void getnetworksolps(const std::function<void(QJsonValue)>& cb);
    void getlocalsolps(const std::function<void(QJsonValue)>& cb);

private:
    void refreshBalances();

    void refreshTransactions();    
    void refreshSentZTrans();
    void refreshReceivedZTrans(QList<QString> zaddresses);

    bool processUnspent     (const QJsonValue& reply, QMap<QString, double>* newBalances, QList<UnspentOutput>* newUtxos);
    void updateUI           (bool anyUnconfirmed);

    void getInfoThenRefresh(bool force);

    void getBalance(const std::function<void(QJsonValue)>& cb);
    QJsonValue makePayload(QString method, QString param, QString param2);
    QJsonValue makePayload(QString method, QString param);
    QJsonValue makePayload(QString method, int param);
    QJsonValue makePayload(QString method);

    void getTransparentUnspent  (const std::function<void(QJsonValue)>& cb);
    void getZUnspent            (const std::function<void(QJsonValue)>& cb);
    void getTransactions        (const std::function<void(QJsonValue)>& cb);
    void listBanned             (const std::function<void(QJsonValue)>& cb);
    void getPeerInfo            (const std::function<void(QJsonValue)>& cb);
    void getZAddresses          (const std::function<void(QJsonValue)>& cb);
    void getTAddresses          (const std::function<void(QJsonValue)>& cb);
    void z_sweepstatus          (const std::function<void(QJsonValue)>& cb);
    void z_consolidationstatus  (const std::function<void(QJsonValue)>& cb);

    Connection*                 conn                        = nullptr;
    std::shared_ptr<QProcess>   ehushd                      = nullptr;
    QList<UnspentOutput>*       utxos                       = nullptr;
    QMap<QString, double>*      allBalances                 = nullptr;
    QMap<QString, bool>*        usedAddresses               = nullptr;
    QList<QString>*             zaddresses                  = nullptr;
    QList<QString>*             taddresses                  = nullptr;
    
    QMap<QString, WatchedTx>    watchingOps;

    TxTableModel*               transactionsTableModel      = nullptr;
    PeersTableModel*            peersTableModel             = nullptr;
    BannedPeersTableModel*      bannedPeersTableModel       = nullptr;
    BalancesTableModel*         balancesTableModel          = nullptr;

    QTimer*                     timer;
    QTimer*                     txTimer;
    QTimer*                     priceTimer;
    QTimer*                     rescanTimer;

    Ui::MainWindow*             ui;
    MainWindow*                 main;

    // Current balance in the UI. If this number updates, then refresh the UI
    QString                     currentBalance;
};

#endif // RPCCLIENT_H
