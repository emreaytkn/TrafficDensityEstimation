#include "player.h"

#include "csvhandler.h"

#include <QThread>
#include <QDebug>
#include <QTimer>
#include <QEventLoop>

using namespace cv;

Player::Player(QObject *parent)
    : QObject(parent) {

    this->_is_working = false;
    this->_is_aborted = false;

    this->_confidence_threshold = 0.5;
    this->_is_show_all_bbox = false;
    this->_is_show_category = false;
    this->_is_show_tracker_id = false;

    colors = { cv::Scalar(255, 0, 0), cv::Scalar(0, 255, 0), cv::Scalar(0, 0, 255), cv::Scalar(255, 255, 0),
               cv::Scalar(0, 255, 255), cv::Scalar(255, 0, 255), cv::Scalar(255, 127, 255),
               cv::Scalar(127, 0, 255), cv::Scalar(127, 0, 127) };

    connect(&this->tracker, SIGNAL(trackRemoved(unsigned long, QString)), this, SIGNAL(trackRemoved(unsigned long, QString)));

}

void Player::init(TrackerSettings settings, const QStringList& frame_list, const QStringList& result_list,
                  const cv::Rect& bbox, double confidence_threshold, int delay) {

    this->_frame_list = frame_list;
    this->_result_list = result_list;
    this->_bounding_box = bbox;
    this->_confidence_threshold = confidence_threshold;
    this->_delay = delay;

    tracker.init(settings.useLocalTracking,
                 settings.distance_type,             //Dist Type
                 settings.kalman_type,               //Kalman Type
                 settings.filter_goal,               //Filter Goal
                 settings.lost_track_type,           //LostTrackType
                 settings.match_type,                //MatchType
                 settings.dt,                        //dt
                 settings.accel_noise_mag,           //accelNoiseMag
                 settings.distance_threshold,        //dist_thresh 90
                 settings.max_skip_frames,            //max allow skip frame
                 settings.max_trace_length);         //max trace length
}

void Player::requestWork() {

    this->_mutex.lock();
    this->_is_working = true;
    this->_is_aborted = false;

    qDebug() << "Request worker start in Thread "
             <<thread()->currentThreadId();

    this->_mutex.unlock();

    emit workRequested();
}

void Player::doWork() {

    qDebug() << "Starting worker process in Thread "
             << thread()->currentThreadId();

    for(int i = 0; i < _result_list.size(); i++) {

        // Checks if the process should be aborted
        this->_mutex.lock();
        bool abort = this->_is_aborted;
        this->_mutex.unlock();

        if (abort) {
            qDebug()<<"Aborting worker process in Thread "
                   << thread()->currentThreadId();
            break;
        }

        detected_t current_detections;

        cv::Mat frame = cv::imread(QString(_frame_list.at(i)).toStdString());
        cv::imshow("Tracker", frame);

        io::CSVReader<6> in(QString(_result_list.at(i)).toStdString());
        in.read_header(io::ignore_extra_column, "x", "y", "w", "h", "category", "confidence");
        std::string category;
        double x, y, w, h, confidence;
        while(in.read_row(x, y, w, h, category, confidence)) {

            if(confidence > _confidence_threshold) {

                cv::Point center = cv::Point(x+w/2, y+h/2);
                if(this->_bounding_box.contains(center)) {

                    cv::circle(frame, center, 1, cv::Scalar(0,0,170), 7, cv::LINE_8, 0);
                    cv::Rect bbox = cv::Rect(x,y,w,h);

                    if(this->_is_show_category) {

                        QString attributes = QString::number(confidence) + " -- " + QString::fromStdString(category);
                        cv::putText(frame, attributes.toStdString(), cv::Point(bbox.x, bbox.y), CV_FONT_HERSHEY_PLAIN, 1, cv::Scalar(255,255,255), 2);
                    }
                    cv::rectangle(frame, bbox, cv::Scalar(0, 255, 0), 1, CV_AA);

                    current_detections.push_back(CDetected(bbox, category));
                }

                else {

                    if(this->_is_show_all_bbox) {

                        cv::Rect bbox = cv::Rect(x,y,w,h);
                        if(this->_is_show_category) {

                            QString attributes = QString::number(confidence) + " -- " + QString::fromStdString(category);
                            cv::putText(frame, attributes.toStdString(), cv::Point(bbox.x, bbox.y), CV_FONT_HERSHEY_PLAIN, 1, cv::Scalar(255,255,255), 2);
                        }
                        cv::rectangle(frame, bbox, cv::Scalar(0, 0, 255), 1, CV_AA);
                    }
                }
            }
        }

        regions_t regions;
        for(size_t i = 0; i < current_detections.size(); ++i) {

            regions.push_back(CRegion(current_detections[i].bbox, current_detections[i].category));
        }

        tracker.update(regions, cv::Mat());

        std::cout << tracker.tracks.size() << std::endl;

        for (size_t i = 0; i < tracker.tracks.size(); i++) {

            const auto& track = tracker.tracks[i];

            if (track->m_trace.size() > 1) {

                for (size_t j = 0; j < track->m_trace.size() - 1; j++) {

                    cv::line(frame, track->m_trace[j], track->m_trace[j + 1], colors[i % colors.size()], 2, CV_AA);
                }
            }
            cv::putText(frame, QString::number(track->m_trackID).toStdString(),track->m_trace[i], 2,3, cv::Scalar(255, 0, 0));

            //qDebug() << "ID: " << track->m_trackID << " Category: " << QString::fromStdString(track->m_lastRegion.m_category);
            //total_cars = track->m_trackID;
        }

        cv::rectangle(frame, this->_bounding_box , cv::Scalar(0,0,255), 2, 1);
        cv::imshow("Tracker", frame);

        //emit frameReady(current_frame);

        // This will stupidly wait 1 sec doing nothing...
        QEventLoop loop;
        QTimer::singleShot(_delay, &loop, SLOT(quit()));
        loop.exec();
    }
    // Set _working to false, meaning the process can't be aborted anymore.
    this->_mutex.lock();
    this->_is_working = false;
    this->_mutex.unlock();

    qDebug() << "Worker process finished in Thread "
             << thread()->currentThreadId();

    emit finished();
}

void Player::abort() {

    this->_mutex.lock();
    if(this->_is_working) {

        this->_is_aborted = true;
        qDebug() << "Request worker aborting in Thread "
                 << thread()->currentThreadId();
    }
    this->_mutex.unlock();
}

void Player::setConfidenceThreshold(double threshold) {

    qDebug() << "Confidence Threshold Value is set to : " << QString::number(threshold);
    this->_confidence_threshold = threshold;
}

void Player::setDelay(int delay) {

    qDebug() << "Delay is set to : " << delay << " ms";
    this->_delay = delay;
}

void Player::setShowAllBBoxStatus(bool enabled) {

    this->_is_show_all_bbox = enabled;
}

void Player::setShowCategoryStatus(bool enabled) {

    this->_is_show_category = enabled;
}

void Player::setShowTrackerIdStatus(bool enabled) {

    this->_is_show_tracker_id = enabled;
}
