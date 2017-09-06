#ifndef EXCHANGEPAGE_H
#define EXCHANGEPAGE_H

#include <QDialog>
#include "walletmodel.h"

#include <QWidget>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDebug>
#include <QByteArray>
#include <QByteRef>

#define LITECOIN_API 55e1-2713-20a4-6c02
#define DOGECOIN_API 60f4-1337-2957-6dd8

namespace Ui {
class exchangepage;
}

class WalletModel;

class exchangepage : public QDialog
{
    Q_OBJECT
    
public:
    explicit exchangepage(QWidget *parent = 0);
    ~exchangepage();

    //void setModel(WalletModel *walletM);

    QNetworkAccessManager *network1;
    QNetworkAccessManager *network2;
    QNetworkAccessManager *network3;

    QString b;


    
private slots:
    void on_comboBox_activated(int index);

    void on_select_recieve_comboBox_activated(const QString &arg1);


    void litecoinbalance(QNetworkReply *pReply);

private:
    Ui::exchangepage *ui;
};

#endif // EXCHANGEPAGE_H
