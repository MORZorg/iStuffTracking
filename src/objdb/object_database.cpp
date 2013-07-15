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
using namespace cv;

namespace fs = boost::filesystem;

/**
 * @brief	Load DB Constructor
 * @details	If the name passed matches an existing DB it loads it, \
 			otherwise throws an exception
 * @param[in] _dbName	The name of the DB to be loaded
 */
ObjectDatabase::ObjectDatabase( string _dbName ) :
	dbPath( "./database" ), dbName( _dbName )
{
	// Add existence check
	if( false )
		throw DBNotExistsException();

	if( debug )
		cerr << "Opening DB " << _dbName << endl;

	load();
}

/**
 * @brief	Create DB Constructor
 * @details	If the name passed matches an existing DB throws an exception, \
 			otherwise generates the structures and fill them performing \
			a recognize over the images in the directory passed as second argument.
 * @param[in] _dbName		The name of the DB to be created
 * @param[in] imagesPath	The position of the images from which the descriptors are to be taken
 */
ObjectDatabase::ObjectDatabase( string _dbName, string imagesPath ) :
	dbPath( "./database" ), dbName( _dbName )
{
	// Add existence check
	if( false )
		throw DBExistsException();

	if( debug )
		cerr << _dbName << " doesn't exists. Start creating it" << endl;

	build( imagesPath );
}

/**
 * @brief	Destructor
 */
ObjectDatabase::~ObjectDatabase() {

}

/**
 * @brief	Search for descriptors matching in passed frame
 * @details	Given an image, searches for the descriptors in the database \
 			and returns a rectangle enclosing the matched object
 * @param[in] frame	The image to search into
 * @retval	A Rect which encloses the object in the frame reference system
 			or an empty Rect if nothing is found
 */
Rect ObjectDatabase::match( Mat frame ) {
	Rect encloser( 0, 0, 0, 0 );

	return encloser;
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
void ObjectDatabase::build( string imagesPath ) {
	// Load images in a Mat vector (mostly thanks to boost)
	vector< Mat > samples = vector< Mat >();	
	
	fs::path fullPath = fs::system_complete( fs::path( imagesPath ) );

	if( debug )
		cerr << "Loading images from " << fullPath << endl;

	if( !fs::exists( fullPath ) || !fs::is_directory( fullPath ) )
		throw DBNotExistsException();

	fs::directory_iterator end_iter;

	for( fs::directory_iterator it( fullPath ); it != end_iter; ++it ) {
		if( debug )
			cerr << it -> path().filename() << endl;

		Mat load = imread( it -> path().string() ); 

		samples.push_back( load );

		imshow( "Loading..", load );
		waitKey();
	}
}

/**
 * @brief	Writes the database to a file in the default directory
 */
void ObjectDatabase::save() {

}
