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

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <Thirdparty/g2o/g2o/types/types_seven_dof_expmap.h>

#include "FrameId.h"

namespace ORB_SLAM2
{

class Map;
class MapPoint;
class Frame;
class KeyFrame;

typedef std::map<KeyFrame*, g2o::Sim3, std::less<KeyFrame*>,
	Eigen::aligned_allocator<std::pair<const KeyFrame*, g2o::Sim3> > > KeyFrameAndPose;

namespace Optimizer
{
//public:
void BundleAdjustment(const std::vector<KeyFrame*> &vpKF, const std::vector<MapPoint*> &vpMP,
	int nIterations = 5, bool *pbStopFlag = nullptr, frameid_t nLoopKF = 0,
	bool bRobust = true);

void GlobalBundleAdjustemnt(Map* map, int niterations, bool* stopFlag = nullptr, frameid_t loopKFId = 0,
	bool robust = true);

void LocalBundleAdjustment(KeyFrame* pKF, bool *pbStopFlag, Map *pMap);

int PoseOptimization(Frame* pFrame);

// if bFixScale is true, 6DoF optimization (stereo,rgbd), 7DoF otherwise (mono)
void OptimizeEssentialGraph(Map* pMap, KeyFrame* pLoopKF, KeyFrame* pCurKF,
	const KeyFrameAndPose &NonCorrectedSim3,
	const KeyFrameAndPose &CorrectedSim3,
	const std::map<KeyFrame *, std::set<KeyFrame *> > &LoopConnections,
	const bool &bFixScale);

// if bFixScale is true, optimize SE3 (stereo,rgbd), Sim3 otherwise (mono)
int OptimizeSim3(KeyFrame* pKF1, KeyFrame* pKF2, std::vector<MapPoint *> &vpMatches1,
	g2o::Sim3 &g2oS12, const float th2, const bool bFixScale);
}

} //namespace ORB_SLAM

#endif // OPTIMIZER_H
