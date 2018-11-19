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

#ifndef LOCALMAPPING_H
#define LOCALMAPPING_H

#include <memory>

namespace ORB_SLAM2
{

class Tracking;
class LoopClosing;
class Map;
class KeyFrame;

class LocalMapping
{
public:

	static std::shared_ptr<LocalMapping> Create(Map* pMap, bool bMonocular);

	virtual void SetLoopCloser(LoopClosing* pLoopCloser) = 0;

	virtual void SetTracker(Tracking* pTracker) = 0;

	// Main function
	virtual void Run() = 0;

	virtual void InsertKeyFrame(KeyFrame* pKF) = 0;

	// Thread Synch
	virtual void RequestStop() = 0;
	virtual void RequestReset() = 0;
	virtual bool Stop() = 0;
	virtual void Release() = 0;
	virtual bool isStopped() = 0;
	virtual bool stopRequested() = 0;
	virtual bool AcceptKeyFrames() = 0;
	virtual void SetAcceptKeyFrames(bool flag) = 0;
	virtual bool SetNotStop(bool flag) = 0;

	virtual void InterruptBA() = 0;

	virtual void RequestFinish() = 0;
	virtual bool isFinished() = 0;

	virtual int KeyframesInQueue() = 0;
};

} //namespace ORB_SLAM

#endif // LOCALMAPPING_H
