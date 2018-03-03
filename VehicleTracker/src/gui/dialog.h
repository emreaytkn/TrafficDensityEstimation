#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include "qcustomplot.h"
namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    void displayResults();
    void updateResults(const QVector<double>&, int);


private:
    Ui::Dialog *ui;

    QVector<double> ticks;
    QCPBars *detected;
    QVector<double> fossilData, nuclearData, detectedData;
};

#endif // DIALOG_H
