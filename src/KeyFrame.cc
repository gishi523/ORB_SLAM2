/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Ra�Yl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
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

#include "KeyFrame.h"

#include "Map.h"
#include "MapPoint.h"
#include "KeyFrameDatabase.h"
#include "Converter.h"

#define LOCK_MUTEX_POSE()        std::unique_lock<std::mutex> lock1(mutexPose_);
#define LOCK_MUTEX_CONNECTIONS() std::unique_lock<std::mutex> lock2(mutexConnections_);
#define LOCK_MUTEX_FEATURES()    std::unique_lock<std::mutex> lock3(mutexFeatures_);

namespace ORB_SLAM2
{

frameid_t KeyFrame::nextId = 0;

using WeightAndKeyFrame = std::pair<int, KeyFrame*>;

template <typename T, typename U>
static void Split(const std::vector<std::pair<T, U>>& vec12, std::vector<T>& vec1, std::vector<U>& vec2)
{
	vec1.clear();
	vec2.clear();
	for (const auto& v : vec12)
	{
		vec1.push_back(v.first);
		vec2.push_back(v.second);
	}
}

KeyFrame::KeyFrame(const Frame& frame, Map* map, KeyFrameDatabase* keyframeDB) :
	frameId(frame.id), timestamp(frame.timestamp), grid(frame.grid),
	trackReferenceForFrame(0), fuseTargetForKF(0), BALocalForKF(0), BAFixedForKF(0),
	loopQuery(0), loopWords(0), relocQuery(0), relocWords(0), BAGlobalForKF(0),
	camera(frame.camera), N(frame.N), keypointsL(frame.keypoints), keypointsUn(frame.keypointsUn),
	uright(frame.uright), depth(frame.depth), descriptorsL(frame.descriptors.clone()),
	bowVector(frame.bowVector), featureVector(frame.featureVector), pyramid(frame.pyramid), imageBounds(frame.imageBounds),
	mappoints_(frame.mappoints), keyFrameDB_(keyframeDB),
	voc_(frame.voc), firstConnection_(true), parent_(nullptr), notErase_(false),
	toBeErased_(false), bad_(false), halfBaseline_(frame.camera.baseline / 2), map_(map)
{
	id = nextId++;
	SetPose(frame.pose);
}

void KeyFrame::ComputeBoW()
{
	if (!bowVector.empty() && !featureVector.empty())
		return;

	// Feature vector associate features with nodes in the 4th level (from leaves up)
	// We assume the vocabulary tree has 6 levels, change the 4 otherwise
	voc_->transform(Converter::toDescriptorVector(descriptorsL), bowVector, featureVector, 4);
}

void KeyFrame::SetPose(const CameraPose& pose)
{
	LOCK_MUTEX_POSE();
	pose_ = pose;
}

CameraPose KeyFrame::GetPose() const
{
	LOCK_MUTEX_POSE();
	return pose_;
}

Point3D KeyFrame::GetCameraCenter() const
{
	LOCK_MUTEX_POSE();
	return pose_.Invt();
}

void KeyFrame::AddConnection(KeyFrame* keyframe, int weight)
{
	{
		LOCK_MUTEX_CONNECTIONS();
		if (!connectionTo_.count(keyframe) || connectionTo_[keyframe] != weight)
			connectionTo_[keyframe] = weight;
		else
			return;
	}

	UpdateBestCovisibles();
}

void KeyFrame::UpdateBestCovisibles()
{
	LOCK_MUTEX_CONNECTIONS();

	std::vector<WeightAndKeyFrame> pairs;
	pairs.reserve(connectionTo_.size());

	for (const auto& v : connectionTo_)
		pairs.push_back(std::make_pair(v.second, v.first));

	std::sort(std::begin(pairs), std::end(pairs), std::greater<WeightAndKeyFrame>());
	Split(pairs, orderedWeights_, orderedConnectedKeyFrames_);
}

std::set<KeyFrame*> KeyFrame::GetConnectedKeyFrames() const
{
	LOCK_MUTEX_CONNECTIONS();
	std::set<KeyFrame*> s;
	for (const auto& v : connectionTo_)
		s.insert(v.first);
	return s;
}

std::vector<KeyFrame*> KeyFrame::GetVectorCovisibleKeyFrames() const
{
	LOCK_MUTEX_CONNECTIONS();
	return orderedConnectedKeyFrames_;
}

std::vector<KeyFrame*> KeyFrame::GetBestCovisibilityKeyFrames(int N) const
{
	LOCK_MUTEX_CONNECTIONS();
	N = std::min(N, static_cast<int>(orderedConnectedKeyFrames_.size()));
	return std::vector<KeyFrame*>(std::begin(orderedConnectedKeyFrames_), std::begin(orderedConnectedKeyFrames_) + N);
}

std::vector<KeyFrame*> KeyFrame::GetCovisiblesByWeight(int w) const
{
	LOCK_MUTEX_CONNECTIONS();

	if (orderedConnectedKeyFrames_.empty())
		return std::vector<KeyFrame*>();

	auto it = std::upper_bound(std::begin(orderedWeights_), std::end(orderedWeights_), w, std::greater<int>());
	if (it == std::end(orderedWeights_))
		return std::vector<KeyFrame*>();

	const auto n = std::distance(std::begin(orderedWeights_), it);
	return std::vector<KeyFrame*>(std::begin(orderedConnectedKeyFrames_), std::begin(orderedConnectedKeyFrames_) + n);
}

int KeyFrame::GetWeight(KeyFrame* keyframe) const
{
	LOCK_MUTEX_CONNECTIONS();
	return connectionTo_.count(keyframe) ? connectionTo_.at(keyframe) : 0;
}

void KeyFrame::AddMapPoint(MapPoint* mappiont, size_t idx)
{
	LOCK_MUTEX_FEATURES();
	mappoints_[idx] = mappiont;
}

void KeyFrame::EraseMapPointMatch(size_t idx)
{
	LOCK_MUTEX_FEATURES();
	mappoints_[idx] = nullptr;
}

void KeyFrame::EraseMapPointMatch(MapPoint* mappiont)
{
	const int idx = mappiont->GetIndexInKeyFrame(this);
	if (idx >= 0)
		mappoints_[idx] = nullptr;
}

void KeyFrame::ReplaceMapPointMatch(size_t idx, MapPoint* mappiont)
{
	mappoints_[idx] = mappiont;
}

std::set<MapPoint*> KeyFrame::GetMapPoints() const
{
	LOCK_MUTEX_FEATURES();
	std::set<MapPoint*> s;
	for (MapPoint* mappint : mappoints_)
		if (mappint && !mappint->isBad())
			s.insert(mappint);
	return s;
}

int KeyFrame::TrackedMapPoints(int minObs) const
{
	LOCK_MUTEX_FEATURES();

	const bool checkObs = minObs > 0;
	int npoints = 0;
	for (MapPoint* mappint : mappoints_)
	{
		if (!mappint || mappint->isBad())
			continue;

		if (checkObs)
		{
			if (mappint->Observations() >= minObs)
				npoints++;
		}
		else
		{
			npoints++;
		}
	}

	return npoints;
}

std::vector<MapPoint*> KeyFrame::GetMapPointMatches() const
{
	LOCK_MUTEX_FEATURES();
	return mappoints_;
}

MapPoint* KeyFrame::GetMapPoint(size_t idx) const
{
	LOCK_MUTEX_FEATURES();
	return mappoints_[idx];
}

void KeyFrame::UpdateConnections()
{
	std::vector<MapPoint*> mappoints;
	{
		LOCK_MUTEX_FEATURES();
		mappoints = mappoints_;
	}

	//For all map points in keyframe check in which other keyframes are they seen
	//Increase counter for those keyframes
	std::map<KeyFrame*, int> KFcounter;
	for (MapPoint* mappoint : mappoints)
	{
		if (!mappoint || mappoint->isBad())
			continue;

		for (const auto& observation : mappoint->GetObservations())
		{
			KeyFrame* keyframe = observation.first;
			if (keyframe->id == id)
				continue;
			KFcounter[keyframe]++;
		}
	}

	// This should not happen
	if (KFcounter.empty())
		return;

	//If the counter is greater than threshold add connection
	//In case no keyframe counter is over threshold add the one with maximum counter
	int maxCount = 0;
	KeyFrame* maxKF = nullptr;
	const int threshold = 15;

	std::vector<WeightAndKeyFrame> pairs;
	pairs.reserve(KFcounter.size());

	for (const auto& v : KFcounter)
	{
		KeyFrame* keyframe = v.first;
		const int count = v.second;
		if (count > maxCount)
		{
			maxCount = count;
			maxKF = keyframe;
		}
		if (count >= threshold)
		{
			pairs.push_back(std::make_pair(count, keyframe));
			keyframe->AddConnection(this, count);
		}
	}

	if (pairs.empty())
	{
		pairs.push_back(std::make_pair(maxCount, maxKF));
		maxKF->AddConnection(this, maxCount);
	}

	std::sort(std::begin(pairs), std::end(pairs), std::greater<WeightAndKeyFrame>());

	{
		LOCK_MUTEX_CONNECTIONS();
		connectionTo_ = KFcounter;
		Split(pairs, orderedWeights_, orderedConnectedKeyFrames_);

		if (firstConnection_ && id != 0)
		{
			parent_ = orderedConnectedKeyFrames_.front();
			parent_->AddChild(this);
			firstConnection_ = false;
		}
	}
}

void KeyFrame::AddChild(KeyFrame* keyframe)
{
	LOCK_MUTEX_CONNECTIONS();
	children_.insert(keyframe);
}

void KeyFrame::EraseChild(KeyFrame* keyframe)
{
	LOCK_MUTEX_CONNECTIONS();
	children_.erase(keyframe);
}

void KeyFrame::ChangeParent(KeyFrame* keyframe)
{
	LOCK_MUTEX_CONNECTIONS();
	parent_ = keyframe;
	keyframe->AddChild(this);
}

std::set<KeyFrame*> KeyFrame::GetChildren() const
{
	LOCK_MUTEX_CONNECTIONS();
	return children_;
}

KeyFrame* KeyFrame::GetParent() const
{
	LOCK_MUTEX_CONNECTIONS();
	return parent_;
}

bool KeyFrame::HasChild(KeyFrame* keyframe) const
{
	LOCK_MUTEX_CONNECTIONS();
	return children_.count(keyframe) > 0;
}

void KeyFrame::AddLoopEdge(KeyFrame* keyframe)
{
	LOCK_MUTEX_CONNECTIONS();
	notErase_ = true;
	loopEdges_.insert(keyframe);
}

std::set<KeyFrame*> KeyFrame::GetLoopEdges() const
{
	LOCK_MUTEX_CONNECTIONS();
	return loopEdges_;
}

void KeyFrame::SetNotErase()
{
	LOCK_MUTEX_CONNECTIONS();
	notErase_ = true;
}

void KeyFrame::SetErase()
{
	{
		LOCK_MUTEX_CONNECTIONS();
		if (loopEdges_.empty())
			notErase_ = false;
	}

	if (toBeErased_)
		SetBadFlag();
}

void KeyFrame::SetBadFlag()
{
	{
		LOCK_MUTEX_CONNECTIONS();

		if (id == 0)
			return;

		if (notErase_)
		{
			toBeErased_ = true;
			return;
		}
	}

	for (const auto& v : connectionTo_)
		v.first->EraseConnection(this);

	for (MapPoint* mappoint : mappoints_)
		if (mappoint)
			mappoint->EraseObservation(this);

	{
		LOCK_MUTEX_CONNECTIONS();
		LOCK_MUTEX_FEATURES();

		connectionTo_.clear();
		orderedConnectedKeyFrames_.clear();

		// Update Spanning Tree
		std::set<KeyFrame*> parentCandidates;
		parentCandidates.insert(parent_);

		// Assign at each iteration one children with a parent (the pair with highest covisibility weight)
		// Include that children as new parent candidate for the rest
		while (!children_.empty())
		{
			bool found = false;

			int maxCount = -1;
			KeyFrame* childKF = nullptr;
			KeyFrame* parentKF = nullptr;

			for (KeyFrame* child : children_)
			{
				if (child->isBad())
					continue;

				// Check if a parent candidate is connected to the keyframe
				for (KeyFrame* connectedKF : child->GetVectorCovisibleKeyFrames())
				{
					for (KeyFrame* candidateKF : parentCandidates)
					{
						if (connectedKF->id == candidateKF->id)
						{
							const int weight = child->GetWeight(connectedKF);
							if (weight > maxCount)
							{
								childKF = child;
								parentKF = connectedKF;
								maxCount = weight;
								found = true;
							}
						}
					}
				}
			}

			if (found)
			{
				childKF->ChangeParent(parentKF);
				parentCandidates.insert(childKF);
				children_.erase(childKF);
			}
			else
			{
				break;
			}
		}

		// If a children has no covisibility links with any parent candidate, assign to the original parent of this KF
		for (KeyFrame* child : children_)
			child->ChangeParent(parent_);

		parent_->EraseChild(this);
		Tcp = pose_ * parent_->GetPose().Inverse();
		bad_ = true;
	}

	map_->EraseKeyFrame(this);
	keyFrameDB_->erase(this);
}

bool KeyFrame::isBad() const
{
	LOCK_MUTEX_CONNECTIONS();
	return bad_;
}

void KeyFrame::EraseConnection(KeyFrame* keyframe)
{
	{
		LOCK_MUTEX_CONNECTIONS();
		if (connectionTo_.count(keyframe))
			connectionTo_.erase(keyframe);
		else
			return;
	}

	UpdateBestCovisibles();
}

std::vector<size_t> KeyFrame::GetFeaturesInArea(float x, float y, float r) const
{
	return grid.GetFeaturesInArea(x, y, r);
}

bool KeyFrame::IsInImage(float x, float y) const
{
	return imageBounds.Contains(x, y);
}

Point3D KeyFrame::UnprojectStereo(int i) const
{
	const float Zc = depth[i];
	if (Zc <= 0.f)
		return cv::Mat();

	const float invfx = 1.f / camera.fx;
	const float invfy = 1.f / camera.fy;

	const float u = keypointsL[i].pt.x;
	const float v = keypointsL[i].pt.y;

	const float Xc = (u - camera.cx) * Zc * invfx;
	const float Yc = (v - camera.cy) * Zc * invfy;

	Point3D x3Dc(Xc, Yc, Zc);

	LOCK_MUTEX_POSE();
	return pose_.InvR() * x3Dc + pose_.Invt();
}

float KeyFrame::ComputeSceneMedianDepth(int q) const
{
	std::vector<MapPoint*> mappoints;
	CameraPose Tcw_;
	{
		LOCK_MUTEX_FEATURES();
		LOCK_MUTEX_POSE();
		mappoints = mappoints_;
		Tcw_ = pose_;
	}

	std::vector<float> depths;
	depths.reserve(N);

	const auto Rcw2 = Tcw_.R().row(2).t();
	const float zcw = Tcw_.t()(2);

	for (MapPoint* mappoint : mappoints)
	{
		if (mappoint)
		{
			const Point3D x3Dw = mappoint->GetWorldPos();
			const float Z = Rcw2.dot(x3Dw) + zcw;
			depths.push_back(Z);
		}
	}

	std::sort(std::begin(depths), std::end(depths));

	return depths[(depths.size() - 1) / q];
}

} //namespace ORB_SLAM
