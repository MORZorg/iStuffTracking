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
	if( debug )
		cerr << "Start matching\n";

	// Compute descriptors of frame
	// SIFT detector and extractor
	Ptr< FeatureDetector > featureDetector = FeatureDetector::create( "SIFT" );
	Ptr< DescriptorExtractor > featureExtractor = DescriptorExtractor::create( "SIFT" );
	
	// Temporary containers
	Mat frameDescriptors;
	vector< KeyPoint > frameKeypoints;

	// Detect the keypoints in the actual image
	featureDetector -> detect( frame, frameKeypoints );

	// Compute the 128 dimension SIFT descriptor at each keypoint detected
	// Each row in descriptors corresponds to the SIFT descriptor for each keypoint
	featureExtractor -> compute( frame, frameKeypoints, frameDescriptors );

	if( debug )
		cerr << "\tFrame descriptors computed\n";

	Object matchingObject = Object();

	// FLANNMatcher, for his usage is not needed the instantiation of an Index
	FlannBasedMatcher matcher;

	// Temporary object for multiple matches
	vector< DMatch > matches;

	// For every entry in the map I try to find good matches and draw it (debug only)
	for( map< Label, Mat >::iterator it = descriptorDB.begin(); it != descriptorDB.end(); it++ ) {
		if( debug )
			cerr << "\tMatching over label " << it -> first << endl;

		matcher.match( frameDescriptors, it -> second, matches );

		// Find the good matches throught distance analysis
		double max_dist = 0, min_dist = 100;

		for( int i = 0; i < frameDescriptors.rows; i++ ) {
			double dist = matches[ i ].distance;

		    min_dist = ( dist < min_dist ? dist : min_dist );
		    max_dist = ( dist > min_dist ? dist : max_dist );
		}

		if( debug )
			cerr << "\tMaximum match distance is " << max_dist << endl
 				 << "\tMinimum match distance is " << min_dist << endl;

		// Consider good matches only those in which the distance is less than 3/2 * min_dist
		std::vector< DMatch > good_matches;

		for( int i = 0; i < frameDescriptors.rows; i++ )
			if( matches[ i ].distance <= 1.5 * min_dist )
				good_matches.push_back( matches[ i ] );

		// Draw the good matches retrieving the sample images from the original folder
		if( debug ) {
			// Retrieve informations about the original sample
			Mat originalSample = imread( "image_sample/" + it -> first );
			vector< KeyPoint > sampleKeypoints;

			// Detect the keypoints in the actual image
			featureDetector -> detect( originalSample, sampleKeypoints );

			// Draw stuff
			Mat img_matches;

			drawMatches( frame, frameKeypoints, originalSample, sampleKeypoints, good_matches, img_matches, Scalar::all(-1), Scalar::all(-1), vector< char >(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

			// Print
			string matchesPath = "output_sample/" + it -> first;
			imwrite( matchesPath, img_matches );

			// Print out something
			for( int i = 0; i < good_matches.size(); i++ ) {
				printf( "\tGood Match [%d] Keypoint 1: %d  -- Keypoint 2: %d  \n", i, good_matches[i].queryIdx, good_matches[i].trainIdx );
			}
		}
	}

	return matchingObject;
}

/**
 * @brief	Load existing database and fill the map 
 */
void Database::load() {
	if( debug )
		cerr << "Loading database\n";

	string dbFileName = dbPath + dbName + ".sbra";
	ifstream f( dbFileName.c_str(), ios::binary );

	if( f.fail() )
		throw DBLoadingException();
	else if( debug )
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
	// Use boost to iterate over a directory content
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

	if( file_count == 0 )
		throw DBCreationException(); 
	
	// For every image, calculate the descriptor, index them with a flann::Index and
	// add the Index to the map
	// As key the source image name
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

		//descriptorDB.insert( pair< Label, flann::Index >( it -> path().filename().string(), newIndex ) );
		descriptorDB[ it -> path().filename().string() ] = descriptors; 

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

	if( f.fail() )
		throw DBSavingException();
	else if( debug )
		cerr << "\tSaving to " << dbFileName << endl;

	boost::archive::binary_oarchive oarch( f );
	oarch << descriptorDB;
	
	if( debug )
		cerr << "\tSave successfull" << endl;
}
