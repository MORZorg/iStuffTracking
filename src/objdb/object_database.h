/**
 * @file	object_database.h
 * @brief	Library for ObjectDatabase class
 * @author	Mattia Rizzini
 * @version	0.1.0
 * @date	2013-07-14
 */

#ifndef OBJECT_DATABASE_H__
#define OBJECT_DATABASE_H__

// Standard C++ libraries
#include <iostream>
#include <vector>
#include <string>

// OpenCV libraries
#include "opencv2/highgui/highgui.hpp"

// Boost libraries
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

using namespace std;
using namespace cv;

namespace fs = boost::filesystem;

extern bool debug;

class ObjectDatabase {
	private:
		string dbPath;
		string dbName;

	public:
		ObjectDatabase( string );
		ObjectDatabase( string, string );
		virtual ~ObjectDatabase();
		
		Rect match( Mat );

	private:
		void load();
		void build( string );
		void save();
};

class DBNotExistsException: public exception {
	virtual const char* what() const throw() {
		return "Error in Database loading, the selected DB doesn't exist.";
	}
};

class DBExistsException: public exception {
	virtual const char* what() const throw() {
		return "Error in Database creation, the DB you want to create already exists.";
	}
};

#endif
