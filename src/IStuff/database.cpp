/**
 * @file	database.cpp
 * @brief	Definition for Database class
 * @class	IStuff::Database
 * @author	Mattia Rizzini
 * @version	0.1.4
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
 * @details	Given an image, searches for descriptor matches in the database
 *			and returns an object containing the estimated label positions
 * @param[in] frame	The image to search into
 * @retval	An Object containing an association between the labels and the
 * 			positions in which every label is found
 * */
Object Database::match( Mat scene ) {
	if( debug )
		cerr << "Start matching\n";

	Object matchingObject;

	// Calculate SIFT keypoints and descriptors
	Ptr< FeatureDetector > featureDetector = FeatureDetector::create( "SIFT" );
	Ptr< DescriptorExtractor > featureExtractor = DescriptorExtractor::create( "SIFT" );

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
		cerr << "\tStart searching for the best sample\n";

	// I consider only the sample with the biggest number of matches (the index will be contained in maxSample)
	vector< int > bestSample( labelDB.size(), 0 );

	for( vector< vector< DMatch > >::iterator m = matches.begin(); m != matches.end(); m++ )
		bestSample[ ( *m )[0].imgIdx ]++;

	int maxSample = 0;

	for( int i = 1; i < bestSample.size(); i++ )
		if( bestSample[ i ] > bestSample[ maxSample ] )
			maxSample = i;

	if( debug )
		cerr << "\tBest sample is #" << maxSample << endl;

	// Keep only the matches with a significant difference in distance between the two nearest neighbours and regarding the best sample
	if( debug )
		cerr << "\t\t" << matches.size() << " matches found, start filtering the good ones\n";

	double NNDRRatio = 0.6;

	for( int i = 0; i < matches.size(); i++ )
		if( matches[ i ][ 0 ].imgIdx == maxSample && matches[ i ][ 0 ].distance <= NNDRRatio * matches[ i ][ 1 ].distance )
			goodMatches.push_back( matches[ i ][ 0 ] );

	if( debug )
		cerr << "\t\t" << goodMatches.size() << " good matches found, starting object localization\n";
	
	// Object localization
	// Prints out the good matching keypoints and draws them for debug
	if( debug ) {
		for( int i = 0; i < goodMatches.size(); i++ )
			cerr <<"\tGood match #" << i
					<< "\n\t\tsceneDescriptorIndex: " << goodMatches[ i ].queryIdx
					<< "\n\t\tsampleDescriptorIndex: " << goodMatches[ i ].trainIdx
					<< "\n\t\tsampleImageIndex: " << goodMatches[ i ].imgIdx << "\n\n";

		Mat imgKeypoints;
		drawKeypoints( scene, sceneKeypoints, imgKeypoints, Scalar::all( -1 ), DrawMatchesFlags::DEFAULT );

		string outsbra = "keypoints_sample/" + dbName + "Frame.jpg";

		imwrite( outsbra, imgKeypoints );
	}

	// Analyze the keypoints found for the sample to estimate homography and apply a perspectiveTransform
	// to the labels associated to that sample
	vector< Point2f > samplePoints, scenePoints;

	for( int i = 0; i < goodMatches.size(); i++ )
		if( goodMatches[ i ].imgIdx == maxSample ) {
			samplePoints.push_back( keypointDB[ maxSample ][ goodMatches[ i ].trainIdx ].pt );
			scenePoints.push_back( sceneKeypoints[ goodMatches[ i ].queryIdx ].pt );
		}

	// Given a proper database, this threshold looks definitely optimal
	int matchThreshold = 20;

	//if( debug )
		cerr << "\t" << samplePoints.size() << " definitely good matches found\n";

	if( samplePoints.size() < matchThreshold ) {
		if( debug )
			cerr << "\tToo few keypoints, exiting..\n";

		return matchingObject;
	}

	// Calculate homography mask, apply transformation to the label points and add the labels to the object 
	// However, if the number of outliers found is too high, an error is given
	vector< uchar > inliers;
	
	Mat H = findHomography( samplePoints, scenePoints, CV_RANSAC, 3, inliers );

	int inliersCount = accumulate( inliers.begin(), inliers.end(), 0 );
	float inliersRatio = (float) inliersCount / inliers.size();

	//if( debug )
		cerr << "\tInliers ratio is " << inliersRatio << endl;

	float minInlierRatio = 0.50;

	if( inliersRatio < minInlierRatio ) {
		if( debug )
			cerr << "\tToo many outliers\n";

		return matchingObject;
	}

	if( debug )
		cerr << "\tHomography matrix calculated, mapping " << labelDB[ maxSample ].size() << " label points\n";

	vector< Point2f > mappedPoints, re;

	for( vector< Label >::iterator point = labelDB[ maxSample ].begin(); point != labelDB[ maxSample ].end(); point++ )
		mappedPoints.push_back( ( *point ).position );

	perspectiveTransform( mappedPoints, re, H );

	for( int i = 0; i < mappedPoints.size(); i++ )
		matchingObject.addLabel( Label( labelDB[ maxSample ][ i ].name, re[ i ], labelDB[ maxSample ][ i ].color ) );

	if( debug )
		cerr << "\n\tMatching done. Returning the object\n\n";

	return matchingObject;
	
}

/**
 * @brief	Creates the database from the sample images
 * @details	Loaded the images contained in the argument path
 * 			associate to every image sample its keypoints and descriptors.
 * 			Also loads the label positions in the samples.
 * 			Saves everything in three structures and to a set of files
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

	// Random color generator for label coloring
	boost::mt19937 rng( time( 0 ) );
	boost::uniform_int<> colorRange( 0, 255 );
	boost::variate_generator< boost::mt19937, boost::uniform_int<> > color( rng, colorRange );

	// For every image compute descriptors, load labels and save them
	// Associate to every label a random color for visualization
	for( fs::directory_iterator it( fullPath ); it != end_iter; ++it ) {
		fs::path extension = fs::extension( it -> path() );

		if( !( extension == ".jpg" || extension == ".png" ) ) {
			if( debug )
				cerr << "\t" << extension << " not an image extension, skipping..\n";

			continue;
		}

		if( debug ) {
			cerr << "\tTreating a new image: ";
			cerr << it -> path().stem() << endl;
		}
	
		load = imread( it -> path().string() );
		
		// Detect the keypoints in the actual image
		featureDetector -> detect( load, keypoints );

		if( debug )
			cerr << "\tFeatures detected" << endl;

		// Compute SURF descriptors
		featureExtractor -> compute( load, keypoints, descriptors );

		if( debug )
			cerr << "\tDescriptors extracted\n";

		string labelName = it -> path().stem().string();

		string labelFileName = it -> path().parent_path().string() + "/" + it -> path().stem().string() + ".lbl";

		ifstream loadLabels( labelFileName.c_str(), ios::in );

		if( debug )
			cerr << "\tLoading labels from file " << labelFileName << endl;

		// Read the labels associated to this image from the .lbl file
		vector< Label > localLabels;
		string name, x, y;

		while( loadLabels >> name ) {
			loadLabels >> x >> y;

			if( debug )
				cerr << "\t\tLoading labed " << name << " " << x << " " << y << endl;

			localLabels.push_back( Label( name, Point2f( ::atof( ( x ).c_str() ), ::atof( ( y ).c_str() ) ), Scalar( color(), color(), color() ) ) );
		}

		// Add everything to the structures
		// The association between the structure is gained by position
		labelDB.push_back( localLabels );

		if( debug )
			cerr << "\tLabels loaded\n";

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
 * @brief	Load existing database and fill the structures
 */
void Database::load() {
	if( debug )
		cerr << "Loading database\n";

	string dbFileName = dbPath + dbName;

	ifstream desc( ( dbFileName + "desc.sbra" ).c_str(), ios::binary );

	ifstream label( ( dbFileName + "label.sbra" ).c_str(), ios::binary );
	ifstream kp( ( dbFileName + "kp.sbra" ).c_str(), ios::in );

	if( label.fail() || kp.fail() || desc.fail() )
		throw DBLoadingException();
	else if( debug )
		cerr << "\tLoading from " << dbFileName << endl;

	boost::archive::binary_iarchive descarch( desc );
	descarch >> descriptorDB;

	if( debug )
		cerr << "\t\tDescriptors loaded\n";
	
	// LabelDB
	string line;
	label >> line;
	
	// Random color generator for label coloring
	boost::mt19937 rng( time( 0 ) );
	boost::uniform_int<> colorRange( 0, 255 );
	boost::variate_generator< boost::mt19937, boost::uniform_int<> > color( rng, colorRange );

	while( true ) {
		vector< Label > temp = vector< Label >();
		string name, x, y;

		if( debug )
			cerr << "\t\t\tLine " << line << endl;

		while( y.compare( "Sample" ) && label >> name ) {
			if( !name.compare( "Sample" ) ) {
				line = name;
				break;
			}

			label >> x >> y;

			temp.push_back( Label( name,
					Point2f( ::atof( ( x ).c_str() ),
							 ::atof( ( y ).c_str() ) ),
					Scalar( color(), color(), color() ) ) );
			
			if( debug )
				cerr << "\t\t\tLabel " << temp[ temp.size() - 1 ].name << " " << temp[ temp.size() - 1 ].position << endl;
		} 

		labelDB.push_back( temp );

		if( label.eof() )
			break;
	}
	
	if( debug )
		cerr << "\t\tLabels loaded\n\n";

	// keypointDB. Discard the first line as I expect it to be a Label marker
	kp >> line;

	while( true ) {
		vector< KeyPoint > temp = vector< KeyPoint >();
		string x, y, size, angle, response, octave, class_id;

		if( debug )
			cerr << "\t\t\tLine " << line << endl;

		while( class_id.compare( "Sample" ) && kp >> x ) {
			if( !x.compare( "Sample" ) ) {
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

	if( debug )
		cerr << "\tLoad successfull" << endl;

	matcher.add( descriptorDB );
	matcher.train();

	if( debug )
		cerr << "\tMatcher trained successfully" << endl;
}

/**
 * @brief	Writes the database to a set of files in the default directory 
 */
void Database::save() {
	if( debug )
		cerr << "Saving the created database" << endl;

	string dbFileName = dbPath + dbName;

	// Empty file for existence check
	ofstream ex( ( dbFileName + ".sbra" ).c_str(), ios::out );
	ex << "SBRA!";
	ex.close();

	ofstream desc( ( dbFileName + "desc.sbra" ).c_str(), ios::binary );

	ofstream label( ( dbFileName + "label.sbra" ).c_str(), ios::binary );
	ofstream kp( ( dbFileName + "kp.sbra" ).c_str(), ios::out );

	if( label.fail() || kp.fail() || desc.fail() )
		throw DBSavingException();
	else if( debug )
		cerr << "\tSaving to " << dbFileName << endl;

	boost::archive::binary_oarchive descarch( desc );
	descarch << descriptorDB;

	// labelDB
	for( vector< vector< Label > >::iterator it = labelDB.begin(); it != labelDB.end(); it++ ) {
		label << "Sample" << endl;

		for( vector< Label >::iterator jt = ( *it ).begin(); jt != ( *it ).end(); jt++ )
			label << ( *jt ).name << " " << ( *jt ).position.x << " " << ( *jt ).position.y << endl;
	}

	// keypointDB
	for( vector< vector< KeyPoint > >::iterator it = keypointDB.begin(); it != keypointDB.end(); it++ ) {
		kp << "Sample" << endl;

		for( vector< KeyPoint >::iterator jt = ( *it ).begin(); jt != ( *it ).end(); jt++ )
			kp << ( *jt ).pt.x << " " << ( *jt ).pt.y << " " << ( *jt ).size << " " << ( *jt ).angle << " " << ( *jt ).response << " " << ( *jt ).octave << " " << ( *jt ).class_id << endl;
	}
	
	label.close();
	desc.close();
	kp.close();

	if( debug )
		cerr << "\tSave successfull" << endl;
}
