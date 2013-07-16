/**
 * @file	object_database.h
 * @brief	Library for ObjectDatabase class
 * @author	Mattia Rizzini
 * @version	0.1.0
 * @date	2013-07-14
 */

#ifndef DATABASE_H__
#define DATABASE_H__

// Standard C++ libraries
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>

// OpenCV libraries
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"

// Boost libraries
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/archive/binary_oarchive.hpp"
#include "boost/archive/binary_iarchive.hpp"
#include "boost/serialization/map.hpp"
#include "serialize_mat.h"

extern bool debug;

namespace IStuff {
	typedef std::string Label;

	class ObjectDatabase {
		private:
			std::string dbPath;
			std::string dbName;
			std::map< Label, cv::Mat > descriptorDB;

		public:
			ObjectDatabase( std::string );
			ObjectDatabase( std::string, std::string );
			virtual ~ObjectDatabase();
			
			cv::Rect match( cv::Mat );

		private:
			void load();
			void build( std::string );
			void save();
	};

	class DBNotExistsException: public std::exception {
		virtual const char* what() const throw() {
			return "Error in Database loading, the selected DB doesn't exist.";
		}
	};

	class DBExistsException: public std::exception {
		virtual const char* what() const throw() {
			return "Error in Database creation, the DB you want to create already exists.";
		}
	};
};

#endif
