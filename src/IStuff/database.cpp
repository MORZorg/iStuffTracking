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

	Mat img_object = imread("./image_sample/mail/mail.png", CV_LOAD_IMAGE_GRAYSCALE);
	//Mat img_scene = imread("./match_sample/sample.png", CV_LOAD_IMAGE_GRAYSCALE);
	Mat img_scene;
	cvtColor(frame, img_scene, CV_RGB2GRAY);

	//-- Step 1: Detect the keypoints using SURF Detector
	int minHessian = 400;

	SurfFeatureDetector detector( minHessian );

	std::vector<KeyPoint> keypoints_object, keypoints_scene;

	detector.detect( img_object, keypoints_object );
	detector.detect( img_scene, keypoints_scene );

	//-- Step 2: Calculate descriptors (feature vectors)
	SurfDescriptorExtractor extractor;

	Mat descriptors_object, descriptors_scene;

	extractor.compute( img_object, keypoints_object, descriptors_object );
	extractor.compute( img_scene, keypoints_scene, descriptors_scene );

	//-- Step 3: Matching descriptor vectors using FLANN matcher
	FlannBasedMatcher matcher;

	std::vector< std::vector<DMatch> > matches;
	matcher.knnMatch( descriptors_object, descriptors_scene, matches, 2 );

	std::vector< DMatch > good_matches;
	//Neares-Neighbour-Distance-Ratio
	std::vector<DMatch> NNDR_matches;
	double NNDR_ratio = 0.85;	
	for( unsigned int i = 0; i < matches.size(); i++ ) 
	{
		if( matches[ i ][ 0 ].distance <= NNDR_ratio * matches[ i ][ 1 ].distance ) 
		{
			good_matches.push_back( matches[ i ][ 0 ]);
		}
	}

	Mat img_matches;
	drawMatches( img_object, keypoints_object, img_scene, keypoints_scene,
			good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
			vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

	//-- Localize the object
	std::vector<Point2f> obj;
	std::vector<Point2f> scene;

	for( int i = 0; i < good_matches.size(); i++ )
	{
		//-- Get the keypoints from the good matches
		obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
		scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
	}

	Mat H = findHomography( obj, scene, CV_RANSAC );

	//-- Get the corners from the image_1 ( the object to be "detected" )
	std::vector<Point2f> obj_corners(4);
	obj_corners[0] = cvPoint(0,0); obj_corners[1] = cvPoint( img_object.cols, 0 );
	obj_corners[2] = cvPoint( img_object.cols, img_object.rows ); obj_corners[3] = cvPoint( 0, img_object.rows );
	std::vector<Point2f> scene_corners(4);

	perspectiveTransform( obj_corners, scene_corners, H);

	//-- Draw lines between the corners (the mapped object in the scene - image_2 )
	line( img_matches, scene_corners[0] + Point2f( img_object.cols, 0), scene_corners[1] + Point2f( img_object.cols, 0), Scalar(0, 255, 0), 4 );
	line( img_matches, scene_corners[1] + Point2f( img_object.cols, 0), scene_corners[2] + Point2f( img_object.cols, 0), Scalar( 0, 255, 0), 4 );
	line( img_matches, scene_corners[2] + Point2f( img_object.cols, 0), scene_corners[3] + Point2f( img_object.cols, 0), Scalar( 0, 255, 0), 4 );
	line( img_matches, scene_corners[3] + Point2f( img_object.cols, 0), scene_corners[0] + Point2f( img_object.cols, 0), Scalar( 0, 255, 0), 4 );

	//-- Show detected matches
	imshow( "Good Matches & Object detection", img_matches );

	Object matchingObject;
	matchingObject.setLabel("prova", scene_corners);

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
