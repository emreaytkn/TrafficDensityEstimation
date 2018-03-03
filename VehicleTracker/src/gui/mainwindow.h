#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <opencv2/opencv.hpp>
#include "dialog.h"

#include "Ctracker.h"
#include "player.h"
namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private Q_SLOTS:
    void onLoadDatasetBtnClicked();
    void onLoadResultsBtnClicked();
    void onStartStopTrackingBtnClicked();
    void onShowResultsBtnClicked();
    void onDelaySliderValueChanged(int);
    void onConfidenceThresholdBtnClicked();
    void onShowBoxesCheckToggled(bool);
    void onShowCategoryCheckToggled(bool);
    void onShowIdCheckToggled(bool);
    void onTrackRemoved(unsigned long, QString);

private:
    Ui::MainWindow *ui;
    //CTracker tracker;
    QStringList _frame_list;
    QStringList _result_list;

    Player *_player;
    QThread *_player_thread;

    Dialog *dialog;
    cv::Rect _bounding_box;

    int total_Cars;
    int total_SUVs;
    int total_LargeTrucks;
    int total_Buses;
    int total_Motorbikes;
};

#endif // MAINWINDOW_H
