/**
 * @file	object_database.cpp
 * @brief	Definition for Database class
 * @class	Database
 * @author	Mattia Rizzini
 * @version	0.1.2
 * @date	2013-07-14
 */

#include "database.h"

using namespace std;
using namespace cv;
using namespace IStuff;

namespace fs = boost::filesystem;

/**
 * @brief	Constructor
 * @details	If the name passed matches an existing DB it loads it, \
 			otherwise it creates it 
 * @param[in] _dbName	The name of the DB to be loaded
 * @param[in] imagesPath	The position of the images from which the descriptors are to be taken
 */
Database::Database( string _dbName, string imagesPath ) :
	dbPath( "database/" ), dbName( _dbName )
{
	// Check for database existence
	string dbFileName = dbPath + dbName + ".sbra";
	ifstream file_check( dbFileName.c_str(), ios::binary );

	if( !file_check ) {
		if( debug )
			cerr << _dbName << " doesn't exists. Start creating it" << endl;

		try {
			build( imagesPath );
		} catch( DBCreationException& e ) {
			throw e;
		}
	} else {
		if( debug )
			cerr << "Opening DB " << _dbName << endl;

		load();
	}
}

/**
 * @brief	Create DB Constructor
 * @details	If the name passed matches an existing DB throws an exception, \
 			otherwise generates the structures and fill them performing \
			a recognize over the images in the directory passed as second argument.
 * @param[in] _dbName		The name of the DB to be created
 * @param[in] imagesPath	The position of the images from which the descriptors are to be taken
 */
/*Database::Database( string _dbName, string imagesPath ) :
	dbPath( "database/" ), dbName( _dbName )
{
	// Add existence check
	if( false )
		throw DBExistsException();

	if( debug )
		cerr << _dbName << " doesn't exists. Start creating it" << endl;

	build( imagesPath );
}*/

/**
 * @brief	Destructor
 */
Database::~Database() {

}

/**
 * @brief	Search for descriptors matching in passed frame
 * @details	Given an image, searches for the descriptors in the database \
 			and returns a rectangle enclosing the matched object
 * @param[in] frame	The image to search into
 * @retval	A Rect which encloses the object in the frame reference system
 			or an empty Rect if nothing is found
 */
Object Database::match( Mat frame ) {
	Object emptyOne = Object();

	return emptyOne;
}

/**
 * @brief	Load existing database and fill the kd-tree containing descriptors
 * param[in] dbName	The name of the DB to load
 */
void Database::load() {
	if( debug )
		cerr << "Loading database" << endl;

	string dbFileName = dbPath + dbName + ".sbra";
	ifstream f( dbFileName.c_str(), ios::binary );
	
	if( f.fail() ) {
		if( debug )
			cerr << "\tError in file stream opening" << endl;
		return;
	} else if( debug )
		cerr << "\tLoading from " << dbFileName << endl;

	boost::archive::binary_iarchive iarch( f );
	iarch >> descriptorDB;

	if( debug )
		cerr << "\tLoad successfull" << endl;
}

/**
 * @brief	Create a new kd-tree and save it to a file
 * @details	Loaded the images contained in the argument path, \
 			detects the SIFT features, turns them in descriptors \
 			and save these to the kd-tree representing the database. \
 			Also saves the database into a file
 * @param[in] imagesPath	The path containing the source images
 */
void Database::build( string imagesPath ) {
	// Load images using boost to retrieve filenames
	//vector< Mat > samples = vector< Mat >();	
	
	fs::path fullPath = fs::system_complete( fs::path( imagesPath ) );

	if( debug )
		cerr << "Loading images from " << fullPath << endl;

	if( !fs::exists( fullPath ) || !fs::is_directory( fullPath ) )
		throw DBCreationException(); 

	fs::directory_iterator end_iter;

	// SIFT detector and extractor
	Ptr< FeatureDetector > featureDetector = FeatureDetector::create( "SIFT" );
	Ptr< DescriptorExtractor > featureExtractor = DescriptorExtractor::create( "SIFT" );
	
	// Temporary containers
	Mat load, descriptors;
	vector< KeyPoint > keypoints;

	// If there are no images in the given directory then throw an exception
	int file_count = std::count_if(
			fs::directory_iterator( fullPath ),
			fs::directory_iterator(),
			bind( static_cast< bool(*)( const fs::path& ) > ( fs::is_regular_file ), 
			bind( &fs::directory_entry::path, boost::lambda::_1 ) ) );	

	if( file_count == 0 ) {
		throw DBCreationException(); 
	}
	
	// For every image, calculate the keypoints and add them to a map
	// As key I use the source image name
	for( fs::directory_iterator it( fullPath ); it != end_iter; ++it ) {
		if( debug ) {
			cerr << "\tTreating a new image: ";
			cerr << it -> path().filename() << endl;
		}
	
		load = imread( it -> path().string() );

		if( debug ) {
			cerr << "\tImage loaded. Showing.." << endl;

			//namedWindow( "Loaded image" );
			//imshow( "Loaded image", load );
		}

		// Detect the keypoints in the actual image
		featureDetector -> detect( load, keypoints );

		if( debug )
			cerr << "\tFeatures detected" << endl;

		// Compute the 128 dimension SIFT descriptor at each keypoint detected
		// Each row in descriptors corresponds to the SIFT descriptor for each keypoint
		featureExtractor -> compute( load, keypoints, descriptors );

		if( debug )
			cerr << "\tDescriptors extracted" << endl;

		descriptorDB.insert( pair< Label, Mat >( it -> path().filename().string(), descriptors ) );

		if( debug )
			cerr << "\tDataBase updated" << endl;

		// Draw the keypoints for debug purposes
		if( debug ) {
			Mat outputImage;
			drawKeypoints( load, keypoints, outputImage, Scalar( 255, 0, 0 ), DrawMatchesFlags::DEFAULT );

			string outsbra = "keypoints_sample/" + it -> path().filename().string();
			cerr << "\tShowing image " << outsbra << endl;

			imwrite( outsbra, outputImage );
			//imshow( "Loaded keypoints", outputImage );
			//waitKey();

			cerr << "\tReiterating" << endl << endl;
		}

		//samples.push_back( load );

		//imshow( "Loading..", load );
		//waitKey();
	}

	// Now that the descriptorDB structure is generated, i serialize it for future usage
	save();
}

/**
 * @brief	Writes the database to a file in the default directory
 */
void Database::save() {
	if( debug )
		cerr << "Saving the created database" << endl;

	string dbFileName = dbPath + dbName + ".sbra";
	ofstream f( dbFileName.c_str(), ios::binary );
	
	if( f.fail() ) {
		if( debug )
			cerr << "\tError in file stream opening" << endl;
		return;
	} else if( debug )
		cerr << "\tSaving to " << dbFileName << endl;

	boost::archive::binary_oarchive oarch( f );
	oarch << descriptorDB;

	if( debug )
		cerr << "\tSave successfull" << endl;
}
