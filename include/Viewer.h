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


#ifndef VIEWER_H
#define VIEWER_H

#include <opencv2/opencv.hpp>

#include <string>
#include <mutex>
#include <memory>

#include "Frame.h"

namespace ORB_SLAM2
{

class System;
class Map;
class Tracking;
class FrameDrawer;
class MapDrawer;

class Viewer
{
public:

	Viewer(System* system, Map* map, const std::string& settingsFile);
	~Viewer();

	// Main thread function. Draw points, keyframes, the current camera pose and the last processed
	// frame. Drawing is refreshed according to the camera fps. We use Pangolin.
	void Run();
	void RequestFinish();
	void RequestStop();
	bool isFinished() const;
	bool isStopped() const;
	void Release();

	void SetCurrentCameraPose(const cv::Mat& Tcw);
	void UpdateFrame(const Tracking* tracker, const Frame& currFrame, const cv::Mat& image);

private:

	bool Stop();
	bool CheckFinish() const;
	void SetFinish();

	System* system_;
	std::unique_ptr<FrameDrawer> frameDrawer_;
	std::unique_ptr<MapDrawer> mapDrawer_;
	
	// 1/fps in ms
	int waittime_;
	
	float viewpointX_, viewpointY_, viewpointZ_, viewpointF_;
	bool finishRequested_;
	bool finished_;
	bool stopped_;
	bool stopRequested_;

	mutable std::mutex mutexFinish_;
	mutable std::mutex mutexStop_;
};

}


#endif // VIEWER_H


