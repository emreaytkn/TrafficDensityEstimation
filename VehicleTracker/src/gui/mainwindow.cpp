#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "csvhandler.h"

#include <QFileDialog>
#include <QDirIterator>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {

    ui->setupUi(this);

    this->_player_thread = new QThread();
    this->_player = new Player();
    this->_player->moveToThread(this->_player_thread);

    this->total_Cars = 0;
    this->total_SUVs = 0;
    this->total_LargeTrucks = 0;
    this->total_Buses = 0;
    this->total_Motorbikes = 0;

    this->ui->lineDataset->setReadOnly(true);
    this->ui->lineResults->setReadOnly(true);

    qDebug() << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "."
             << CV_SUBMINOR_VERSION;

    dialog = new Dialog();

    connect(this->_player, SIGNAL(workRequested()), this->_player_thread, SLOT(start()));
    connect(this->_player_thread, SIGNAL(started()), this->_player, SLOT(doWork()));
    //connect(this->_player, SIGNAL(frameReady(cv::Mat)), this, SLOT(onFrameReady(cv::Mat)));



    connect(this->_player, SIGNAL(trackRemoved(unsigned long, QString)), this, SLOT(onTrackRemoved(unsigned long, QString)));

    connect(this->ui->btnLoadDataset, SIGNAL(clicked()), this, SLOT(onLoadDatasetBtnClicked()));
    connect(this->ui->btnLoadResults, SIGNAL(clicked()), this, SLOT(onLoadResultsBtnClicked()));
    connect(this->ui->btnStartStopTracking, SIGNAL(clicked()), this, SLOT(onStartStopTrackingBtnClicked()));
    connect(this->ui->btnShowResults, SIGNAL(clicked()), this, SLOT(onShowResultsBtnClicked()));
    connect(this->ui->checkShowBoxes, SIGNAL(toggled(bool)), this, SLOT(onShowBoxesCheckToggled(bool)));
    connect(this->ui->checkShowCategory, SIGNAL(toggled(bool)), this, SLOT(onShowCategoryCheckToggled(bool)));
    connect(this->ui->checkShowId, SIGNAL(toggled(bool)), this, SLOT(onShowIdCheckToggled(bool)));
    connect(this->ui->sliderDelay, SIGNAL(valueChanged(int)), this, SLOT(onDelaySliderValueChanged(int)));
    connect(this->ui->btnConfidenceThreshold, SIGNAL(clicked()), this, SLOT(onConfidenceThresholdBtnClicked()));

}

void MainWindow::onLoadDatasetBtnClicked() {

    QString dataset_dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                            "/home/eaytekin/Desktop/CsvResults-First7000/input",
                                                            QFileDialog::ShowDirsOnly);
    if(!dataset_dir.isEmpty()) {

        qDebug() << "Selected dataset directory is: " << dataset_dir;
        this->ui->lineDataset->setText(dataset_dir);

        QDirIterator it(dataset_dir,
                        QStringList() << "*.jpg" << "*.jpeg" << "*.png",
                        QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext()) {

            QString outputFrameFileName = it.next();
            _frame_list.append(outputFrameFileName);
        }
        std::sort(_frame_list.begin(), _frame_list.end());
        qDebug() << "FrameList size: " << _frame_list.size();
    }
}

void MainWindow::onLoadResultsBtnClicked() {

    QString result_dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                           "/home/eaytekin/Desktop/CsvResults-First7000/results",
                                                           QFileDialog::ShowDirsOnly);
    if(!result_dir.isEmpty()) {

        qDebug() << "Selected input directory is: " << result_dir;
        this->ui->lineResults->setText(result_dir);

        QDirIterator it(result_dir,
                        QStringList() << "*.csv" << "*.xml" << "*.json",
                        QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext()) {

            QString resultFileName = it.next();
            _result_list.append(resultFileName);
        }
        std::sort(_result_list.begin(), _result_list.end());
        qDebug() << "ResultList size: " << _result_list.size();
    }
}

void MainWindow::onStartStopTrackingBtnClicked() {

    if(this->ui->btnStartStopTracking->text() == "Start Tracking") {

        // Read first frame
        cv::Mat frame = cv::imread(QString(this->_frame_list.at(0)).toStdString());

        this->_bounding_box = selectROI("Select Roi", frame, false, false);
        //quit if ROI was not selected
        if(this->_bounding_box.width==0 || this->_bounding_box.height==0)
            return;

        bool useLocalTracking = false;
        tracking::DistType distance_type;
        tracking::KalmanType kalman_type;
        tracking::FilterGoal filter_goal;
        tracking::LostTrackType lost_track_type;
        tracking::MatchType match_type;
        float dt;
        float accel_noise_mag;
        float distance_threshold;
        int max_skip_frame;
        int max_trace_lenght;

        if(this->ui->checkUseLocalTraining->isChecked()) {

            useLocalTracking = true;
        }

        switch (this->ui->cmbDistanceType->currentIndex()) {
        case 0:
            distance_type = tracking::DistCenters;
            break;
        case 1:
            distance_type = tracking::DistRects;
            break;
        case 2:
            distance_type = tracking::DistJaccard;
            break;
        default:
            qDebug() << "CmbDistaceType wrong index!!";
            break;
        }

        switch (this->ui->cmbKalmanType->currentIndex()) {
        case 0:
            kalman_type = tracking::KalmanLinear;
            break;
        case 1:
            kalman_type = tracking::KalmanUnscented;
            break;
        case 2:
            kalman_type = tracking::KalmanAugmentedUnscented;
            break;
        default:
            qDebug() << "cmbKalmanType wrong index!!";
            break;
        }

        switch (this->ui->cmbFilterGoal->currentIndex()) {
        case 0:
            filter_goal = tracking::FilterCenter;
            break;
        case 1:
            filter_goal = tracking::FilterRect;
            break;
        default:
            qDebug() << "cmbFilterGoal wrong index!!";
            break;
        }

        switch (this->ui->cmbLostTrackType->currentIndex()) {
        case 0:
            lost_track_type = tracking::TrackNone;
            break;
        case 1:
            lost_track_type = tracking::TrackKCF;
            break;
        case 2:
            lost_track_type = tracking::TrackMIL;
            break;
        case 3:
            lost_track_type = tracking::TrackMedianFlow;
            break;
        default:
            qDebug() << "cmbLostTrackType wrong index!!";
            break;
        }

        switch (this->ui->cmbMatchTrackType->currentIndex()) {
        case 0:
            match_type = tracking::MatchHungrian;
            break;
        case 1:
            match_type = tracking::MatchBipart;
            break;
        default:
            qDebug() << "cmbMatchTrackType wrong index!!";
            break;
        }

        dt = this->ui->dspinDt->value();
        accel_noise_mag = this->ui->dspinAccelNoiseMag->value();
        distance_threshold = this->ui->dspinDistThreshold->value();
        max_skip_frame = this->ui->spinMaxAllowedSkip->value();
        max_trace_lenght = this->ui->spinMaxTraceLength->value();

        TrackerSettings tracker_settings(useLocalTracking,
                                         distance_type,             //Dist Type
                                         kalman_type,               //Kalman Type
                                         filter_goal,               //Filter Goal
                                         lost_track_type,           //LostTrackType
                                         match_type,                //MatchType
                                         dt,                        //dt
                                         accel_noise_mag,           //accelNoiseMag
                                         distance_threshold,        //dist_thresh 90
                                         max_skip_frame,            //max allow skip frame
                                         max_trace_lenght);         //max trace length

        this->_player->setShowAllBBoxStatus(this->ui->checkShowBoxes->isChecked());
        this->_player->setShowCategoryStatus(this->ui->checkShowCategory->isChecked());
        this->_player->setShowTrackerIdStatus(this->ui->checkShowId->isChecked());

        // To avoid having two threads running simultaneously, the previous thread is aborted.
        this->_player->init(tracker_settings, this->_frame_list, this->_result_list, this->_bounding_box,
                            this->ui->dspinConfidenceThreshold->value(), this->ui->sliderDelay->value());
        this->_player->abort();
        this->_player_thread->wait(); // If the thread is not running, this will immediately return.
        this->_player->requestWork();

        this->ui->btnStartStopTracking->setText("Stop Tracking");
    }
    else if(this->ui->btnStartStopTracking->text() == "Stop Tracking") {

        this->_player->abort();
        this->_player_thread->wait();

        this->ui->btnStartStopTracking->setText("Start Tracking");
    }
}

void MainWindow::onTrackRemoved(unsigned long id, QString category) {

    qDebug() << "Removing ----> ID: " << id << " Class: " << category;

    //cv::Point center(rect.x + rect.width/2, rect.y+ rect.height/2);

    //if(!this->bounding_box.contains(center)) {

        if(category == "Car") {
            this->total_Cars++;
        }
        else if(category == "SUV"
                || category == "Van") {
            this->total_SUVs++;
        }
        else if(category == "Bus") {
            this->total_Buses++;
        }
        else if(category == "Motorbike") {
            this->total_Motorbikes++;
        }
        else if(category == "LargeTruck"
                || category == "MediumTruck" || category == "SmallTruck" ) {
            this->total_LargeTrucks++;
        }

        QVector<double> current_result;
        current_result << this->total_Cars << this->total_SUVs << this->total_LargeTrucks
                       << this->total_Motorbikes << this->total_Buses;
        this->dialog->updateResults(current_result, (this->total_Cars+this->total_SUVs+this->total_LargeTrucks
                                                     +this->total_Motorbikes+this->total_Buses));
        current_result.clear();
    //}
}

void MainWindow::onShowResultsBtnClicked() {

    this->dialog->displayResults();
}

void MainWindow::onShowBoxesCheckToggled(bool checked) {

    this->_player->setShowAllBBoxStatus(checked);
}

void MainWindow::onShowCategoryCheckToggled(bool checked) {

    this->_player->setShowCategoryStatus(checked);

}

void MainWindow::onShowIdCheckToggled(bool checked) {

    this->_player->setShowTrackerIdStatus(checked);
}

void MainWindow::onDelaySliderValueChanged(int delay) {

    this->_player->setDelay(delay);
}

void MainWindow::onConfidenceThresholdBtnClicked() {

    this->_player->setConfidenceThreshold(this->ui->dspinConfidenceThreshold->value());
}

MainWindow::~MainWindow() {

    this->_player->abort();
    this->_player_thread->wait();

    qDebug()<<"Deleting thread and worker in Thread "<<this->QObject::thread()->currentThreadId();
    delete this->_player;
    delete this->_player_thread;

    delete ui;
}
