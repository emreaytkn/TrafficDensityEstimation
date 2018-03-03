#include "Ctracker.h"
#include "Tracker/HungarianAlg/HungarianAlg.h"

#include <QDebug>
// ---------------------------------------------------------------------------
// Tracker. Manage tracks. Create, remove, update.
// ---------------------------------------------------------------------------
CTracker::CTracker(QObject *parent)
    : QObject(parent), NextTrackID(0) {

}

CTracker::~CTracker(void) {

}

void CTracker::init(bool useLocalTracking, tracking::DistType distType, tracking::KalmanType kalmanType, tracking::FilterGoal filterGoal,
                    tracking::LostTrackType lostTrackType, tracking::MatchType matchType, track_t dt_, track_t accelNoiseMag_, track_t dist_thres_,
                    size_t maximum_allowed_skipped_frames_, size_t max_trace_length_) {

    this->m_useLocalTracking = useLocalTracking;
    this->m_distType = distType;
    this->m_kalmanType = kalmanType;
    this->m_filterGoal = filterGoal;
    this->m_lostTrackType = lostTrackType;
    this->m_matchType = matchType;
    this->dt = dt_;
    this->accelNoiseMag = accelNoiseMag_;
    this->dist_thres = dist_thres_;
    this->maximum_allowed_skipped_frames = maximum_allowed_skipped_frames_;
    this->max_trace_length = max_trace_length_;
}

void CTracker::update(const regions_t& regions, cv::Mat grayFrame) {

    if (m_prevFrame.size() == grayFrame.size()) {

        if (m_useLocalTracking) {

            m_localTracker.Update(tracks, m_prevFrame, grayFrame);
        }
    }

    // -----------------------------------
    // If there is no tracks yet, then every cv::Point begins its own track.
    // -----------------------------------
    if (tracks.size() == 0) {

        // If no tracks yet
        for (size_t i = 0; i < regions.size(); ++i) {

            tracks.push_back(std::make_unique<CTrack>(regions[i],
                                                      m_kalmanType,
                                                      dt,
                                                      accelNoiseMag,
                                                      NextTrackID++,
                                                      m_filterGoal == tracking::FilterRect,
                                                      m_lostTrackType));
        }
    }

    size_t N = tracks.size();		// Tracks
    size_t M = regions.size();	// Detects

    assignments_t assignment(N, -1); // Assigned ones

    if (!tracks.empty()) {

        // The matrix of distances from the N-th track to the M-th detective.
        distMatrix_t Cost(N * M);

        // -----------------------------------
        // We already have tracks, we will compose a distance matrix
        // -----------------------------------
        const track_t maxPossibleCost = grayFrame.cols * grayFrame.rows;
        track_t maxCost = 0;
        switch (m_distType) {

        case tracking::DistCenters:
            for (size_t i = 0; i < tracks.size(); i++) {

                for (size_t j = 0; j < regions.size(); j++) {

                    auto dist = tracks[i]->CheckType(regions[j].m_type) ?
                                tracks[i]->CalcDist((regions[j].m_rect.tl() + regions[j].m_rect.br()) / 2) : maxPossibleCost;
					Cost[i + j * N] = dist;
                    if (dist > maxCost) {

						maxCost = dist;
					}
                }
            }
            break;

        case tracking::DistRects:
            for (size_t i = 0; i < tracks.size(); i++)
            {
                for (size_t j = 0; j < regions.size(); j++)
                {
                    auto dist = tracks[i]->CheckType(regions[j].m_type) ? tracks[i]->CalcDist(regions[j].m_rect) : maxPossibleCost;
					Cost[i + j * N] = dist;
					if (dist > maxCost)
					{
						maxCost = dist;
					}
                }
            }
            break;

        case tracking::DistJaccard:
            for (size_t i = 0; i < tracks.size(); i++)
            {
                for (size_t j = 0; j < regions.size(); j++)
                {
                    auto dist = tracks[i]->CheckType(regions[j].m_type) ? tracks[i]->CalcDistJaccard(regions[j].m_rect) : 1;
                    Cost[i + j * N] = dist;
                    if (dist > maxCost)
                    {
                        maxCost = dist;
                    }
                }
            }
            break;
        }
        // -----------------------------------
        // Solving assignment problem (tracks and predictions of Kalman filter)
        // -----------------------------------
        if (m_matchType == tracking::MatchHungrian)
		{
			AssignmentProblemSolver APS;
			APS.Solve(Cost, N, M, assignment, AssignmentProblemSolver::optimal);
        }
		// -----------------------------------
		// clean assignment from pairs with large distance
		// -----------------------------------
		for (size_t i = 0; i < assignment.size(); i++)
		{
			if (assignment[i] != -1)
			{
				if (Cost[i + assignment[i] * N] > dist_thres)
				{
					assignment[i] = -1;
                    tracks[i]->m_skippedFrames++;
				}
			}
			else
			{
				// If track have no assigned detect, then increment skipped frames counter.
                tracks[i]->m_skippedFrames++;
			}
		}

		// -----------------------------------
        // If track didn't get detects long time, remove it.
        // -----------------------------------
        for (int i = 0; i < static_cast<int>(tracks.size()); i++)
        {
            if (tracks[i]->m_skippedFrames > maximum_allowed_skipped_frames)
            {
                //qDebug() << "Removing ----> ID: " << tracks[i]->m_trackID
                //         << " Class: " << QString::fromStdString(tracks[i]->m_lastRegion.m_category);

                emit trackRemoved(tracks[i]->m_trackID, QString::fromStdString(tracks[i]->m_lastRegion.m_category));
                tracks.erase(tracks.begin() + i);
                assignment.erase(assignment.begin() + i);
                i--;
            }
        }
    }

    // -----------------------------------
    // Search for unassigned detects and start new tracks for them.
    // -----------------------------------
    for (size_t i = 0; i < regions.size(); ++i)
    {
        if (find(assignment.begin(), assignment.end(), i) == assignment.end())
        {
            tracks.push_back(std::make_unique<CTrack>(regions[i],
                                                      m_kalmanType,
                                                      dt,
                                                      accelNoiseMag,
                                                      NextTrackID++,
                                                      m_filterGoal == tracking::FilterRect,
                                                      m_lostTrackType));
        }
    }

    // Update Kalman Filters state

    for (size_t i = 0; i < assignment.size(); i++)
    {
        // If track updated less than one time, than filter state is not correct.

        if (assignment[i] != -1) // If we have assigned detect, then update using its coordinates,
        {
            tracks[i]->m_skippedFrames = 0;
            tracks[i]->Update(regions[assignment[i]], true, max_trace_length, m_prevFrame, grayFrame);
        }
        else				     // if not continue using predictions
        {
            tracks[i]->Update(CRegion(), false, max_trace_length, m_prevFrame, grayFrame);
        }
    }

    grayFrame.copyTo(m_prevFrame);
}
