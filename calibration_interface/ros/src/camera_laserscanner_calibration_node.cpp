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
 * Author: Richard Bormann, email:richard.bormann@ipa.fhg.de
 *
 * Date of creation: December 2015
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
#include <robotino_calibration/robot_calibration.h>
#include <robotino_calibration/calibration_interface.h>
#include <calibration_interface/ipa_interface.h>
#include <calibration_interface/calibration_marker.h>
#include <calibration_interface/pitag_marker.h>
#include <calibration_interface/checkerboard_marker.h>
#include <calibration_interface/calibration_type.h>
#include <calibration_interface/camera_laserscanner_type.h>


//#######################
//#### main programm ####
int main(int argc, char** argv)
{
	// Initialize ROS, specify name of node
	ros::init(argc, argv, "camera_base_calibration");

	// Create a handle for this node, initialize node
	ros::NodeHandle nh("~");

	// load parameters
	std::string marker_type;
	bool load_data = false;
	std::cout << "\n========== Relative Localization Parameters ==========\n";
	nh.param<std::string>("marker_type", marker_type, "");
	std::cout << "marker_type: " << marker_type << std::endl;
	nh.param("load_data", load_data, false);
	std::cout << "load_data: " << load_data << std::endl;
	int calibration_ID = 0;
	nh.param("calibration_ID", calibration_ID, 0);
	std::cout << "calibration_ID: " << calibration_ID << std::endl;

	bool arm_calibration = false;

	// setting up objects needed for calibration
	CalibrationType* calibration_type = new CameraLaserscannerType();
	CalibrationMarker* marker = 0;
	CalibrationInterface* interface = 0;

	if (marker_type.compare("checkerboard") == 0)
	{
		marker = new CheckerboardMarker();
	}
	else if (marker_type.compare("pitag") == 0)
	{
		marker = new PitagMarker();
	}

	if ( marker != 0 )
		marker->initialize(nh);
	else
	{
		ROS_WARN("Marker object has not been created!");
		return -1;
	}

	interface = IPAInterface::createInterfaceByID(calibration_ID, nh, calibration_type, marker, arm_calibration);

	if ( interface != 0 )
	{
		try
		{
			RobotCalibration calib_obj(nh, interface);
			calib_obj.startCalibration(load_data);
		}
		catch ( std::exception &e )
		{
			return -1;
		}
	}
	else
	{
		ROS_WARN("Interface object has not been created!");
		return -1;
	}

	return 0;
}
