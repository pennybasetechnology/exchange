#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QDialog>

namespace Ui {
class homepage;
}

class homepage : public QDialog
{
    Q_OBJECT
    
public:
    explicit homepage(QWidget *parent = 0);
    ~homepage();
    
private:
    Ui::homepage *ui;
};

#endif // HOMEPAGE_H
