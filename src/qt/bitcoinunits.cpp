#include "bitcoinunits.h"

#include <QStringList>
#include <QDebug>

BitcoinUnits::BitcoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<BitcoinUnits::Unit> BitcoinUnits::availableUnits()
{
    QList<BitcoinUnits::Unit> unitlist;
    unitlist.append(BCH);
    //unitlist.append(mTIT);
    //unitlist.append(uTIT);
    unitlist.append(BTC);
    return unitlist;
}

bool BitcoinUnits::valid(int unit)
{
    switch(unit)
    {
    case BTC:
    case BCH:
        return true;
    default:
        return false;
    }
}

QString BitcoinUnits::name(int unit)
{
    switch(unit)
    {
    case BCH: return QString("BCH");
    //case mTIT: return QString("mTIT");
    //case uTIT: return QString::fromUtf8("μTIT");
    case BTC: return QString("BTC");
    default: return QString("???");
    }
}

QString BitcoinUnits::description(int unit)
{
    switch(unit)
    {
    case BCH: return QString("Bitcoins Cash");
    //case mTIT: return QString("Milli-Titcoins (1 / 1,000)");
    //case uTIT: return QString("Micro-Titcoins (1 / 1,000,000)");
    case BTC: return QString("Bitcoins");
    default: return QString("???");
    }
}

qint64 BitcoinUnits::factor(int unit)
{
    switch(unit)
    {
    case BCH:  return 100000000;
    //case mTIT: return 100000;
    //case uTIT: return 100;
    case BTC: return 1;
    default:   return 100000000;
    }
}

int BitcoinUnits::amountDigits(int unit)
{
    switch(unit)
    {
    case BCH: return 8; // 21,000,000 (# digits, without commas)
    //case mTIT: return 11; // 21,000,000,000
    //case uTIT: return 14; // 21,000,000,000,000
    case BTC: return 8;
    default: return 0;
    }
}

int BitcoinUnits::decimals(int unit)
{
    switch(unit)
    {
    case BCH: return 8;
    //case mTIT: return 5;
    //case uTIT: return 2;
    case BTC: return 8;
    default: return 0;
    }
}

QString BitcoinUnits::format(int unit, qint64 n, bool fPlus)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit)){ qDebug()<<"Refuse to format invalid unit";
        return QString();} // Refuse to format invalid unit
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    // Right-trim excess zeros after the decimal point
    int nTrim = 0;
    for (int i = remainder_str.size()-1; i>=2 && (remainder_str.at(i) == '0'); --i)
        ++nTrim;
    remainder_str.chop(nTrim);

    if (n < 0){ qDebug()<<" n < 0";
        quotient_str.insert(0, '-');}
    else if (fPlus && n > 0){ qDebug()<<" n > 0 ssafsf";
        quotient_str.insert(0, '+');}
    return quotient_str + QString(".") + remainder_str;
}

QString BitcoinUnits::formatWithUnit(int unit, qint64 amount, bool plussign)
{
    //qDebug()<<"Amount "<<(double)amount;
    return format(unit, amount, plussign) + QString(" ") + name(unit);
}

bool BitcoinUnits::parse(int unit, const QString &value, qint64 *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = decimals(unit);
    QStringList parts = value.split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    qint64 retvalue = str.toLongLong(&ok);
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

int BitcoinUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant BitcoinUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(name(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}
