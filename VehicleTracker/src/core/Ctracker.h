#pragma once
#include <QObject>
#include <iostream>
#include <vector>
#include <memory>
#include <array>

#include "Tracker/defines.h"
#include "Tracker/track.h"
#include "Tracker/LocalTracker.h"

using namespace tracking;

class CTracker : public QObject
{
    Q_OBJECT
public:
    CTracker(QObject *parent = 0);
	~CTracker(void);

    void init(bool, DistType, KalmanType, FilterGoal, LostTrackType, MatchType,
              track_t, track_t, track_t, size_t , size_t);

    tracks_t tracks;
    void update(const regions_t& regions, cv::Mat grayFrame);

    bool GrayFrameToTrack() const
    {
        return m_lostTrackType != tracking::LostTrackType::TrackGOTURN;
    }

signals:
    void trackRemoved(unsigned long, QString);

private:
    // Use local tracking for regions between two frames
    bool m_useLocalTracking;

    tracking::DistType m_distType;
    tracking::KalmanType m_kalmanType;
    tracking::FilterGoal m_filterGoal;
    tracking::LostTrackType m_lostTrackType;
    tracking::MatchType m_matchType;

    // Filter time interval
	track_t dt;

	track_t accelNoiseMag;

    // Distance threshold. If the points are arcs from a friend at a distance,
    // exceeding this threshold, then this pair is not considered in the assignment problem.
	track_t dist_thres;
    // The maximum number of frames that a track is saved without receiving measurement data.
    size_t maximum_allowed_skipped_frames;

    size_t max_trace_length;

	size_t NextTrackID;

    LocalTracker m_localTracker;

    cv::Mat m_prevFrame;
};
