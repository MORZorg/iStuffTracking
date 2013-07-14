/**
 * @file object_database.h
 * @brief Library for ObjectRecognition class
 * @author Mattia Rizzini
 * @version 0.1.0
 * @date 2013-07-14
 */

#ifndef OBJECT_DATABASE_H__
#define OBJECT_DATABASE_H__

#include <iostream>

#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

extern debug;

class ObjectDatabase {
	private:

	public:
		ObjectDatabase();
		virtual ~ObjectDatabase();

		load( String db_name );
		build( VideoCapture* );
}

#endif
