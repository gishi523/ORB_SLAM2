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


#ifndef CAMERAPARAMETERS_H
#define CAMERAPARAMETERS_H

#include <opencv2/opencv.hpp>

namespace ORB_SLAM2
{

struct CameraParams
{
	float fx;                 //!< focal length x (pixel)
	float fy;                 //!< focal length y (pixel)
	float cx;                 //!< principal point x (pixel)
	float cy;                 //!< principal point y (pixel)
	float bf;                 //!< stereo baseline times fx
	float baseline;

	CameraParams();
	cv::Mat1f Mat() const;
};

}

#endif // CAMERA_PARAMETERS_H
	

