#include "track.h"
#define USE_OCV_KCF
///
/// \brief CTrack
/// \param pt
/// \param region
/// \param deltaTime
/// \param accelNoiseMag
/// \param trackID
/// \param filterObjectSize
/// \param externalTrackerForLost
///
CTrack::CTrack(
        const CRegion& region,
        tracking::KalmanType kalmanType,
        track_t deltaTime,
        track_t accelNoiseMag,
        size_t trackID,
        bool filterObjectSize,
        tracking::LostTrackType externalTrackerForLost
        )
    :
      m_trackID(trackID),
      m_skippedFrames(0),
      m_lastRegion(region),
      m_predictionPoint((region.m_rect.tl() + region.m_rect.br()) / 2),
      m_filterObjectSize(filterObjectSize),
      m_externalTrackerForLost(externalTrackerForLost)
{
    if (filterObjectSize)
    {
        m_kalman = new TKalmanFilter(kalmanType, region.m_rect, deltaTime, accelNoiseMag);
    }
    else
    {
        m_kalman = new TKalmanFilter(kalmanType, m_predictionPoint, deltaTime, accelNoiseMag);
    }
    m_trace.push_back(m_predictionPoint, m_predictionPoint);
}

///
/// \brief CalcDist
/// \param pt
/// \return
///
track_t CTrack::CalcDist(const Point_t& pt) const
{
    Point_t diff = m_predictionPoint - pt;
    return sqrtf(diff.x * diff.x + diff.y * diff.y);
}

///
/// \brief CalcDist
/// \param r
/// \return
///
track_t CTrack::CalcDist(const cv::Rect& r) const {

    std::array<track_t, 4> diff;
    diff[0] = m_predictionPoint.x - m_lastRegion.m_rect.width / 2.f - r.x;
    diff[1] = m_predictionPoint.y - m_lastRegion.m_rect.height / 2.f - r.y;
    diff[2] = static_cast<track_t>(m_lastRegion.m_rect.width - r.width);
    diff[3] = static_cast<track_t>(m_lastRegion.m_rect.height - r.height);

    track_t dist = 0;
    for (size_t i = 0; i < diff.size(); ++i) {

        dist += diff[i] * diff[i];
    }
    return sqrtf(dist);
}

///
/// \brief CalcOverlap
/// \param r
/// \return
///
track_t CTrack::CalcDistJaccard(const cv::Rect& r) const
{
    cv::Rect rr(GetLastRect());

    track_t intArea = (r & rr).area();
    track_t unionArea = r.area() + rr.area() - intArea;

    return 1 - intArea / unionArea;
}

///
/// \brief CTrack::CheckType
/// \param type
/// \return
///
bool CTrack::CheckType(const std::string& type) const
{
    return m_lastRegion.m_type.empty() || type.empty() || (m_lastRegion.m_type == type);
}

///
/// \brief Update
/// \param pt
/// \param region
/// \param dataCorrect
/// \param max_trace_length
/// \param prevFrame
/// \param currFrame
///
void CTrack::Update(
        const CRegion& region,
        bool dataCorrect,
        size_t max_trace_length,
        cv::Mat prevFrame,
        cv::Mat currFrame
        )
{
    cv::Point pt((region.m_rect.tl() + region.m_rect.br()) / 2);

    if (m_filterObjectSize) // Kalman filter for object coordinates and size
    {
        RectUpdate(region, dataCorrect, prevFrame, currFrame);
    }
    else // Kalman filter only for object center
    {
        PointUpdate(pt, dataCorrect, currFrame.size());
    }

    if (dataCorrect)
    {
        m_lastRegion = region;
        m_trace.push_back(m_predictionPoint, pt);
    }
    else
    {
        m_trace.push_back(m_predictionPoint);
    }

    if (m_trace.size() > max_trace_length)
    {
        m_trace.pop_front(m_trace.size() - max_trace_length);
    }
}

///
/// \brief IsRobust
/// \param minTraceSize
/// \param minRawRatio
/// \param sizeRatio
/// \return
///
bool CTrack::IsRobust(int minTraceSize, float minRawRatio, cv::Size2f sizeRatio) const
{
    bool res = m_trace.size() > static_cast<size_t>(minTraceSize);
    res &= m_trace.GetRawCount(m_trace.size() - 1) / static_cast<float>(m_trace.size()) > minRawRatio;
    if (sizeRatio.width + sizeRatio.height > 0)
    {
        float sr = m_lastRegion.m_rect.width / static_cast<float>(m_lastRegion.m_rect.height);
        if (sizeRatio.width > 0)
        {
            res &= (sr > sizeRatio.width);
        }
        if (sizeRatio.height > 0)
        {
            res &= (sr < sizeRatio.height);
        }
    }
    return res;
}

///
/// \brief GetLastRect
/// \return
///
cv::Rect CTrack::GetLastRect() const
{
    if (m_filterObjectSize)
    {
        return m_predictionRect;
    }
    else
    {
        return cv::Rect(
                    static_cast<int>(m_predictionPoint.x - m_lastRegion.m_rect.width / 2),
                    static_cast<int>(m_predictionPoint.y - m_lastRegion.m_rect.height / 2),
                    m_lastRegion.m_rect.width,
                    m_lastRegion.m_rect.height);
    }
}

///
/// \brief RectUpdate
/// \param region
/// \param dataCorrect
/// \param prevFrame
/// \param currFrame
///
void CTrack::RectUpdate(
        const CRegion& region,
        bool dataCorrect,
        cv::Mat prevFrame,
        cv::Mat currFrame
        )
{
    m_kalman->GetRectPrediction();

    bool recalcPrediction = true;

    auto Clamp = [](int& v, int& size, int hi)
    {
        if (size < 2)
        {
            size = 2;
        }
        if (v < 0)
        {
            v = 0;
        }
        else if (v + size > hi - 1)
        {
            v = hi - 1 - size;
        }
    };

    switch (m_externalTrackerForLost)
    {
    case tracking::TrackNone:
        break;

    case tracking::TrackKCF:
    case tracking::TrackMIL:
    case tracking::TrackMedianFlow:
    case tracking::TrackGOTURN:
    case tracking::TrackMOSSE:
#ifdef USE_OCV_KCF
        if (!dataCorrect)
        {
            cv::Size roiSize(std::max(2 * m_predictionRect.width, currFrame.cols / 4), std::min(2 * m_predictionRect.height, currFrame.rows / 4));
            if (roiSize.width > currFrame.cols)
            {
                roiSize.width = currFrame.cols;
            }
            if (roiSize.height > currFrame.rows)
            {
                roiSize.height = currFrame.rows;
            }
            cv::Point roiTL(m_predictionRect.x + m_predictionRect.width / 2 - roiSize.width / 2, m_predictionRect.y + m_predictionRect.height / 2 - roiSize.height / 2);
            cv::Rect roiRect(roiTL, roiSize);
            Clamp(roiRect.x, roiRect.width, currFrame.cols);
            Clamp(roiRect.y, roiRect.height, currFrame.rows);

            if (!m_tracker || m_tracker.empty())
            {
                CreateExternalTracker();

                cv::Rect2d lastRect(m_predictionRect.x - roiRect.x, m_predictionRect.y - roiRect.y, m_predictionRect.width, m_predictionRect.height);
                if (lastRect.x >= 0 &&
                        lastRect.y >= 0 &&
                        lastRect.x + lastRect.width < roiRect.width &&
                        lastRect.y + lastRect.height < roiRect.height &&
                        lastRect.area() > 0)
                {
                    m_tracker->init(cv::Mat(prevFrame, roiRect), lastRect);
                }
                else
                {
                    m_tracker.release();
                }
            }
            cv::Rect2d newRect;
            if (!m_tracker.empty() && m_tracker->update(cv::Mat(currFrame, roiRect), newRect))
            {
                cv::Rect prect(cvRound(newRect.x) + roiRect.x, cvRound(newRect.y) + roiRect.y, cvRound(newRect.width), cvRound(newRect.height));

                m_predictionRect = m_kalman->Update(prect, true);

                recalcPrediction = false;

                m_boundidgRect = cv::Rect();
                m_lastRegion.m_points.clear();
            }
        }
        else
        {
            if (m_tracker && !m_tracker.empty())
            {
                m_tracker.release();
            }
        }
#else
        std::cerr << "KCF tracker was disabled in CMAKE! Set lostTrackType = TrackNone in constructor." << std::endl;
#endif
        break;
    }

    if (recalcPrediction)
    {
        if (m_boundidgRect.area() > 0)
        {
            if (dataCorrect)
            {
                cv::Rect prect(
                            (m_boundidgRect.x + region.m_rect.x) / 2,
                            (m_boundidgRect.y + region.m_rect.y) / 2,
                            (m_boundidgRect.width + region.m_rect.width) / 2,
                            (m_boundidgRect.height + region.m_rect.height) / 2);

                m_predictionRect = m_kalman->Update(prect, dataCorrect);
            }
            else
            {
                cv::Rect prect(
                            (m_boundidgRect.x + m_predictionRect.x) / 2,
                            (m_boundidgRect.y + m_predictionRect.y) / 2,
                            (m_boundidgRect.width + m_predictionRect.width) / 2,
                            (m_boundidgRect.height + m_predictionRect.height) / 2);

                m_predictionRect = m_kalman->Update(prect, true);
            }
        }
        else
        {
            m_predictionRect = m_kalman->Update(region.m_rect, dataCorrect);
        }
    }

    Clamp(m_predictionRect.x, m_predictionRect.width, currFrame.cols);
    Clamp(m_predictionRect.y, m_predictionRect.height, currFrame.rows);

    m_predictionPoint = (m_predictionRect.tl() + m_predictionRect.br()) / 2;
}

///
/// \brief CreateExternalTracker
///
void CTrack::CreateExternalTracker()
{
    switch (m_externalTrackerForLost)
    {
    case tracking::TrackNone:
        break;

    case tracking::TrackKCF:
#ifdef USE_OCV_KCF
        if (!m_tracker || m_tracker.empty())
        {
            cv::TrackerKCF::Params params;
            params.compressed_size = 1;
            params.desc_pca = cv::TrackerKCF::GRAY;
            params.desc_npca = cv::TrackerKCF::GRAY;
            params.resize = true;
            //params.detect_thresh = 0.3;
#if (((CV_VERSION_MAJOR == 3) && (CV_VERSION_MINOR >= 3)) || (CV_VERSION_MAJOR > 3))
            m_tracker = cv::TrackerKCF::create(params);
#else
            m_tracker = cv::TrackerKCF::createTracker(params);
#endif
        }
#endif
        break;

    case tracking::TrackMIL:
#ifdef USE_OCV_KCF
        if (!m_tracker || m_tracker.empty())
        {
            cv::TrackerMIL::Params params;

#if (((CV_VERSION_MAJOR == 3) && (CV_VERSION_MINOR >= 3)) || (CV_VERSION_MAJOR > 3))
            m_tracker = cv::TrackerMIL::create(params);
#else
            m_tracker = cv::TrackerMIL::createTracker(params);
#endif
        }
#endif
        break;

    case tracking::TrackMedianFlow:
#ifdef USE_OCV_KCF
        if (!m_tracker || m_tracker.empty())
        {
            cv::TrackerMedianFlow::Params params;

#if (((CV_VERSION_MAJOR == 3) && (CV_VERSION_MINOR >= 3)) || (CV_VERSION_MAJOR > 3))
            m_tracker = cv::TrackerMedianFlow::create(params);
#else
            m_tracker = cv::TrackerMedianFlow::createTracker(params);
#endif
        }
#endif
        break;

    case tracking::TrackGOTURN:
#ifdef USE_OCV_KCF
        if (!m_tracker || m_tracker.empty())
        {
            cv::TrackerGOTURN::Params params;

#if (((CV_VERSION_MAJOR == 3) && (CV_VERSION_MINOR >= 3)) || (CV_VERSION_MAJOR > 3))
            m_tracker = cv::TrackerGOTURN::create(params);
#else
            //m_tracker = cv::TrackerGOTURN::createTracker(params);
#endif
        }
#endif
        break;

    case tracking::TrackMOSSE:
#ifdef USE_OCV_KCF
        if (!m_tracker || m_tracker.empty())
        {
#if (((CV_VERSION_MAJOR == 3) && (CV_VERSION_MINOR >= 3)) || (CV_VERSION_MAJOR > 3))
            m_tracker = cv::TrackerMOSSE::create();
#else
            //m_tracker = cv::TrackerMOSSE::createTracker();
#endif
        }
#endif
        break;
    }
}

///
/// \brief PointUpdate
/// \param pt
/// \param dataCorrect
///
void CTrack::PointUpdate(
        const Point_t& pt,
        bool dataCorrect,
        const cv::Size& frameSize
        )
{
    m_kalman->GetPointPrediction();

    if (m_averagePoint.x + m_averagePoint.y > 0)
    {
        if (dataCorrect)
        {
            m_predictionPoint = m_kalman->Update((pt + m_averagePoint) / 2, dataCorrect);
        }
        else
        {
            m_predictionPoint = m_kalman->Update((m_predictionPoint + m_averagePoint) / 2, true);
        }
    }
    else
    {
        m_predictionPoint = m_kalman->Update(pt, dataCorrect);
    }

    auto Clamp = [](float& v, int hi)
    {
        if (v < 0)
        {
            v = 0;
        }
        else if (hi && v > hi - 1)
        {
            v = hi - 1;
        }
    };
    Clamp(m_predictionPoint.x, frameSize.width);
    Clamp(m_predictionPoint.y, frameSize.height);
}
