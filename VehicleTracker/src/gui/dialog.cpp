#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent), ui(new Ui::Dialog) {

    ui->setupUi(this);

    // set dark background gradient:
    QLinearGradient gradient(0, 0, 0, 400);
    gradient.setColorAt(0, QColor(90, 90, 90));
    gradient.setColorAt(0.38, QColor(105, 105, 105));
    gradient.setColorAt(1, QColor(70, 70, 70));
    this->ui->customPlot->setBackground(QBrush(gradient));

    // create empty bar chart objects:
    detected = new QCPBars(this->ui->customPlot->xAxis, this->ui->customPlot->yAxis);
    detected->setAntialiased(false); // gives more crisp, pixel aligned bar borders

    // set names and colors:
    detected->setName("Detected");
    detected->setPen(QPen(QColor(0, 168, 140).lighter(130)));
    detected->setBrush(QColor(0, 168, 140));

    // prepare x axis with class labels:
    QVector<QString> labels;
    ticks << 1 << 2 << 3 << 4 << 5;
    labels << "Car" << "SUV" << "Truck" << "Motorbike" << "Bus" ;

    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    textTicker->addTicks(ticks, labels);
    this->ui->customPlot->xAxis->setTicker(textTicker);
    this->ui->customPlot->xAxis->setTickLabelRotation(60);
    this->ui->customPlot->xAxis->setSubTicks(false);
    this->ui->customPlot->xAxis->setTickLength(0, 4);
    this->ui->customPlot->xAxis->setRange(0, 8);
    this->ui->customPlot->xAxis->setBasePen(QPen(Qt::white));
    this->ui->customPlot->xAxis->setTickPen(QPen(Qt::white));
    this->ui->customPlot->xAxis->grid()->setVisible(true);
    this->ui->customPlot->xAxis->grid()->setPen(QPen(QColor(130, 130, 130), 0, Qt::DotLine));
    this->ui->customPlot->xAxis->setTickLabelColor(Qt::white);
    this->ui->customPlot->xAxis->setLabelColor(Qt::white);

    // prepare y axis:
    this->ui->customPlot->yAxis->setRange(0, 100);
    this->ui->customPlot->yAxis->setPadding(5); // a bit more space to the left border
    this->ui->customPlot->yAxis->setLabel("Total Number Detected");
    this->ui->customPlot->yAxis->setBasePen(QPen(Qt::white));
    this->ui->customPlot->yAxis->setTickPen(QPen(Qt::white));
    this->ui->customPlot->yAxis->setSubTickPen(QPen(Qt::white));
    this->ui->customPlot->yAxis->grid()->setSubGridVisible(true);
    this->ui->customPlot->yAxis->setTickLabelColor(Qt::white);
    this->ui->customPlot->yAxis->setLabelColor(Qt::white);
    this->ui->customPlot->yAxis->grid()->setPen(QPen(QColor(130, 130, 130), 0, Qt::SolidLine));
    this->ui->customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(130, 130, 130), 0, Qt::DotLine));

    // Add data:
    detectedData   << 0 << 0 << 0 << 0 << 0;
    detected->setData(ticks, detectedData);

    // setup legend:
    this->ui->customPlot->legend->setVisible(true);
    this->ui->customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignHCenter);
    this->ui->customPlot->legend->setBrush(QColor(255, 255, 255, 100));
    this->ui->customPlot->legend->setBorderPen(Qt::NoPen);
    QFont legendFont = font();
    legendFont.setPointSize(10);
    this->ui->customPlot->legend->setFont(legendFont);
    this->ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void Dialog::displayResults() {

    this->show();
}

void Dialog::updateResults(const QVector<double>& current_result, int frame_num) {

    detected->setData(ticks, current_result);

    this->ui->lblFrameNum->setText(QString::number(frame_num));

    this->ui->customPlot->replot();
    this->repaint();
}

Dialog::~Dialog() {

    delete ui;
}
