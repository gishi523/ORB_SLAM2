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

#include "MapDrawer.h"

#include "Map.h"
#include "MapPoint.h"
#include "KeyFrame.h"
#include "CameraPose.h"

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

void MapDrawer::DrawMapPoints() const
{
	const std::vector<MapPoint*>& mappionts = map_->GetAllMapPoints();
	const std::vector<MapPoint*>& _referenceMPs = map_->GetReferenceMapPoints();

	std::set<MapPoint*> referenceMPs(std::begin(_referenceMPs), std::end(_referenceMPs));

	if (mappionts.empty())
		return;

	glPointSize(pointSize_);
	glBegin(GL_POINTS);
	glColor3f(0.f, 0.f, 0.f);

	for (MapPoint* mappoint : mappionts)
	{
		if (mappoint->isBad() || referenceMPs.count(mappoint))
			continue;
		const Point3D pos = mappoint->GetWorldPos();
		glVertex3f(pos(0), pos(1), pos(2));
	}
	glEnd();

	glPointSize(pointSize_);
	glBegin(GL_POINTS);
	glColor3f(1.f, 0.f, 0.f);

	for (MapPoint* mappoint : referenceMPs)
	{
		if (mappoint->isBad())
			continue;
		const Point3D pos = mappoint->GetWorldPos();
		glVertex3f(pos(0), pos(1), pos(2));
	}

	glEnd();
}

void MapDrawer::DrawKeyFrames(bool drawKF, bool drawGraph) const
{
	const float w = keyFrameSize_;
	const float h = 0.75f * w;
	const float z = 0.6f * w;

	const std::vector<KeyFrame*> keyframes = map_->GetAllKeyFrames();

	if (drawKF)
	{
		for (KeyFrame* keyframe : keyframes)
		{
			const cv::Mat Twc = keyframe->GetPose().Inverse().Mat().t();

			glPushMatrix();

			glMultMatrixf(Twc.ptr<GLfloat>(0));

			glLineWidth(keyFrameLineWidth_);
			glColor3f(0.f, 0.f, 1.f);
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

	if (drawGraph)
	{
		glLineWidth(graphLineWidth_);
		glColor4f(0.f, 1.f, 0.f, 0.6f);
		glBegin(GL_LINES);

		//for (size_t i = 0; i < keyframes.size(); i++)
		for (KeyFrame* keyframe : keyframes)
		{
			// Covisibility Graph
			const Point3D Ow = keyframe->GetCameraCenter();
			for (KeyFrame* covisibleKF : keyframe->GetCovisiblesByWeight(100))
			{
				if (covisibleKF->id < keyframe->id)
					continue;

				const Point3D Ow2 = covisibleKF->GetCameraCenter();
				glVertex3f(Ow(0), Ow(1), Ow(2));
				glVertex3f(Ow2(0), Ow2(1), Ow2(2));
			}

			// Spanning tree
			KeyFrame* parentKF = keyframe->GetParent();
			if (parentKF)
			{
				const Point3D Owp = parentKF->GetCameraCenter();
				glVertex3f(Ow(0), Ow(1), Ow(2));
				glVertex3f(Owp(0), Owp(1), Owp(2));
			}

			// Loops
			for (KeyFrame* loopKF : keyframe->GetLoopEdges())
			{
				if (loopKF->id < keyframe->id)
					continue;

				const Point3D Owl = loopKF->GetCameraCenter();
				glVertex3f(Ow(0), Ow(1), Ow(2));
				glVertex3f(Owl(0), Owl(1), Owl(2));
			}
		}

		glEnd();
	}
}

void MapDrawer::DrawCurrentCamera(const pangolin::OpenGlMatrix& Twc) const
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
	glColor3f(0.f, 1.f, 0.f);
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

void MapDrawer::SetCurrentCameraPose(const cv::Mat& Tcw)
{
	std::unique_lock<std::mutex> lock(mutexCamera_);
	cameraPose_ = Tcw.clone();
}

void MapDrawer::GetCurrentOpenGLCameraMatrix(pangolin::OpenGlMatrix& M) const
{
	if (!cameraPose_.empty())
	{
		CameraPose Tcw;
		{
			std::unique_lock<std::mutex> lock(mutexCamera_);
			Tcw = CameraPose(cameraPose_);
		}

		const auto Rwc = Tcw.InvR();
		const auto twc = Tcw.Invt();

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
