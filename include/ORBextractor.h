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

#ifndef ORBEXTRACTOR_H
#define ORBEXTRACTOR_H

#include <vector>
#include <list>

#include <opencv2/core.hpp>

#include "Point.h"

namespace ORB_SLAM2
{

class ORBextractor
{
public:

	struct Parameters
	{
		int nfeatures;
		float scaleFactor;
		int nlevels;
		int iniThFAST;
		int minThFAST;

		Parameters(int nfeatures = 2000, float scaleFactor = 1.2f, int nlevels = 8, int iniThFAST = 20, int minThFAST = 7);
	};

	ORBextractor(const Parameters& param);
	void Init();

	// Compute the ORB features and descriptors on an image.
	// ORB are dispersed on the image using an octree.
	// Mask is ignored in the current implementation.
	void Extract(const cv::Mat& image, KeyPoints& keypoints, cv::Mat& descriptors);

	int GetLevels() const;
	float GetScaleFactor() const;
	const std::vector<float>& GetScaleFactors() const;
	const std::vector<float>& GetInverseScaleFactors() const;
	const std::vector<float>& GetScaleSigmaSquares() const;	
	const std::vector<float>& GetInverseScaleSigmaSquares() const;
	const std::vector<cv::Mat>& GetImagePyramid() const;

private:

	std::vector<int> nfeaturesPerScale_, umax_;

	std::vector<float> scaleFactors_;
	std::vector<float> invScaleFactors_;
	std::vector<float> sigmaSq_;
	std::vector<float> invSigmaSq_;

	std::vector<cv::Mat> images_;
	std::vector<cv::Mat> blurImages_;
	std::vector<KeyPoints> keypoints_;
	std::vector<cv::Point> pattern_;

	Parameters param_;
};

} //namespace ORB_SLAM

#endif

