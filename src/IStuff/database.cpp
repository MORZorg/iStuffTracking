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

	// FLANNMatcher, for his usage is not needed the instantiation of an Index
	FlannBasedMatcher matcher;

	// Matcher training
	matcher.add( descriptorDB );
	matcher.train();

	if( debug )
		cerr << "\tMatcher Trained\n";

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
		
		/*if( debug ) {
			for( int j = 0; j < labelPoints.size(); j++ )
				cerr << "\t\t\tSource points" << j << ": " << labelPoints[ j ] << endl
					 << "\t\t\tFrame points" << j << ": " << framePoints[ j ] << endl;

			cerr << endl << H << endl;
		}*/

		perspectiveTransform( cornersDB[ i ], labelCorners, H );

		if( debug )
			for( int j = 0; j < labelCorners.size(); j++ )
				cerr << "\t\t\tCoordinate ori " << j << ": " << cornersDB[ i ][ j ] << endl
					 << "\t\t\tCoordinate per " << j << ": " << labelCorners[ j ] << endl;

		// Debug drawing
		if( debug ) {
			Mat imgMatches = frame.clone();
			line( imgMatches, labelCorners[0], labelCorners[1], Scalar( 0, 255, 0 ), 4 );
			line( imgMatches, labelCorners[1], labelCorners[2], Scalar( 0, 255, 0 ), 4 );
			line( imgMatches, labelCorners[2], labelCorners[3], Scalar( 0, 255, 0 ), 4 );
			line( imgMatches, labelCorners[3], labelCorners[0], Scalar( 0, 255, 0 ), 4 );

			/*for( int j = 0; j < framePoints.size(); j++ )
				circle( imgMatches, framePoints[ i ], 3, Scalar( 255, 0, 0 ) );*/

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
			cerr << "\tDescriptors extracted\n"
	 			 << "\t\tSaving label, keypoints, descriptors and sample corners\n";

		string labelName = dbName + "Label" + boost::lexical_cast<std::string>( labelCounter++ );

		labelDB.push_back( labelName ); 
		keypointDB.push_back( keypoints );
		descriptorDB.push_back( descriptors) ;

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

	// Now that the descriptorDB structure is generated, i serialize it for future usage
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

	boost::archive::binary_oarchive oarch( f );
	oarch << descriptorDB;
	
	if( debug )
		cerr << "\tSave successfull" << endl;
}
