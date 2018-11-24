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

#include "MapDrawer.h"

#include "Map.h"
#include "MapPoint.h"
#include "KeyFrame.h"

namespace ORB_SLAM2
{


MapDrawer::MapDrawer(Map* map, const std::string &settingsFile) : map_(map)
{
	cv::FileStorage settings(settingsFile, cv::FileStorage::READ);

	keyFrameSize_ = settings["Viewer.KeyFrameSize"];
	keyFrameLineWidth_ = settings["Viewer.KeyFrameLineWidth"];
	graphLineWidth_ = settings["Viewer.GraphLineWidth"];
	pointSize_ = settings["Viewer.PointSize"];
	cameraSize_ = settings["Viewer.CameraSize"];
	cameraLineWidth_ = settings["Viewer.CameraLineWidth"];
}

void MapDrawer::DrawMapPoints()
{
	const std::vector<MapPoint*> &vpMPs = map_->GetAllMapPoints();
	const std::vector<MapPoint*> &vpRefMPs = map_->GetReferenceMapPoints();

	std::set<MapPoint*> spRefMPs(vpRefMPs.begin(), vpRefMPs.end());

	if (vpMPs.empty())
		return;

	glPointSize(pointSize_);
	glBegin(GL_POINTS);
	glColor3f(0.0, 0.0, 0.0);

	for (size_t i = 0, iend = vpMPs.size(); i < iend; i++)
	{
		if (vpMPs[i]->isBad() || spRefMPs.count(vpMPs[i]))
			continue;
		cv::Mat pos = vpMPs[i]->GetWorldPos();
		glVertex3f(pos.at<float>(0), pos.at<float>(1), pos.at<float>(2));
	}
	glEnd();

	glPointSize(pointSize_);
	glBegin(GL_POINTS);
	glColor3f(1.0, 0.0, 0.0);

	for (std::set<MapPoint*>::iterator sit = spRefMPs.begin(), send = spRefMPs.end(); sit != send; sit++)
	{
		if ((*sit)->isBad())
			continue;
		cv::Mat pos = (*sit)->GetWorldPos();
		glVertex3f(pos.at<float>(0), pos.at<float>(1), pos.at<float>(2));

	}

	glEnd();
}

void MapDrawer::DrawKeyFrames(const bool bDrawKF, const bool bDrawGraph)
{
	const float &w = keyFrameSize_;
	const float h = w*0.75;
	const float z = w*0.6;

	const std::vector<KeyFrame*> vpKFs = map_->GetAllKeyFrames();

	if (bDrawKF)
	{
		for (size_t i = 0; i < vpKFs.size(); i++)
		{
			KeyFrame* pKF = vpKFs[i];
			cv::Mat Twc = pKF->GetPoseInverse().t();

			glPushMatrix();

			glMultMatrixf(Twc.ptr<GLfloat>(0));

			glLineWidth(keyFrameLineWidth_);
			glColor3f(0.0f, 0.0f, 1.0f);
			glBegin(GL_LINES);
			glVertex3f(0, 0, 0);
			glVertex3f(w, h, z);
			glVertex3f(0, 0, 0);
			glVertex3f(w, -h, z);
			glVertex3f(0, 0, 0);
			glVertex3f(-w, -h, z);
			glVertex3f(0, 0, 0);
			glVertex3f(-w, h, z);

			glVertex3f(w, h, z);
			glVertex3f(w, -h, z);

			glVertex3f(-w, h, z);
			glVertex3f(-w, -h, z);

			glVertex3f(-w, h, z);
			glVertex3f(w, h, z);

			glVertex3f(-w, -h, z);
			glVertex3f(w, -h, z);
			glEnd();

			glPopMatrix();
		}
	}

	if (bDrawGraph)
	{
		glLineWidth(graphLineWidth_);
		glColor4f(0.0f, 1.0f, 0.0f, 0.6f);
		glBegin(GL_LINES);

		for (size_t i = 0; i < vpKFs.size(); i++)
		{
			// Covisibility Graph
			const std::vector<KeyFrame*> vCovKFs = vpKFs[i]->GetCovisiblesByWeight(100);
			cv::Mat Ow = vpKFs[i]->GetCameraCenter();
			if (!vCovKFs.empty())
			{
				for (std::vector<KeyFrame*>::const_iterator vit = vCovKFs.begin(), vend = vCovKFs.end(); vit != vend; vit++)
				{
					if ((*vit)->id < vpKFs[i]->id)
						continue;
					cv::Mat Ow2 = (*vit)->GetCameraCenter();
					glVertex3f(Ow.at<float>(0), Ow.at<float>(1), Ow.at<float>(2));
					glVertex3f(Ow2.at<float>(0), Ow2.at<float>(1), Ow2.at<float>(2));
				}
			}

			// Spanning tree
			KeyFrame* pParent = vpKFs[i]->GetParent();
			if (pParent)
			{
				cv::Mat Owp = pParent->GetCameraCenter();
				glVertex3f(Ow.at<float>(0), Ow.at<float>(1), Ow.at<float>(2));
				glVertex3f(Owp.at<float>(0), Owp.at<float>(1), Owp.at<float>(2));
			}

			// Loops
			std::set<KeyFrame*> sLoopKFs = vpKFs[i]->GetLoopEdges();
			for (std::set<KeyFrame*>::iterator sit = sLoopKFs.begin(), send = sLoopKFs.end(); sit != send; sit++)
			{
				if ((*sit)->id < vpKFs[i]->id)
					continue;
				cv::Mat Owl = (*sit)->GetCameraCenter();
				glVertex3f(Ow.at<float>(0), Ow.at<float>(1), Ow.at<float>(2));
				glVertex3f(Owl.at<float>(0), Owl.at<float>(1), Owl.at<float>(2));
			}
		}

		glEnd();
	}
}

void MapDrawer::DrawCurrentCamera(const pangolin::OpenGlMatrix &Twc)
{
	const float w = cameraSize_;
	const float h = 0.75f * w;
	const float z = 0.6f * w;

	glPushMatrix();

#ifdef HAVE_GLES
	glMultMatrixf(Twc.m);
#else
	glMultMatrixd(Twc.m);
#endif

	glLineWidth(cameraLineWidth_);
	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(w, h, z);
	glVertex3f(0, 0, 0);
	glVertex3f(w, -h, z);
	glVertex3f(0, 0, 0);
	glVertex3f(-w, -h, z);
	glVertex3f(0, 0, 0);
	glVertex3f(-w, h, z);

	glVertex3f(w, h, z);
	glVertex3f(w, -h, z);

	glVertex3f(-w, h, z);
	glVertex3f(-w, -h, z);

	glVertex3f(-w, h, z);
	glVertex3f(w, h, z);

	glVertex3f(-w, -h, z);
	glVertex3f(w, -h, z);
	glEnd();

	glPopMatrix();
}

void MapDrawer::SetCurrentCameraPose(const cv::Mat &Tcw)
{
	std::unique_lock<std::mutex> lock(mMutexCamera);
	mCameraPose = Tcw.clone();
}

void MapDrawer::GetCurrentOpenGLCameraMatrix(pangolin::OpenGlMatrix &M)
{
	if (!mCameraPose.empty())
	{
		cv::Mat1f Rwc(3, 3);
		cv::Mat1f twc(3, 1);
		{
			unique_lock<mutex> lock(mMutexCamera);
			Rwc = CameraPose::GetR(mCameraPose).t();
			twc = -Rwc * CameraPose::Gett(mCameraPose);
		}

		M.m[0] = Rwc(0, 0);
		M.m[1] = Rwc(1, 0);
		M.m[2] = Rwc(2, 0);
		M.m[3] = 0.0;

		M.m[4] = Rwc(0, 1);
		M.m[5] = Rwc(1, 1);
		M.m[6] = Rwc(2, 1);
		M.m[7] = 0.0;

		M.m[8] = Rwc(0, 2);
		M.m[9] = Rwc(1, 2);
		M.m[10] = Rwc(2, 2);
		M.m[11] = 0.0;

		M.m[12] = twc(0);
		M.m[13] = twc(1);
		M.m[14] = twc(2);
		M.m[15] = 1.0;
	}
	else
	{
		M.SetIdentity();
	}
}

} //namespace ORB_SLAM
