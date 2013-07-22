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

	Object matchingObject;

	// Convert frame in grayscale
	Mat scene = frame;

	// Calculate SURF keypoints and descriptors
	Ptr< FeatureDetector > featureDetector = FeatureDetector::create( "SURF" );
	Ptr< DescriptorExtractor > featureExtractor = DescriptorExtractor::create( "SURF" );

	vector< KeyPoint > sceneKeypoints;
	Mat sceneDescriptors;

	featureDetector -> detect( scene, sceneKeypoints );
	featureExtractor -> compute( scene, sceneKeypoints, sceneDescriptors );

	if( debug )
		cerr << "\t\tFrame keypoints and descriptors computed\n";

	// Matching..
	vector< vector< DMatch > > matches;
	vector< DMatch > goodMatches;

	matcher.knnMatch( sceneDescriptors, matches, 2 );

	if( debug )
		cerr << "\t\t" << matches.size() << " matches found, start filtering the good ones\n";

	double NNDRRatio = 0.6;

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
					<< "\n\t\tsampleImageIndex: " << goodMatches[ i ].imgIdx << ")\n\n";

		Mat imgKeypoints;
		drawKeypoints( scene, sceneKeypoints, imgKeypoints, Scalar::all( -1 ), DrawMatchesFlags::DEFAULT );

		string outsbra = "keypoints_sample/" + dbName + "Frame.jpg";

		imwrite( outsbra, imgKeypoints );
	}

	// I consider only the sample with the biggest number of matches
	vector< int > bestSample( labelDB.size() + 1, 0 );

	for( vector< DMatch >::iterator m = goodMatches.begin(); m != goodMatches.end(); m++ )
		bestSample[ ( *m ).imgIdx ]++;

	int maxSample = 1;

	for( int i = 2; i < bestSample.size(); i++ )
		if( bestSample[ i ] > bestSample[ maxSample ] )
			maxSample = i;

	if( debug )
		cerr << "\t\tBest sample is #" << maxSample << endl;

	// Analyze the keypoints found for the sample to estimate homography and apply that to the
	// labels associated to the sample
	vector< Point2f > samplePoints, scenePoints;
	vector< Point2f > sampleCorners;

	for( int i = 0; i < goodMatches.size(); i++ )
		if( goodMatches[ i ].imgIdx == maxSample ) {
			samplePoints.push_back( keypointDB[ maxSample ][ goodMatches[ i ].trainIdx ].pt );
			scenePoints.push_back( sceneKeypoints[ goodMatches[ i ].queryIdx ].pt );
		}

	// Calculate homography mask, apply transformation to the label points and add the labels to the object 
	Mat H = findHomography( samplePoints, scenePoints, CV_RANSAC );

	perspectiveTransform( labelDB[ maxSample ], sampleCorners, H );

	for( int i = 0; i < sampleCorners.size(); i++ ) {
		matchingObject.setLabel( labelDB[ maxSample ][ i ].name, sampleCorners[ i ], labelDB[ maxSample ][ i ].color );
	}

	if( debug )
		cerr << "\n\tMatching done. Returning the object\n\n";

	return matchingObject;
	
}

/**
 * @brief	Creates the database from the sample images
 * @details	Loaded the images contained in the argument path
 * 			generate a set of labels, detects the SURF features
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

	// SURF detector and extractor
	Ptr< FeatureDetector > featureDetector = FeatureDetector::create( "SURF" );
	Ptr< DescriptorExtractor > featureExtractor = DescriptorExtractor::create( "SURF" );

	// Temporary containers
	Mat load, descriptors;
	vector< KeyPoint > keypoints;

	// Random color generator for label coloring
	boost::mt19937 rng( time( 0 ) );
	boost::uniform_int<> colorRange( 0, 255 );
	boost::variate_generator< boost::mt19937, boost::uniform_int<> > color( rng, colorRange );

	// For every image generate a new label and save the various informations
	for( fs::directory_iterator it( fullPath ); it != end_iter; ++it ) {
		fs::path extension = fs::extension( it -> path() );

		if( !( extension.string() == "jpg" || extension.string() == "png" ) ) {
			if( debug )
				cerr << "\tNot an image, skipping..\n";

			continue;
		}

		if( debug ) {
			cerr << "\tTreating a new image: ";
			cerr << it -> path().filename() << endl;
		}
	
		ifstream loadLabels( it -> path().stem().string() + ".lbl", ios::in );

		load = imread( it -> path().string() );

		if( !load.data || loadLabels.fail() )
			throw DBCreationException();

		// Detect the keypoints in the actual image
		featureDetector -> detect( load, keypoints );

		if( debug )
			cerr << "\tFeatures detected" << endl;

		// Compute SURF descriptors
		featureExtractor -> compute( load, keypoints, descriptors );

		if( debug )
			cerr << "\tDescriptors extracted\n";

		string labelName = it -> path().stem().string();

		// Read the labels associated to this image from the .lbl file
		vector< Label > localLabels;
		string name, x, y;

		while( loadLabels >> name ) {
			loadLabels >> x >> y;

			localLabels.push_back( Label( name, Point2f( ::atof( ( x ).c_str() ), ::atof( ( y ).c_str() ) ), Scalar( color(), color(), color() ) ) );
		}

		labelDB.push_back( localLabels );

		if( debug )
			cerr << "\tLabel read\n";

		keypointDB.push_back( keypoints );

		descriptorDB.push_back( descriptors );

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
 * @brief	Load existing database and fill the map 
 */
void Database::load() {
	/*if( debug )
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

		//cornersDB.push_back( temp );

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
		cerr << "\tMatcher trained successfully" << endl;*/
}

/**
 * @brief	Writes the database to a file in the default directory 
 */
void Database::save() {
	/*if( debug )
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
		cerr << "\tSave successfull" << endl;*/
}
