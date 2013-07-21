/**
* @file database.h
* @brief Library for Database class
* @author Mattia Rizzini
* @version 0.1.3
* @date 2013-07-14
*/

#ifndef DATABASE_H__
#define DATABASE_H__

// Standard C++ libraries
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// Custom header files
#include "object.h"

// OpenCV libraries
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"

// Boost libraries
#include "boost/filesystem.hpp"
#include "boost/lambda/bind.hpp"
#include "boost/foreach.hpp"

#include "boost/archive/binary_oarchive.hpp"
#include "boost/archive/binary_iarchive.hpp"

#include "boost/random.hpp"

#include "boost/serialization/vector.hpp"
#include "serialize_opencv.h"

#include "boost/lexical_cast.hpp"

// DIP course library
#include "transform.h"

extern bool debug;

namespace IStuff {
	class Database {
		private:
			std::string dbPath;
			std::string dbName;

			cv::FlannBasedMatcher matcher;
			cv::BFMatcher bfmatcher;
			std::vector< Label > labelDB;
			std::vector< cv::Scalar > labelColor;
			std::vector< std::vector< cv::Point2f > > cornersDB;
			std::vector< std::vector< cv::KeyPoint > > keypointDB;
			std::vector< cv::Mat > descriptorDB;

		public:
			Database( std::string, std::string = "image_sample/" );
			virtual ~Database();

			Object match( cv::Mat );

		private:
			void load();
			void build( std::string );
			void save();
			cv::Mat featureReduction( cv::Mat, cv::Ptr< cv::FeatureDetector >, cv::Ptr< cv::DescriptorExtractor > );
	};

	class DBCreationException: public std::exception {
		public: virtual const char* what() const throw() {
			return "***Error in Database creation, no files in given directory or wrong path given***\n";
		}
	};

	class DBLoadingException: public std::exception {
		public: virtual const char* what() const throw() {
			return "***Error in Database loading, database seems to exist but files are missing***\n";
		}
	};

	class DBSavingException: public std::exception {
		public: virtual const char* what() const throw() {
			return "***Error in Database saving***\n";
		}
	};
};

#endif
