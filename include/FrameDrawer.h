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

#ifndef FRAMEDRAWER_H
#define FRAMEDRAWER_H

#include <mutex>

#include <opencv2/opencv.hpp>

#include "Frame.h"

namespace ORB_SLAM2
{

class Map;
class Tracking;

class FrameDrawer
{
public:

	FrameDrawer(Map* pMap);

	// Update info from the last processed frame.
	void Update(const Tracking* tracker, const Frame& currFrame, const cv::Mat& image);

	// Draw last processed frame.
	cv::Mat DrawFrame();

private:

	// Info of the frame to be drawn
	cv::Mat image_;
	std::vector<cv::KeyPoint> currKeyPoints_;
	std::vector<int> status_;
	bool localizationMode_;
	int ntracked_, ntrackedVO_;
	std::vector<cv::KeyPoint> initKeyPoints_;
	std::vector<int> initMatches_;
	int state_;

	Map* map_;
	std::mutex mutex_;
};

} //namespace ORB_SLAM

#endif // FRAMEDRAWER_H
