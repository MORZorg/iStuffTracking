/**
 * @file	object_database.h
 * @brief	Library for ObjectDatabase class
 * @author	Mattia Rizzini
 * @version	0.1.0
 * @date	2013-07-14
 */

#ifndef OBJECT_DATABASE_H__
#define OBJECT_DATABASE_H__

#include <iostream>

// Actually HORRIBLE
#include "../main.h"

#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

extern debug;

class ObjectDatabase {
	private:
		const String DATABASE_PATH = "./database/";
		String dbName;

	public:
		ObjectDatabase( String, String);
		virtual ~ObjectDatabase();
		
		void getDescriptorDB();

	private:
		void load();
		void build( String );
		void save();
}

#endif
