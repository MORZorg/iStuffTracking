/**
 * @file	database.cpp
 * @brief	Definition for Database class
 * @class	Database
 * @author	Mattia Rizzini
 * @version	0.1.3
 * @date	2013-07-14
 */

#include "database.h"

using namespace std;
using namespace cv;
using namespace IStuff;

namespace fs = boost::filesystem;

/**
 * @brief	Constructor
 * @details	If the name passed matches an existing DB it loads it,
 * 			otherwise it creates it 
 * @param[in] _dbName The name of the DB to be loaded
 * @param[in] imagesPath The position of the sample images from which
 * 			the descriptors are to be taken
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
 * @details	Given an image, searches for the descriptors in the database
 *			and returns a rectangle enclosing the matched object
 * @param[in] frame	The image to search into
 * @retval	An Object containing an association between the labels and the
 * 			areas in which every label is found
 * */
Object Database::match( Mat frame ) {
	if( debug )
		cerr << "Start matching\n";

	initModule_nonfree();

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
		cerr << "\tFrame descriptors and keypoints computed\n";

	Object matchingObject = Object();

	// I use the matcher already trained to perform a descriptor match
	/*FlannBasedMatcher matcher;

	// Matcher training
	matcher.add( descriptorDB );
	matcher.train();

	if( debug )
		cerr << "\tMatcher Trained\n";*/

	// Matches vector
	vector< vector< DMatch > > matches;

	matcher.knnMatch( frameDescriptors, matches, 2 );

	if( debug )
		cerr << "\tMatches found\n";
	
	// The knnMatch method returns the two best matches for every descriptor
	// I keep only the best and only when the distance of the very best is significantly
	// lower than the distance of the relatively worst match
	std::vector< DMatch > good_matches;

	for( int i = 0; i < (int)matches.size(); i++ )
		if( matches[ i ][ 0 ].distance < 0.8 * matches[ i ][ 1 ].distance )
				good_matches.push_back( matches[ i ][ 0 ] );

	// Prints out the good matching keypoints and draws them
	if( debug ) {
		for( int i = 0; i < good_matches.size(); i++ )
			cerr <<"\tGood match #" << i
					<< "\n\t\tqueryDescriptorIndex: " << good_matches[ i ].queryIdx
					<< "\n\t\ttrainDescriptorIndex: " << good_matches[ i ].trainIdx
					<< "\n\t\ttrainImageIndex: " << good_matches[ i ].imgIdx << " (" << labelDB[ good_matches[ i ].imgIdx ] << ")\n\n";

		Mat img_keypoints;
		drawKeypoints( frame, frameKeypoints, img_keypoints, Scalar::all( -1 ), DrawMatchesFlags::DEFAULT );

		string outsbra = "keypoints_sample/" + dbName + "Frame.jpg";

		imwrite( outsbra, img_keypoints );
	}

	// For every label in the database, extract the points and search homography mask
	for( int i = 0; i < labelDB.size(); i++ ) {
		vector< Point2f > labelPoints, framePoints;
		vector< Point2f > labelCorners;
		bool active = false;

		if( debug )
			cerr << "\tSearching mask for label " << labelDB[ i ] << endl;

		// If there is at least one good match in the actual label, mark the label
		// as active and add the points to the lists
		for( int j = 0; j < good_matches.size(); j++ )
			if( good_matches[ j ].imgIdx == i ) {
				active = true;
				labelPoints.push_back( keypointDB[ i ][ good_matches[ j ].trainIdx ].pt );
				framePoints.push_back( frameKeypoints[ good_matches[ j ].queryIdx ].pt );
			}

		if( debug )
			cerr << "\t\tFound " << labelPoints.size() << " keypoints in this mask\n";

		if( !active || labelPoints.size() < 4 ) {
			if( debug )
				cerr << "\t\t\tToo few keypoints, passing to next mask..\n";

			continue;
		}

		if( debug )
			cerr << "\t\tCalculating homography mask\n";

		// Calculate homography mask and add it to the matchingObject
		Mat H = findHomography( labelPoints, framePoints, CV_RANSAC );

		perspectiveTransform( cornersDB[ i ], labelCorners, H );

		// Debug drawing
		if( debug ) {
			Mat imgMatches = frame.clone();
			line( imgMatches, labelCorners[0], labelCorners[1], Scalar( 0, 255, 0 ), 4 );
			line( imgMatches, labelCorners[1], labelCorners[2], Scalar( 0, 255, 0 ), 4 );
			line( imgMatches, labelCorners[2], labelCorners[3], Scalar( 0, 255, 0 ), 4 );
			line( imgMatches, labelCorners[3], labelCorners[0], Scalar( 0, 255, 0 ), 4 );

			imwrite( "output_sample/" + labelDB[ i ] + ".jpg", imgMatches );
		}

		if( debug )
			cerr << "\t\tMask calculated, adding current label to output Object\n";

		matchingObject.setLabel( labelDB[ i ], labelCorners );
	}

	if( debug )
		cerr << "\n\tMatching done. Returning the object\n\n";

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
	
	/*boost::archive::binary_iarchive iarch( f );
	iarch >> labelDB;
	iarch >> keypointDB;
	iarch >> descriptorDB;
	iarch >> cornersDB;*/

	if( debug )
		cerr << "\tLoad successfull" << endl;

	matcher.add( descriptorDB );
	matcher.train();

	if( debug )
		cerr << "\tMatcher train successfull" << endl;
}

/**
 * @brief	Creates the database from the sample images
 * @details	Loaded the images contained in the argument path
 * 			generate a set of labels, detects the SIFT features
 * 			and descriptors for every sample and associate them to
 * 			every label, then trains a matcher with the descriptors
 * 			and save everything in the corresponding structures.
 * 			Also save the database into a file
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
	
	int labelCounter = 0;

	// For every image, calculate the descriptor, index them with a flann::Index and
	// add the Index to the map
	// As key the source image name
	for( fs::directory_iterator it( fullPath ); it != end_iter; ++it ) {
		if( debug ) {
			cerr << "\tTreating a new image: ";
			cerr << it -> path().filename() << endl;
		}
	
		load = imread( it -> path().string() );

		// Detect the keypoints in the actual image
		featureDetector -> detect( load, keypoints );

		if( debug )
			cerr << "\tFeatures detected" << endl;

		// Compute the 128 dimension SIFT descriptor at each keypoint detected
		// Each row in descriptors corresponds to the SIFT descriptor for each keypoint
		featureExtractor -> compute( load, keypoints, descriptors );

		if( debug )
			cerr << "\tDescriptors extracted\n"
	 			 << "\t\tSaving label, keypoints and sample corners\n";

		string labelName = dbName + "Label" + boost::lexical_cast<std::string>( labelCounter++ );

		labelDB.push_back( labelName ); 
		keypointDB.push_back( keypoints );

		descriptorDB.push_back( descriptors );

		// The corner of the image will be required to find the correspondent boundaries in the sample 
		vector< Point2f > sampleCorners( 4 );

		sampleCorners[ 0 ] = cvPoint( 0, 0 );
		sampleCorners[ 1 ] = cvPoint( load.cols, 0 );
		sampleCorners[ 2 ] = cvPoint( load.cols, load.rows );
		sampleCorners[ 3 ] = cvPoint( 0, load.rows );

		cornersDB.push_back( sampleCorners );

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

	// Now train the matcher
	// NOTE the descriptorDB is stored anyway because it is used to train a new matcher
	// after a Database load from file
	matcher.add( descriptorDB );
	matcher.train();

	// Now that the structures are filled, save them to a file for future usage
	//save();
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

	/*boost::archive::binary_oarchive oarch( f );
	oarch << labelDB;
	oarch << keypointDB;
	oarch << descriptorDB;
	oarch << cornersDB;*/
	
	if( debug )
		cerr << "\tSave successfull" << endl;
}
