/****************************************************************
 *
 * Copyright (c) 2015
 *
 * Fraunhofer Institute for Manufacturing Engineering
 * and Automation (IPA)
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Project name: squirrel
 * ROS stack name: squirrel_calibration
 * ROS package name: robotino_calibration
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Author: Marc Riedlinger, email:marc.riedlinger@ipa.fraunhofer.de
 *
 * Date of creation: June 2018
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Fraunhofer Institute for Manufacturing
 *       Engineering and Automation (IPA) nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License LGPL as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License LGPL for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License LGPL along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************/


#include <ros/ros.h>
#include <string>
#include <image_transport/image_transport.h>
#include <image_transport/subscriber_filter.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include <sensor_msgs/CameraInfo.h>
#include <message_filters/subscriber.h>
#include <robotino_calibration/transformation_utilities.h>
#include <calibration_interface/checkerboard_marker.h>
#include <tf/tf.h>
#include <tf/transform_broadcaster.h>


// Global variables
boost::mutex camera_data_mutex;
cv::Mat camera_image;
ros::Time latest_image_time;
cv::Mat cam_matrix;// = cv::Mat::eye(3, 3, CV_64F);
cv::Mat distortion;// = cv::Mat::zeros(8, 1, CV_64F);
bool initialized = false;
int num_distortion_params = 0;


bool convertImageMessageToMat(const sensor_msgs::Image::ConstPtr& image_msg, cv_bridge::CvImageConstPtr& image_ptr, cv::Mat& image)
{
	try
	{
		image_ptr = cv_bridge::toCvShare(image_msg, sensor_msgs::image_encodings::BGR8);//image_msg->encoding);
	}
	catch (cv_bridge::Exception& e)
	{
		ROS_ERROR("ImageFlip::convertColorImageMessageToMat: cv_bridge exception: %s", e.what());
		return false;
	}
	image = image_ptr->image.clone();

	return true;
}

void infoCallback(const sensor_msgs::CameraInfoConstPtr& camera_info_msg)
{
	if ( !initialized )
	{
		cam_matrix = (cv::Mat_<double>(3,3) <<
					camera_info_msg->K[0], camera_info_msg->K[1], camera_info_msg->K[2],
					camera_info_msg->K[3], camera_info_msg->K[4], camera_info_msg->K[5],
					camera_info_msg->K[6], camera_info_msg->K[7], camera_info_msg->K[8]);

		if ( num_distortion_params > 0 )
		{
			distortion = cv::Mat::zeros(num_distortion_params, 1, CV_64F);
			for ( int i=0; i<num_distortion_params; ++i )
				distortion.at<double>(i) = camera_info_msg->D[i];
		}
		else // fallback
		{
			ROS_WARN("Zero distortion parameters, using plumb_bob model with all entries set to zero.");
			distortion = cv::Mat::zeros(5, 1, CV_64F);
		}

		if ( !cam_matrix.empty() && !distortion.empty() )
			initialized = true;
	}
}

void imageCallback(const sensor_msgs::ImageConstPtr& color_image_msg)
{
	boost::mutex::scoped_lock lock(camera_data_mutex);

	// read image
	cv_bridge::CvImageConstPtr color_image_ptr;
	bool result = convertImageMessageToMat(color_image_msg, color_image_ptr, camera_image);

	// delete image if conversion failed
	if ( !result )
		camera_image.release();
	else
	{
		latest_image_time = color_image_msg->header.stamp;

		if ( !latest_image_time.isValid() )
			latest_image_time = ros::Time::now();
	}
}

// detect and publish checkerboard markers
int main(int argc, char** argv)
{
	// Initialize ROS, specify name of node
	ros::init(argc, argv, "checkerboard_detection");

	// Create a handle for this node, initialize node
	ros::NodeHandle node_handle("~");

	// Load necessary parameters
	std::cout << "\n========== Checkerboard Detection Node Parameters ==========\n";
	std::string camera_frame;
	node_handle.param<std::string>("camera_frame", camera_frame, "");
	std::cout << "camera_frame: " << camera_frame << std::endl;

	std::string checkerboard_frame;
	node_handle.param<std::string>("checkerboard_frame", checkerboard_frame, "");
	std::cout << "checkerboard_frame: " << checkerboard_frame << std::endl;

	std::string camera_image_topic;
	node_handle.param<std::string>("camera_image_raw_topic", camera_image_topic, "");
	std::cout << "camera_image_raw_topic: " << camera_image_topic << std::endl;

	std::string camera_info;
	node_handle.param<std::string>("camera_info_topic", camera_info, "");
	std::cout << "camera_info_topic: " << camera_info << std::endl;

	node_handle.param("number_distortion_parameters", num_distortion_params, 0);
	num_distortion_params = std::max(num_distortion_params, 0);  // min: 0
	std::cout << "number_distortion_parameters: " << num_distortion_params << std::endl;

	double checkerboard_cell_size;
	node_handle.param("checkerboard_cell_size", checkerboard_cell_size, 0.05);
	std::cout << "checkerboard_cell_size: " << checkerboard_cell_size << std::endl;

	cv::Size checkerboard_pattern_size;
	std::vector<double> temp;
	node_handle.getParam("checkerboard_pattern_size", temp);
	if (temp.size() == 2)
		checkerboard_pattern_size = cv::Size(temp[0], temp[1]);

	double update_freq;
	node_handle.param("update_frequency", update_freq, 10.0);
	update_freq = fmax(update_freq, -update_freq);  // must be positive
	std::cout << "update_frequency: " << update_freq << std::endl;

	double update_pct;  // determines the percentage of how much a new transform will update the old one
	node_handle.param("update_percentage", update_pct, 0.5);
	update_pct = fmin( fmax(update_pct, 0.1), 1.0 );  // between [0.1, 1]
	std::cout << "update_percentage: " << update_pct << std::endl;

	// Set up callback
	ros::Subscriber info_sub = node_handle.subscribe<sensor_msgs::CameraInfo>(camera_info, 0, infoCallback);

	image_transport::ImageTransport it(node_handle);
	image_transport::SubscriberFilter color_image_sub;
	color_image_sub.subscribe(it, camera_image_topic, 1);
	color_image_sub.registerCallback(boost::bind(&imageCallback, _1));

	// wait for intrinsic parameters to initialize
	while ( !initialized )
	{
		ros::spinOnce();
		ros::Duration(0.25).sleep();
	}

	std::cout << "Intrinsic parameters loaded" << std::endl;

	// Cyclically detect checkerboard and publish resulting frame to tf
	tf::Vector3 avg_translation;
	tf::Quaternion avg_orientation;
	tf::TransformBroadcaster transform_broadcaster;

	ros::Rate rate(update_freq);  // in Hz
	while ( ros::ok() )
	{
		ros::spinOnce();

		if ( (ros::Time::now() - latest_image_time).toSec() < 10.f )
		{
			boost::mutex::scoped_lock lock(camera_data_mutex);

			if ( !camera_image.empty() )
			{
				int image_width = camera_image.cols;
				int image_height = camera_image.rows;

				// process color image and check if all corner points are available
				cv::Mat image, gray;
				image = camera_image.clone();

				// convert to grayscale
				gray = cv::Mat::zeros(image_height, image_width, CV_8UC1);
				cv::cvtColor(image, gray, CV_BGR2GRAY);

				std::vector<cv::Point2f> checkerboard_points_2d;
				bool pattern_found = cv::findChessboardCorners(gray, checkerboard_pattern_size, checkerboard_points_2d,
																   cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE + cv::CALIB_CB_FAST_CHECK);

				if ( pattern_found )
				{
					// improves result, taken from https://docs.opencv.org/2.4/doc/tutorials/calib3d/camera_calibration/camera_calibration.html
					cv::cornerSubPix( gray, checkerboard_points_2d, cv::Size(11,11),
										cv::Size(-1,-1), cv::TermCriteria( cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1 ));

					// compute checkerboard transform
					std::vector<cv::Point3f> pattern_points_3d;
					CheckerboardMarker::getPatternPoints3D(pattern_points_3d, checkerboard_pattern_size, checkerboard_cell_size);

					// get rotation and translation vectors
					cv::Mat rvec, tvec;
					cv::solvePnPRansac(pattern_points_3d, checkerboard_points_2d, cam_matrix, distortion, rvec, tvec);

					if ( rvec.empty() || rvec.size() != tvec.size() )  // check for valid size, we should have a size of one for both vectors
						continue;

					cv::Mat R;
					cv::Rodrigues(rvec, R);
					tf::Vector3 translation(tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2));
					cv::Vec3d ypr = transform_utilities::YPRFromRotationMatrix(R);
					tf::Quaternion orientation;
					orientation.setRPY(ypr.val[2], ypr.val[1], ypr.val[0]);

					// publish checkerboard transform to tf
					tf::Transform transform;

					// update transform
					if ( avg_translation.isZero() )
					{
						// use value directly on first message
						avg_translation = translation;
						avg_orientation = orientation;
					}
					else
					{
						// update value
						avg_translation = (1.0 - update_pct) * avg_translation + update_pct * translation;
						avg_orientation.setW((1.0 - update_pct) * avg_orientation.getW() + update_pct * orientation.getW());
						avg_orientation.setX((1.0 - update_pct) * avg_orientation.getX() + update_pct * orientation.getX());
						avg_orientation.setY((1.0 - update_pct) * avg_orientation.getY() + update_pct * orientation.getY());
						avg_orientation.setZ((1.0 - update_pct) * avg_orientation.getZ() + update_pct * orientation.getZ());
					}

					transform.setOrigin(avg_translation);
					transform.setRotation(avg_orientation);

					tf::StampedTransform tf_msg(transform, ros::Time::now(), camera_frame, checkerboard_frame);
					transform_broadcaster.sendTransform(tf_msg);
				}
			}
		}

		rate.sleep();  // try to keep looping at update_freq
	}
}
