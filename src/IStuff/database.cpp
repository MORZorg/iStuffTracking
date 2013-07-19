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
 * @brief Another attempt of match
 */
Object Database::match( Mat frame ) {
	if( debug )
		cerr << "Start matching\n";

	Object matchingObject;

	// Convert frame in grayscale
	Mat scene;
	cvtColor( frame, scene, CV_RGB2GRAY );

	// Calculate SURF keypoints and descriptors
	int minHessian = 400;

	SurfFeatureDetector featureDetector( minHessian );
	SurfDescriptorExtractor featureExtractor;

	vector< KeyPoint > sceneKeypoints;
	Mat sceneDescriptors;

	featureDetector.detect( scene, sceneKeypoints );
	featureExtractor.compute( scene, sceneKeypoints, sceneDescriptors );

	if( debug )
		cerr << "\t\tFrame keypoints and descriptors computed\n";

	// Matching..
	vector< vector< DMatch > > matches;
	vector< DMatch > goodMatches;

	matcher.knnMatch( sceneDescriptors, matches, 2 );

	if( debug )
		cerr << "\t\t" << matches.size() << " matches found, start filtering the good ones\n";

	double NNDRRatio = 0.85;

	for( int i = 0; i < matches.size(); i++ )
		if( matches[ i ][ 0 ].distance <= NNDRRatio * matches[ i ][ 1 ].distance )
			goodMatches.push_back( matches[ i ][ 0 ] );

	if( debug )
		cerr << "\t\t" << goodMatches.size() << " good matches found, starting object localization\n";
	
	// Object localization
	// Prints out the good matching keypoints and draws them
	if( debug ) {
		for( int i = 0; i < goodMatches.size(); i++ )
			cerr <<"\tGood match #" << i
					<< "\n\t\tsceneDescriptorIndex: " << goodMatches[ i ].queryIdx
					<< "\n\t\tsampleDescriptorIndex: " << goodMatches[ i ].trainIdx
					<< "\n\t\tsampleImageIndex: " << goodMatches[ i ].imgIdx << " (" << labelDB[ goodMatches[ i ].imgIdx ] << ")\n\n";

		Mat imgKeypoints;
		drawKeypoints( scene, sceneKeypoints, imgKeypoints, Scalar::all( -1 ), DrawMatchesFlags::DEFAULT );

		string outsbra = "keypoints_sample/" + dbName + "Frame.jpg";

		imwrite( outsbra, imgKeypoints );
	}

	// For every label analyze the matches and process a
	for( int i = 0; i < labelDB.size(); i++ ) {
		vector< Point2f > labelPoints, scenePoints;
		vector< Point2f > labelCorners;
		bool active = false;

		if( debug )
			cerr << "\tSearching mask for label " << labelDB[ i ] << endl;

		// If there is at least one good match in the actual label, mark the label
		// as active and add the points to the lists
		for( int j = 0; j < goodMatches.size(); j++ )
			if( goodMatches[ j ].imgIdx == i ) {
				active = true;
				labelPoints.push_back( keypointDB[ i ][ goodMatches[ j ].trainIdx ].pt );
				scenePoints.push_back( sceneKeypoints[ goodMatches[ j ].queryIdx ].pt );
			}

		if( debug )
			cerr << "\t\tFound " << labelPoints.size() << " keypoints in this label\n";

		if( !active || labelPoints.size() < 4 ) {
			if( debug )
				cerr << "\t\t\tToo few keypoints, passing to next label..\n";

			continue;
		}

		if( debug )
			cerr << "\t\tCalculating homography mask\n";

		// Calculate homography mask and add it to the matchingObject
		Mat H = findHomography( labelPoints, scenePoints, CV_RANSAC );

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
 * @brief	Search for descriptors matching in passed frame
 * @details	Given an image, searches for the descriptors in the database
 *			and returns a rectangle enclosing the matched object
 * @param[in] frame	The image to search into
 * @retval	An Object containing an association between the labels and the
 * 			areas in which every label is found
 * */
/*Object Database::match( Mat frame ) {
	if( debug )
		cerr << "Start matching\n";

	//initModule_nonfree();

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
		cerr << "\tFrame descriptors (" << frameDescriptors.rows << ") and keypoints (" << frameKeypoints.size() << ") computed\n";

	Object matchingObject = Object();

	// I use the matcher already trained to perform a descriptor match
	vector< vector< DMatch > > matches;

	matcher.knnMatch( frameDescriptors, matches, 2 );

	if( debug )
		cerr << "\tMatches found\n";
	
	// The knnMatch method returns the two best matches for every descriptor
	// I keep only the best and only when the distance of the very best is significantly
	// lower than the distance of the relatively worst match
	std::vector< DMatch > good_matches;

	for( int i = 0; i < (int)matches.size(); i++ )
		if( matches[ i ][ 0 ].distance < 0.9 * matches[ i ][ 1 ].distance )
				good_matches.push_back( matches[ i ][ 0 ] );

	// Prints out the good matching keypoints and draws them
	if( debug ) {
		for( int i = 0; i < good_matches.size(); i++ )
			cerr <<"\tGood match #" << i
					<< "\n\t\tframeDescriptorIndex: " << good_matches[ i ].queryIdx
					<< "\n\t\tsampleDescriptorIndex: " << good_matches[ i ].trainIdx
					<< "\n\t\tsampleImageIndex: " << good_matches[ i ].imgIdx << " (" << labelDB[ good_matches[ i ].imgIdx ] << ")\n\n";

		Mat img_keypoints;
		drawKeypoints( frame, frameKeypoints, img_keypoints, Scalar::all( -1 ), DrawMatchesFlags::DEFAULT );

		string outsbra = "keypoints_sample/" + dbName + "Frame.jpg";

		imwrite( outsbra, img_keypoints );
	}

	// For every label analyze the matches and process a
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
			cerr << "\t\tFound " << labelPoints.size() << " keypoints in this label\n";

		if( !active || labelPoints.size() < 4 ) {
			if( debug )
				cerr << "\t\t\tToo few keypoints, passing to next label..\n";

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
}*/

/**
 * @brief	Load existing database and fill the map 
 */
void Database::load() {
	if( debug )
		cerr << "Loading database\n";

	string dbFileName = dbPath + dbName;

	ifstream label( ( dbFileName + "label.sbra" ).c_str(), ios::binary );
	ifstream desc( ( dbFileName + "desc.sbra" ).c_str(), ios::binary );
	
	ifstream kp( ( dbFileName + "kp.sbra" ).c_str(), ios::in );
	ifstream corn( ( dbFileName + "corn.sbra" ).c_str(), ios::in );

	if( label.fail() || kp.fail() || desc.fail() || corn.fail() )
		throw DBLoadingException();
	else if( debug )
		cerr << "\tLoading from " << dbFileName << endl;

	boost::archive::binary_iarchive labelarch( label );
	labelarch >> labelDB;

	if( debug )
		cerr << "\t\tLabels loaded\n";

	boost::archive::binary_iarchive descarch( desc );
	descarch >> descriptorDB;

	if( debug )
		cerr << "\t\tDescriptors loaded\n";

	/*boost::archive::binary_iarchive kparch( kp );
	kparch >> keypointDB;

	boost::archive::binary_iarchive cornarch( corn );
	cornarch >> cornersDB;*/

	// Very very awful loading
	// keypointDB. Discard the first line as I expect it to be a Label marker
	string line;
	kp >> line;

	while( true ) {
		vector< KeyPoint > temp = vector< KeyPoint >();
		string x, y, size, angle, response, octave, class_id;

		if( debug )
			cerr << "\t\t\tLine " << line << endl;

		while( class_id.compare( "Label" ) && kp >> x ) {
			if( !x.compare( "Label" ) ) {
				line = x;
				break;
			}

			kp >> y >> size >> angle >> response >> octave >> class_id;

			temp.push_back(	KeyPoint(
					::atof( ( x ).c_str() ),
					::atof( ( y ).c_str() ),
					::atof( ( size ).c_str() ),
					::atof( ( angle ).c_str() ),
					::atof( ( response ).c_str() ),
					atoi( ( octave ).c_str() ),
					atoi( ( class_id ).c_str() ) ) );
			
			if( debug )
				cerr << "\t\t\tPoint " << temp[ temp.size() - 1 ].pt << endl;
		} 

		keypointDB.push_back( temp );

		if( kp.eof() )
			break;
	}

	if( debug )
		cerr << "\t\tKeypoints loaded\n";

	corn >> line;

	// cornerDB. Much easier due to the content nature
	while( true ) {
		vector< Point2f > temp = vector< Point2f >();
		string x, y;

		if( debug )
			cerr << "\t\t\tLine " << line << endl;

		while( y.compare( "Corners" ) && corn >> x ) {
			if( !x.compare( "Corners" ) ) {
				line = x;
				break;
			}

			corn >> y;

			temp.push_back(	Point2f(
					::atof( ( x ).c_str() ),
					::atof( ( y ).c_str() ) ) );
			
			if( debug )
				cerr << "\t\t\tPoint " << temp[ temp.size() - 1 ] << endl;
		}

		cornersDB.push_back( temp );

		if( corn.eof() )
			break;
	}

	if( debug )
		cerr << "\t\tCorners loaded\n";

	if( debug )
		cerr << "\tLoad successfull" << endl;

	matcher.add( descriptorDB );
	matcher.train();

	if( debug )
		cerr << "\tMatcher trained successfully" << endl;
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
	//Ptr< FeatureDetector > featureDetector = FeatureDetector::create( "SURF" );
	//Ptr< DescriptorExtractor > featureExtractor = DescriptorExtractor::create( "SURF" );
	
	// SURF detector and extractor
	int minHessian = 400;
	
	SurfFeatureDetector featureDetector( minHessian );
	SurfDescriptorExtractor featureExtractor;
	
	// Temporary containers
	Mat load, descriptors;
	vector< KeyPoint > keypoints;

	// If there are no images in the given directory then throw an exception
	/*int file_count = std::count_if(
			fs::directory_iterator( fullPath ),
			fs::directory_iterator(),
			bind( static_cast< bool(*)( const fs::path& ) > ( fs::is_regular_file ), 
			bind( &fs::directory_entry::path, boost::lambda::_1 ) ) );	

	if( file_count == 0 )
		throw DBCreationException();*/
	
	int labelCounter = 0;

	// For every image, calculate the descriptor, index them with a flann::Index and
	// add the Index to the map
	// As key the source image name
	for( fs::directory_iterator it( fullPath ); it != end_iter; ++it ) {
		if( debug ) {
			cerr << "\tTreating a new image: ";
			cerr << it -> path().filename() << endl;
		}
	
		load = imread( it -> path().string(), CV_LOAD_IMAGE_GRAYSCALE );

		// Detect the keypoints in the actual image
		featureDetector.detect( load, keypoints );

		if( debug )
			cerr << "\tFeatures detected" << endl;

		// Compute the 128 dimension SIFT descriptor at each keypoint detected
		// Each row in descriptors corresponds to the SIFT descriptor for each keypoint
		featureExtractor.compute( load, keypoints, descriptors );

		if( debug )
			cerr << "\tDescriptors extracted\n"
	 			 << "\t\tSaving label, keypoints and sample corners\n";

		string labelName = dbName + "Label" + boost::lexical_cast< string >( labelCounter++ );

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
	save();
}

/**
 * @brief	Writes the database to a file in the default directory 
 */
void Database::save() {
	if( debug )
		cerr << "Saving the created database" << endl;

	string dbFileName = dbPath + dbName;

	// Empty file for existence check
	ofstream ex( ( dbFileName + ".sbra" ).c_str(), ios::out );
	ex << "SBRA!";
	ex.close();

	ofstream label( ( dbFileName + "label.sbra" ).c_str(), ios::binary );
	ofstream desc( ( dbFileName + "desc.sbra" ).c_str(), ios::binary );

	ofstream kp( ( dbFileName + "kp.sbra" ).c_str(), ios::out );
	ofstream corn( ( dbFileName + "corn.sbra" ).c_str(), ios::out );

	if( label.fail() || kp.fail() || desc.fail() || corn.fail() )
		throw DBSavingException();
	else if( debug )
		cerr << "\tSaving to " << dbFileName << endl;

	boost::archive::binary_oarchive labelarch( label );
	labelarch << labelDB;

	boost::archive::binary_oarchive descarch( desc );
	descarch << descriptorDB;

	/*boost::archive::binary_oarchive kparch( kp );
	kparch << keypointDB;

	boost::archive::binary_oarchive cornarch( corn );
	cornarch << cornersDB;*/

	// Very very awful saving
	// keypointDB
	for( vector< vector< KeyPoint > >::iterator it = keypointDB.begin(); it != keypointDB.end(); it++ ) {
		kp << "Label" << endl;

		for( vector< KeyPoint >::iterator jt = ( *it ).begin(); jt != ( *it ).end(); jt++ )
			kp << ( *jt ).pt.x << " " << ( *jt ).pt.y << " " << ( *jt ).size << " " << ( *jt ).angle << " " << ( *jt ).response << " " << ( *jt ).octave << " " << ( *jt ).class_id << endl;
	}
	
	// cornerDB
	for( vector< vector< Point2f > >::iterator it = cornersDB.begin(); it != cornersDB.end(); it++ ) {
		corn << "Corners" << endl;
		for( vector< Point2f >::iterator jt = ( *it ).begin(); jt != ( *it ).end(); jt++ )
			corn << ( *jt ).x << " " << ( *jt ).y << endl;
	}
	
	label.close();
	desc.close();
	kp.close();
	corn.close();

	if( debug )
		cerr << "\tSave successfull" << endl;
}
