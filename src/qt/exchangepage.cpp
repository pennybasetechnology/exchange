#include "exchangepage.h"
#include "ui_exchangepage.h"
#include <QDebug>
#include <parser.h>

exchangepage::exchangepage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::exchangepage)
{
    ui->setupUi(this);


}

exchangepage::~exchangepage()
{
    delete ui;
}

void exchangepage::on_comboBox_activated(int index)
{
    if(index==0)
    {
        ui->unit_available_amoun_label->setText("   ");
        ui->unit_exchange_amount_label->setText("   ");

        ui->select_recieve_comboBox->clear();
        ui->select_recieve_comboBox->addItem("Select");
        ui->select_recieve_comboBox->addItem("Bitcoin");
        ui->select_recieve_comboBox->addItem("Litecoin");
        ui->select_recieve_comboBox->addItem("Dogecoin");

        ui->available_amount_exchange_label->setText("");

    }
    else if(index==1)
    {
        ui->unit_available_amoun_label->setText("BTC");
        ui->unit_exchange_amount_label->setText("BTC");

        ui->select_recieve_comboBox->clear();
        ui->select_recieve_comboBox->addItem("Select");
        ui->select_recieve_comboBox->addItem("Litecoin");
        ui->select_recieve_comboBox->addItem("Dogecoin");

        ui->available_amount_exchange_label->setText("0.0000000");



    }
    else if(index==2)
    {
        ui->unit_available_amoun_label->setText("LTC");
        ui->unit_exchange_amount_label->setText("LTC");

        ui->select_recieve_comboBox->clear();
        ui->select_recieve_comboBox->addItem("Select");
        ui->select_recieve_comboBox->addItem("Bitcoin");
        ui->select_recieve_comboBox->addItem("Dogecoin");


        network1 = new QNetworkAccessManager(this);
        network1->get(QNetworkRequest(QUrl("https://block.io/api/v2/get_balance/?api_key=55e1-2713-20a4-6c02")));

        connect(network1, SIGNAL(finished(QNetworkReply*)),
                this, SLOT(litecoinbalance(QNetworkReply*)));




        qDebug()<<"qwerty"<<b;

        ui->available_amount_exchange_label->setText(b);

    }
    else if(index==3)
    {
        ui->unit_available_amoun_label->setText("DGC");
        ui->unit_exchange_amount_label->setText("DGC");

        ui->select_recieve_comboBox->clear();
        ui->select_recieve_comboBox->addItem("Select");
        ui->select_recieve_comboBox->addItem("Bitcoin");
        ui->select_recieve_comboBox->addItem("Litecoin");

        ui->available_amount_exchange_label->setText("0.0000000");

    }
}

void exchangepage::on_select_recieve_comboBox_activated(const QString &arg1)
{
    if(arg1=="Select")
    {
        ui->unit_available_amount_recieve_label->setText("   ");
        ui->unit_recieve_amount_label->setText("   ");

    }
    else if(arg1=="Bitcoin")
    {
        ui->unit_available_amount_recieve_label->setText("BTC");
        ui->unit_recieve_amount_label->setText("BTC");

    }
    else if(arg1=="Litecoin")
    {
        ui->unit_available_amount_recieve_label->setText("LTC");
        ui->unit_recieve_amount_label->setText("LTC");

    }
    else if(arg1=="Dogecoin")
    {
        ui->unit_available_amount_recieve_label->setText("DGC");
        ui->unit_recieve_amount_label->setText("DGC");

    }
}

void exchangepage::litecoinbalance(QNetworkReply *pReply)
{
    QByteArray data=pReply->readAll();

    qDebug()<<data;
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
    b = a.toString();

    qDebug()<<"litecoin"<<b;
    //ui->balancelabel->setText(b);
}
