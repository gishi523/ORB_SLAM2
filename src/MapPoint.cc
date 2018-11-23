/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/

#include "MapPoint.h"

#include "Frame.h"
#include "KeyFrame.h"
#include "Map.h"
#include "ORBmatcher.h"

#define LOCK_MUTEX_POINT_CREATION() std::unique_lock<std::mutex> lock1(map_->mutexPointCreation);
#define LOCK_MUTEX_POSITION()       std::unique_lock<std::mutex> lock2(mutexPos_);
#define LOCK_MUTEX_FEATURES()       std::unique_lock<std::mutex> lock3(mutexFeatures_);
#define LOCK_MUTEX_GLOBAL()         std::unique_lock<std::mutex> lock3(globalMutex_);

namespace ORB_SLAM2
{

MapPoint::mappointid_t MapPoint::nextId = 0;
std::mutex MapPoint::globalMutex_;

MapPoint::MapPoint(const cv::Mat& Xw, KeyFrame* referenceKF, Map* map) :
	firstKFid(referenceKF->id), firstFrame(referenceKF->frameId), nobservations_(0), trackReferenceForFrame(0),
	lastFrameSeen(0), BALocalForKF(0), fuseCandidateForKF(0), loopPointForKF(0), correctedByKF(0),
	correctedReference(0), BAGlobalForKF(0), referenceKF_(referenceKF), nvisible_(1), nfound_(1), bad_(false),
	replaced_(nullptr), minDistance_(0), maxDistance_(0), map_(map)
{
	Xw.copyTo(Xw_);
	normal_ = cv::Mat::zeros(3, 1, CV_32F);

	// MapPoints can be created from Tracking and Local Mapping. This mutex avoid conflicts with id.
	LOCK_MUTEX_POINT_CREATION();
	id = nextId++;
}

MapPoint::MapPoint(const cv::Mat& Xw, Map* map, Frame* frame, int idx) :
	firstKFid(-1), firstFrame(frame->id), nobservations_(0), trackReferenceForFrame(0), lastFrameSeen(0),
	BALocalForKF(0), fuseCandidateForKF(0), loopPointForKF(0), correctedByKF(0),
	correctedReference(0), BAGlobalForKF(0), referenceKF_(nullptr), nvisible_(1),
	nfound_(1), bad_(false), replaced_(nullptr), map_(map)
{
	Xw.copyTo(Xw_);
	cv::Mat Ow = frame->GetCameraCenter();
	normal_ = Xw_ - Ow;
	normal_ = normal_ / cv::norm(normal_);

	const float dist = static_cast<float>(cv::norm(Xw - Ow));
	const int level = frame->keypointsUn[idx].octave;
	const float scaleFactor = frame->pyramid.scaleFactors[level];
	
	maxDistance_ = scaleFactor * dist;
	minDistance_ = maxDistance_ / frame->pyramid.scaleFactors.back();

	frame->descriptorsL.row(idx).copyTo(descriptor_);

	// MapPoints can be created from Tracking and Local Mapping. This mutex avoid conflicts with id.
	LOCK_MUTEX_POINT_CREATION();
	id = nextId++;
}

void MapPoint::SetWorldPos(const cv::Mat& Xw)
{
	LOCK_MUTEX_GLOBAL();
	LOCK_MUTEX_POSITION();
	Xw.copyTo(Xw_);
}

cv::Mat MapPoint::GetWorldPos()
{
	LOCK_MUTEX_POSITION();
	return Xw_.clone();
}

cv::Mat MapPoint::GetNormal()
{
	LOCK_MUTEX_POSITION();
	return normal_.clone();
}

KeyFrame* MapPoint::GetReferenceKeyFrame()
{
	LOCK_MUTEX_FEATURES();
	return referenceKF_;
}

void MapPoint::AddObservation(KeyFrame* keyframe, size_t idx)
{
	LOCK_MUTEX_FEATURES();

	if (observations_.count(keyframe))
		return;

	observations_[keyframe] = idx;

	if (keyframe->uright[idx] >= 0)
		nobservations_ += 2;
	else
		nobservations_++;
}

void MapPoint::EraseObservation(KeyFrame* keyframe)
{
	bool bad = false;
	{
		LOCK_MUTEX_FEATURES();
		if (observations_.count(keyframe))
		{
			const int idx = observations_[keyframe];
			if (keyframe->uright[idx] >= 0)
				nobservations_ -= 2;
			else
				nobservations_--;

			observations_.erase(keyframe);

			if (referenceKF_ == keyframe)
				referenceKF_ = !observations_.empty() ? std::begin(observations_)->first : nullptr;

			// If only 2 observations or less, discard point
			if (nobservations_ <= 2)
				bad = true;
		}
	}

	if (bad)
		SetBadFlag();
}

std::map<KeyFrame*, size_t> MapPoint::GetObservations()
{
	LOCK_MUTEX_FEATURES();
	return observations_;
}

int MapPoint::Observations()
{
	LOCK_MUTEX_FEATURES();
	return nobservations_;
}

void MapPoint::SetBadFlag()
{
	std::map<KeyFrame*, size_t> observations;
	{
		LOCK_MUTEX_FEATURES();
		LOCK_MUTEX_POSITION();
		bad_ = true;
		observations = observations_;
		observations_.clear();
	}

	for (const auto& observation : observations)
	{
		KeyFrame* keyframe = observation.first;
		keyframe->EraseMapPointMatch(observation.second);
	}

	map_->EraseMapPoint(this);
}

MapPoint* MapPoint::GetReplaced()
{
	LOCK_MUTEX_FEATURES();
	LOCK_MUTEX_POSITION();
	return replaced_;
}

void MapPoint::Replace(MapPoint* mappoint)
{
	if (mappoint->id == this->id)
		return;

	int nvisible = 0, nfound = 0;
	std::map<KeyFrame*, size_t> observations;
	{
		LOCK_MUTEX_FEATURES();
		LOCK_MUTEX_POSITION();
		observations = observations_;
		observations_.clear();
		bad_ = true;
		nvisible = nvisible_;
		nfound = nfound_;
		replaced_ = mappoint;
	}

	for (const auto& observation : observations)
	{
		// Replace measurement in keyframe
		KeyFrame* keyframe = observation.first;
		const size_t idx = observation.second;

		if (!mappoint->IsInKeyFrame(keyframe))
		{
			keyframe->ReplaceMapPointMatch(idx, mappoint);
			mappoint->AddObservation(keyframe, idx);
		}
		else
		{
			keyframe->EraseMapPointMatch(idx);
		}
	}
	mappoint->IncreaseFound(nfound);
	mappoint->IncreaseVisible(nvisible);
	mappoint->ComputeDistinctiveDescriptors();

	map_->EraseMapPoint(this);
}

bool MapPoint::isBad()
{
	LOCK_MUTEX_FEATURES();
	LOCK_MUTEX_POSITION();
	return bad_;
}

void MapPoint::IncreaseVisible(int n)
{
	LOCK_MUTEX_FEATURES();
	nvisible_ += n;
}

void MapPoint::IncreaseFound(int n)
{
	LOCK_MUTEX_FEATURES();
	nfound_ += n;
}

float MapPoint::GetFoundRatio()
{
	LOCK_MUTEX_FEATURES();
	return static_cast<float>(nfound_) / nvisible_;
}

void MapPoint::ComputeDistinctiveDescriptors()
{
	// Retrieve all observed descriptors
	std::map<KeyFrame*, size_t> observations;
	{
		LOCK_MUTEX_FEATURES();
		if (bad_)
			return;
		observations = observations_;
	}

	if (observations.empty())
		return;

	std::vector<cv::Mat> descriptors;
	descriptors.reserve(observations.size());

	//for (std::map<KeyFrame*, size_t>::iterator mit = observations.begin(), mend = observations.end(); mit != mend; mit++)
	for (const auto& observation : observations)
	{
		KeyFrame* keyframe = observation.first;
		const int idx = static_cast<int>(observation.second);
		if (!keyframe->isBad())
			descriptors.push_back(keyframe->descriptorsL.row(idx));
	}

	if (descriptors.empty())
		return;

	// Compute distances between them
	const size_t N = descriptors.size();

	std::vector<std::vector<int>> distances(N, std::vector<int>(N, 0));
	for (size_t i = 0; i < N; i++)
	{
		distances[i][i] = 0;
		for (size_t j = i + 1; j < N; j++)
		{
			const int distij = ORBmatcher::DescriptorDistance(descriptors[i], descriptors[j]);
			distances[i][j] = distij;
			distances[j][i] = distij;
		}
	}

	// Take the descriptor with least median distance to the rest
	int bestMedian = std::numeric_limits<int>::max();
	int bestIdx = 0;
	for (size_t i = 0; i < N; i++)
	{
		std::vector<int> dists(distances[i]);
		std::sort(std::begin(dists), std::end(dists));
		const int median = dists[(N - 1) / 2];

		if (median < bestMedian)
		{
			bestMedian = median;
			bestIdx = i;
		}
	}

	{
		LOCK_MUTEX_FEATURES();
		descriptor_ = descriptors[bestIdx].clone();
	}
}

cv::Mat MapPoint::GetDescriptor()
{
	LOCK_MUTEX_FEATURES();
	return descriptor_.clone();
}

int MapPoint::GetIndexInKeyFrame(KeyFrame *pKF)
{
	LOCK_MUTEX_FEATURES();
	if (observations_.count(pKF))
		return observations_[pKF];
	else
		return -1;
}

bool MapPoint::IsInKeyFrame(KeyFrame *pKF)
{
	LOCK_MUTEX_FEATURES();
	return (observations_.count(pKF));
}

void MapPoint::UpdateNormalAndDepth()
{
	std::map<KeyFrame*, size_t> observations;
	KeyFrame* pRefKF;
	cv::Mat Pos;
	{
		LOCK_MUTEX_FEATURES();
		LOCK_MUTEX_POSITION();
		if (bad_)
			return;
		observations = observations_;
		pRefKF = referenceKF_;
		Pos = Xw_.clone();
	}

	if (observations.empty())
		return;

	cv::Mat normal = cv::Mat::zeros(3, 1, CV_32F);
	int n = 0;
	for (std::map<KeyFrame*, size_t>::iterator mit = observations.begin(), mend = observations.end(); mit != mend; mit++)
	{
		KeyFrame* pKF = mit->first;
		cv::Mat Owi = pKF->GetCameraCenter();
		cv::Mat normali = Xw_ - Owi;
		normal = normal + normali / cv::norm(normali);
		n++;
	}

	cv::Mat PC = Pos - pRefKF->GetCameraCenter();
	const float dist = cv::norm(PC);
	const int level = pRefKF->keypointsUn[observations[pRefKF]].octave;
	const float levelScaleFactor = pRefKF->pyramid.scaleFactors[level];
	const int nLevels = pRefKF->pyramid.nlevels;

	{
		LOCK_MUTEX_POSITION();
		maxDistance_ = dist*levelScaleFactor;
		minDistance_ = maxDistance_ / pRefKF->pyramid.scaleFactors[nLevels - 1];
		normal_ = normal / n;
	}
}

float MapPoint::GetMinDistanceInvariance()
{
	LOCK_MUTEX_POSITION();
	return 0.8f*minDistance_;
}

float MapPoint::GetMaxDistanceInvariance()
{
	LOCK_MUTEX_POSITION();
	return 1.2f*maxDistance_;
}

int MapPoint::PredictScale(const float &currentDist, KeyFrame* pKF)
{
	float ratio;
	{
		LOCK_MUTEX_POSITION();
		ratio = maxDistance_ / currentDist;
	}

	int nScale = ceil(log(ratio) / pKF->pyramid.logScaleFactor);
	if (nScale < 0)
		nScale = 0;
	else if (nScale >= pKF->pyramid.nlevels)
		nScale = pKF->pyramid.nlevels - 1;

	return nScale;
}

int MapPoint::PredictScale(const float &currentDist, Frame* pF)
{
	float ratio;
	{
		LOCK_MUTEX_POSITION();
		ratio = maxDistance_ / currentDist;
	}

	int nScale = ceil(log(ratio) / pF->pyramid.logScaleFactor);
	if (nScale < 0)
		nScale = 0;
	else if (nScale >= pF->pyramid.nlevels)
		nScale = pF->pyramid.nlevels - 1;

	return nScale;
}



} //namespace ORB_SLAM
