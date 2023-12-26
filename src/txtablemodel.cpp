// Copyright 2019-2022 The Hush developers
// Released under the GPLv3
#include "txtablemodel.h"
#include "settings.h"
#include "rpc.h"
#include "guiconstants.h"

extern bool iszdeex;

TxTableModel::TxTableModel(QObject *parent)
     : QAbstractTableModel(parent) {
    headers << QObject::tr("Type") << QObject::tr("Address") << QObject::tr("Date/Time") << QObject::tr("Amount");
}

TxTableModel::~TxTableModel() {
    delete modeldata;
    delete tTrans;
    delete zsTrans;
    delete zrTrans;
}

void TxTableModel::addZSentData(const QList<TransactionItem>& data) {
    delete zsTrans;
    zsTrans = new QList<TransactionItem>();
    std::copy(data.begin(), data.end(), std::back_inserter(*zsTrans));

    updateAllData();
}

void TxTableModel::addZRecvData(const QList<TransactionItem>& data) {
    delete zrTrans;
    zrTrans = new QList<TransactionItem>();
    std::copy(data.begin(), data.end(), std::back_inserter(*zrTrans));

    updateAllData();
}


void TxTableModel::addTData(const QList<TransactionItem>& data) {
    delete tTrans;
    tTrans = new QList<TransactionItem>();
    std::copy(data.begin(), data.end(), std::back_inserter(*tTrans));

    updateAllData();
}

bool TxTableModel::exportToCsv(QString fileName) const {
    if (!modeldata)
        return false;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return false;

    QTextStream out(&file);   // we will serialize the data into the file

    // Write headers
    for (int i = 0; i < headers.length(); i++) {
        out << "\"" << headers[i] << "\",";
    }
    out << "\"Memo\"";
    out << endl;
    
    // Write out each row
    for (int row = 0; row < modeldata->length(); row++) {
        for (int col = 0; col < headers.length(); col++) {
            out << "\"" << data(index(row, col), Qt::DisplayRole).toString() << "\",";
        }
        // Memo
        out << "\"" << modeldata->at(row).memo << "\"";
        out << endl;
    }

    file.close();
    return true;
}

void TxTableModel::updateAllData() {
    auto newmodeldata = new QList<TransactionItem>();

    if (tTrans  != nullptr) std::copy( tTrans->begin(),  tTrans->end(), std::back_inserter(*newmodeldata));
    if (zsTrans != nullptr) std::copy(zsTrans->begin(), zsTrans->end(), std::back_inserter(*newmodeldata));
    if (zrTrans != nullptr) std::copy(zrTrans->begin(), zrTrans->end(), std::back_inserter(*newmodeldata));

    // Sort by reverse time
    std::sort(newmodeldata->begin(), newmodeldata->end(), [=] (auto a, auto b) {
        return a.datetime > b.datetime; // reverse sort
    });

    // And then swap out the modeldata with the new one.
    delete modeldata;
    modeldata = newmodeldata;

    dataChanged(index(0, 0), index(modeldata->size()-1, columnCount(index(0,0))-1));
    layoutChanged();
}


QImage TxTableModel::colorizeIcon(QIcon icon, QColor color) const{
    QImage img(icon.pixmap(16, 16).toImage());
    img = img.convertToFormat(QImage::Format_ARGB32);
    for (int x = img.width(); x--; )
    {
        for (int y = img.height(); y--; )
        {
            const QRgb rgb = img.pixel(x, y);
            img.setPixel(x, y, qRgba(color.red(), color.green(), color.blue(), qAlpha(rgb)));
        }
    }
    return img;
}


int TxTableModel::rowCount(const QModelIndex&) const
{
    if (modeldata == nullptr) return 0;
    return modeldata->size();
}

int TxTableModel::columnCount(const QModelIndex&) const
{
    return headers.size();
}


 QVariant TxTableModel::data(const QModelIndex &index, int role) const
 {
    // Get current theme name
    QString theme_name = Settings::getInstance()->get_theme_name();
    QBrush b;
    QColor color;
    if (theme_name == "dark" || theme_name == "midnight") {
        color = COLOR_WHITE;
    }else if(theme_name == "zdeex"){
        color = COLOR_ZDEEX_TEXT;
    }else{
        color = COLOR_BLACK;
    }

    // Align column 4 (amount) right
    if (role == Qt::TextAlignmentRole && index.column() == 3) return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    
    if (role == Qt::ForegroundRole) {
        if (modeldata->at(index.row()).confirmations == 0) {
            b.setColor(COLOR_UNCONFIRMED_TX);
            return b;
        }
        if (theme_name == "dark" || theme_name == "midnight") {
            b.setColor(color);
            return b;
        }else{
            b.setColor(color);
            return b;
        }
        return b;        
    }

    auto dat = modeldata->at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return dat.type;
        case 1: { 
                    auto addr = modeldata->at(index.row()).address;
                    if (addr.trimmed().isEmpty()) 
                        return "(Shielded)";
                    else 
                        return addr;
                }
        case 2: return QDateTime::fromMSecsSinceEpoch(modeldata->at(index.row()).datetime *  (qint64)1000).toLocalTime().toString();
        case 3: return Settings::getDisplayFormat(modeldata->at(index.row()).amount);
        }
    } 

    if (role == Qt::ToolTipRole) {
        switch (index.column()) {
        case 0: { 
                    if (dat.memo.startsWith("hush:")) {
                        return Settings::paymentURIPretty(Settings::parseURI(dat.memo));
                    } else {
                        return modeldata->at(index.row()).type + 
                        (dat.memo.isEmpty() ? "" : " tx memo: \"" + dat.memo.toHtmlEscaped() + "\"");
                    }
                }
        case 1: { 
                    auto addr = modeldata->at(index.row()).address;
                    if (addr.trimmed().isEmpty()) 
                        return "(Shielded)";
                    else 
                        return addr;
                }
        case 2: return QDateTime::fromMSecsSinceEpoch(modeldata->at(index.row()).datetime * (qint64)1000).toLocalTime().toString();
        case 3: return Settings::getInstance()->getUSDFormat(modeldata->at(index.row()).amount);
        }    
    }

    if (role == Qt::DecorationRole && index.column() == 0) {

        //qDebug() << "TX Type = " + dat.type;

        if (!dat.memo.isEmpty()) {
            // If the memo is a Payment URI, then show a payment request icon
            if(iszdeex) {
                if (dat.memo.startsWith("zdeex:")) {
                    QIcon icon(":/icons/paymentreq.gif");
                    return QVariant(icon.pixmap(16, 16));
                }
            } else if (dat.memo.startsWith("hush:")) {
                QIcon icon(":/icons/paymentreq.gif");
                return QVariant(icon.pixmap(16, 16));
            }

            // Return the info pixmap to indicate memo
            QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);            
            return QVariant(icon.pixmap(16, 16));
        } else {
            // TODO: Add appropriate icons for types of txs instead of empty pixmap
            //qDebug() << "Type = " +getType(index.row()) + "Address = " +getAddr(index.row()) + "From Address = " +getFromAddr(index.row());

            // Send
            if(this->getType(index.row()) == "send"){
                QImage image = colorizeIcon(QIcon(":/icons/tx_output.png"), color);
                QIcon icon;
                icon.addPixmap(QPixmap::fromImage(image));
                return QVariant(icon.pixmap(16, 16));

            }

            // Send T->Z - Untested
            if(this->getType(index.row()) == "send" && !this->getFromAddr(index.row()).startsWith("zs1")){                
                QImage image = colorizeIcon(QIcon(":/icons/lock_closed.png"), color);
                QIcon icon;
                icon.addPixmap(QPixmap::fromImage(image));
                return QVariant(icon.pixmap(16, 16));
            }

            // Receive
            if(this->getType(index.row()) == "receive"){
                QImage image = colorizeIcon(QIcon(":/icons/tx_input.png"), color);
                QIcon icon;
                icon.addPixmap(QPixmap::fromImage(image));
                return QVariant(icon.pixmap(16, 16));
            }

            // Mined
            if(this->getType(index.row()) == "generate"){
                QImage image = colorizeIcon(QIcon(":/icons/tx_mined.png"), color);
                QIcon icon;
                icon.addPixmap(QPixmap::fromImage(image));
                return QVariant(icon.pixmap(16, 16));
            }

            // Empty pixmap to make it align (old behavior)
            QPixmap p(16, 16);
            p.fill(Qt::white);
            return QVariant(p);
        }
    }

    return QVariant();
 }


 QVariant TxTableModel::headerData(int section, Qt::Orientation orientation, int role) const
 {
     if (role == Qt::TextAlignmentRole && section == 3) return QVariant(Qt::AlignRight | Qt::AlignVCenter);

     if (role == Qt::FontRole) {
         QFont f;
         f.setBold(true);
         return f;
     }

     if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
         return headers.at(section);
     }

     return QVariant();
 }

QString TxTableModel::getTxId(int row) const {
    return modeldata->at(row).txid;
}

QString TxTableModel::getMemo(int row) const {
    return modeldata->at(row).memo;
}

qint64 TxTableModel::getConfirmations(int row) const {
    return modeldata->at(row).confirmations;
}

QString TxTableModel::getAddr(int row) const {
    return modeldata->at(row).address.trimmed();
}

QString TxTableModel::getFromAddr(int row) const {
    return modeldata->at(row).fromAddr.trimmed();
}

qint64 TxTableModel::getDate(int row) const {
    return modeldata->at(row).datetime;
}

QString TxTableModel::getType(int row) const {
    return modeldata->at(row).type;
}

QString TxTableModel::getAmt(int row) const {
    return Settings::getDecimalString(modeldata->at(row).amount);
}
