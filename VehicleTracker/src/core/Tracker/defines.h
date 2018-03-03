#pragma once
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>





// ---------------------------------------------------------------------------
typedef float track_t;
typedef cv::Point_<track_t> Point_t;
#define Mat_t CV_32FC

///
/// \brief The CRegion class
///
class CRegion
{
public:
    CRegion()
        : m_type(""), m_confidence(-1)
    {
    }

    CRegion(const cv::Rect& rect, const std::string& category)
        : m_rect(rect), m_category(category) { }

    CRegion(const cv::Rect& rect, const std::string& type, float confidence)
        : m_rect(rect), m_type(type), m_confidence(confidence)
    {

    }

    cv::Rect m_rect;
    std::vector<cv::Point2f> m_points;

    std::string m_category;
    std::string m_type;
    float m_confidence;
};

typedef std::vector<CRegion> regions_t;

///
///
///
namespace tracking
{
///
enum Detectors
{
    Motion_VIBE,
    Motion_MOG,
    Motion_GMG,
    Motion_CNT,
    Motion_SuBSENSE,
    Motion_LOBSTER,
    Motion_MOG2,
    Face_HAAR,
    Pedestrian_HOG,
    Pedestrian_C4,
    DNN
};

///
/// \brief The DistType enum
///
enum DistType
{
    DistCenters = 0,
    DistRects = 1,
    DistJaccard = 2
};

///
/// \brief The FilterGoal enum
///
enum FilterGoal
{
    FilterCenter = 0,
    FilterRect = 1
};

///
/// \brief The KalmanType enum
///
enum KalmanType
{
    KalmanLinear = 0,
    KalmanUnscented = 1,
    KalmanAugmentedUnscented
};

///
/// \brief The MatchType enum
///
enum MatchType
{
    MatchHungrian = 0,
    MatchBipart = 1
};

///
/// \brief The LostTrackType enum
///
enum LostTrackType
{
    TrackNone = 0,
    TrackKCF = 1,
    TrackMIL,
    TrackMedianFlow,
    TrackGOTURN,
    TrackMOSSE
};

struct TrackerSettings
{
    bool useLocalTracking;
    DistType distance_type;
    KalmanType kalman_type;
    FilterGoal filter_goal;
    LostTrackType lost_track_type;
    MatchType match_type;
    float dt;
    float accel_noise_mag;
    float distance_threshold;
    int max_skip_frames;
    int max_trace_length;

    TrackerSettings(bool localTrackingEnabled, DistType distanceType,
                    KalmanType kalmanType, FilterGoal filterGoal,
                    LostTrackType lostTrackType, MatchType matchType,
                    float dt, float accelNoiseMag, float distanceThreshold,
                    int maxSkipFrames, int maxTraceLength)
        : useLocalTracking(localTrackingEnabled), distance_type(distanceType),
          kalman_type(kalmanType), filter_goal(filterGoal), lost_track_type(lostTrackType),
          match_type(matchType), dt(dt), accel_noise_mag(accelNoiseMag),
          distance_threshold(distanceThreshold), max_skip_frames(maxSkipFrames),
          max_trace_length(maxTraceLength)
    {}
};
}
