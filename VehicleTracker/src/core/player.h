#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QMutex>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "Ctracker.h"

struct CDetected {

    cv::Rect bbox;
    std::string category;

    CDetected(cv::Rect r, std::string c)
        : bbox(r), category(c) {}
};
typedef std::vector<CDetected> detected_t;


class Player : public QObject
{
    Q_OBJECT
public:
    explicit Player(QObject *parent = nullptr);

    void init(TrackerSettings, const QStringList&, const QStringList&, const cv::Rect&, double, int);
    void requestWork();
    void abort();

    std::vector<cv::Scalar> colors;

    void setDelay(int);
    void setConfidenceThreshold(double);
    void setShowAllBBoxStatus(bool);
    void setShowCategoryStatus(bool);
    void setShowTrackerIdStatus(bool);

signals:
    void frameReady(cv::Mat);
    void trackRemoved(unsigned long, QString);
    void workRequested();
    void finished();

public slots:
    void doWork();

private:
    QMutex _mutex;
    QStringList _frame_list;
    QStringList _result_list;
    bool _is_working;
    bool _is_aborted;
    int _delay;
    double _confidence_threshold;

    CTracker tracker;

    cv::Rect2d _bounding_box;

    bool _is_show_all_bbox;
    bool _is_show_category;
    bool _is_show_tracker_id;
};

#endif // PLAYER_H
