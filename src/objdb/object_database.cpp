/**
 * @file	object_database.cpp
 * @brief	Definition for ObjectDatabase class
 * @class	ObjectDatabase
 * @author	Mattia Rizzini
 * @version	0.1.0
 * @date	2013-07-14
 */

#include "object_database.h"

using namespace std;

/**
 * @brief	Constructor
 * @details	If the name passed matches an existing DB i load it, \
 			otherwise I generate the structures and fill them performing \
			a recognize over the images in the directory passed as second argument
 */
ObjectDatabase::ObjectDatabase( String _dbName, String imagesPath = "./image_sample/" ) {
	if( false )
		load();
	else {
		this -> dbName = _dbName;

		build( imagesPath );
	}
}

/**
 * @brief	Load existing database and fill the kd-tree containing descriptors
 * param[in] dbName	The name of the DB to load
 */
void ObjectDatabase::load() {

}

/**
 * @brief	Create a new kd-tree and save it to a file
 * @details	Loaded the images contained in the argument path, \
 			detects the SIFT features, turns them in descriptors \
 			and save these to the kd-tree representing the database. \
 			Also saves the database into a file
 * @param[in] imagesPath	The path containing the source images
 */
void ObjectDatabase::build( String imagesPath ) {
	
}

/**
 * @brief	Writes the database to a file in the default directory
 */
void ObjectDatabase::save() {

}

/**
 * @brief Returns the structure containing the descriptors
 */
void ObjectDatabase::getDescriptorDB() {

}
