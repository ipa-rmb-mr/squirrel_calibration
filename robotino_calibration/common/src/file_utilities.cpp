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
 * Date of creation: July 2018
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


#include <robotino_calibration/file_utilities.h>
#include <ros/ros.h>
#include <boost/filesystem.hpp>
#include <fstream>


namespace file_utilities
{
	// create data storage path if it does not yet exist
	void createStorageFolder(std::string path)
	{
		boost::filesystem::path storage_path(path);

		if (boost::filesystem::exists(storage_path) == false)
		{
			if (boost::filesystem::create_directories(storage_path) == false && boost::filesystem::exists(storage_path) == false)
			{
				ROS_ERROR("RobotCalibration: Could not create directory %s ", storage_path.c_str());
				return;
			}
		}
	}

	void saveCalibrationResult(std::string file_path, std::string content)
	{
		std::fstream file_output;
		file_output.open(file_path.c_str(), std::ios::out | std::ios::app);  // append new results at end of file
		if (file_output.is_open())
			file_output << content;
		else
			ROS_WARN("Failed to open %s, not saving calibration results!", file_path.c_str());
		file_output.close();
	}
}



