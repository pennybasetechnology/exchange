#include "overviewpage.h"
#include "ui_overviewpage.h"
#include "iostream"

#include "clientmodel.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QDebug>
#include <json_spirit_reader.h>

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <string.h>

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "boost/foreach.hpp"

#include "parser.h"
#include "qobjecthelper.h"




#define DECORATION_SIZE 64
#define NUM_ITEMS 3

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::BTC)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);


    manager = new QNetworkAccessManager(this);
    manager1 = new QNetworkAccessManager(this);
    manager2 = new QNetworkAccessManager(this);
    manager3 = new QNetworkAccessManager(this);
    manager->get(QNetworkRequest(QUrl("https://block.io/api/v2/get_balance/?api_key=55e1-2713-20a4-6c02")));

    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(litecoinbalance(QNetworkReply*)));

    manager1->get(QNetworkRequest(QUrl("https://block.io//api/v2/get_transactions/?api_key=55e1-2713-20a4-6c02&type=sent")));

    connect(manager1, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(litecointransaction(QNetworkReply*)));



    manager2->get(QNetworkRequest(QUrl("https://block.io//api/v2/get_balance/?api_key=60f4-1337-2957-6dd8")));

    connect(manager2, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(dogecoinbalance(QNetworkReply*)));


    manager3->get(QNetworkRequest(QUrl("https://block.io//api/v2/get_transactions/?api_key=60f4-1337-2957-6dd8&type=received")));

    connect(manager3, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(dogecointransaction(QNetworkReply*)));


}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("TIT")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

//void OverviewPage::a()
//{
//    if(ui->tab_2->isActiveWindow())
//    {

//    }
//}





void OverviewPage::litecoinbalance(QNetworkReply *pReply)
{
    QByteArray data=pReply->readAll();

    QJson::Parser parser;
    bool ok;
    QVariantMap result = parser.parse (data, &ok).toMap();
    if (!ok) {
        qFatal("An error occured during parsing");
        exit (1);
    }
    QVariantMap nestedMap = result["data"].toMap();
    qDebug() << "available_balance:" << nestedMap["available_balance"].toString();
    QVariant a = nestedMap["available_balance"];

    QVariant pend = nestedMap["pending_received_balance"];

    QString b = a.toString();

    QString c = pend.toString();
    ui->balancelabel->setText(b);
    ui->litecoin_uncon_balance_label->setText(c);
}

void OverviewPage::litecointransaction(QNetworkReply *pReply)
{
    QByteArray data=pReply->readAll();

//    QJson::Parser parser;
//    bool ok;
//    QVariantMap result = parser.parse (data, &ok).toMap();
//    if (!ok) {
//        qFatal("An error occured during parsing");
//        exit (1);
//    }
//    QVariantMap nestedMap = result["data"].toMap();
//    qDebug() << "available_balance:" << nestedMap["available_balance"].toString();
//    QVariant a = nestedMap["available_balance"];
//    QString b = a.toString();
//    ui->balancelabel->setText(b);
//    ui->textEdit->setText(str);

    qDebug()<<data;


}


void OverviewPage::dogecoinbalance(QNetworkReply *pReply)
{
    QByteArray data=pReply->readAll();

    QJson::Parser parser;
    bool ok;
    QVariantMap result = parser.parse (data, &ok).toMap();
    if (!ok) {
        qFatal("An error occured during parsing");
        exit (1);
    }
    QVariantMap nestedMap = result["data"].toMap();
    qDebug() << "available_balance:" << nestedMap["available_balance"].toString();
    QVariant a = nestedMap["available_balance"];
    QString b = a.toString();
    ui->dogecoinbalancelabel->setText(b);


    QVariant pend = nestedMap["pending_received_balance"];


    QString c = pend.toString();
    ui->dogecoin_uncon_balance_label->setText(c);
}

void OverviewPage::dogecointransaction(QNetworkReply *pReply)
{
    QByteArray data=pReply->readAll();


//    "{
//      "status" : "success",
//      "data" : {
//        "network" : "DOGETEST",
//        "txs" : [
//          {
//            "txid" : "d719fc87b3ef5b9512f1a70ad0be423ba0b542ce84a0a6d27421474b116f0937",
//            "from_green_address" : false,
//            "time" : 1504616980,
//            "confirmations" : 0,
//            "amounts_received" : [
//              {
//                "recipient" : "2N8KmKQwLncNgzQ2JgLi4gNNvPkXNYzwsj4",
//                "amount" : "1000.00000000"
//              }
//            ],
//            "senders" : [
//              "neseJWuAu633oFShK1MCE7LJovRSzVnLXw"
//            ],
//            "confidence" : 0.0,
//            "propagated_by_nodes" : 10
//          },
//          {
//            "txid" : "bc933afe1f32e5626c29b6680a3dd07cca094eaedfe180e022b19397009097a0",
//            "from_green_address" : false,
//            "time" : 1504611750,
//            "confirmations" : 0,
//            "amounts_received" : [
//              {
//                "recipient" : "2N8KmKQwLncNgzQ2JgLi4gNNvPkXNYzwsj4",
//                "amount" : "1000.00000000"
//              }
//            ],
//            "senders" : [
//              "nYqKAYjEt7t16Hazr8EAMpt5Tu9jVadVwx"
//            ],
//            "confidence" : 0.0,
//            "propagated_by_nodes" : 10
//          }
//        ]
//      }
//    }"




//    QJson::Parser parser;
//    bool ok;
//    QVariantMap result = parser.parse (data, &ok).toMap();
//    if (!ok) {
//        qFatal("An error occured during parsing");
//        exit (1);
//    }
//    QVariantMap nestedMap = result["data"].toMap();
//    qDebug() << "available_balance:" << nestedMap["available_balance"].toString();
//    QVariant a = nestedMap["available_balance"];
//    QString b = a.toString();
//    ui->balancelabel->setText(b);
//    ui->textEdit->setText(str);

    qDebug()<<data;


}
