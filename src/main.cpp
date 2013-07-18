/**
 * @file main.cpp
 * @brief Main file
 * @author Maurizio Zucchelli
 * @author Mattia Rizzini
 * @version 0.1.1-dbtest
 * @date 2013-07-13
 */

#include "main.h"

using namespace cv;
using namespace std;

/**
 * @brief Main function.
 *
 * @param argc
 * @param argv[]
 *
 * @return 
 */
int main( int argc, char* argv[] )
{
	int i = 0;

	string dbName = "Aragorn", dbDir = "image_sample/clean/";

	// Command line flags parsing, mostly debug level
	while (++i < argc)
	{
    	if ( argv[i][0] != '-' )
		{
			printHelp();
			exit(1);
		}

		if ( argv[i][1] == '-' )
		{
			if (!strcmp(argv[i] + 2, "help")) {
				printHelp();
				return 0;
			} else if( !strcmp( argv[i] + 2, "database" ) ) {
				dbName = argv[ ++i ];
			} else if( !strcmp( argv[i] + 2, "folder" ) ) {
				dbDir = argv[ ++i ];
			}

		}
		else
		{
			if (argv[i][1] == 'd')
			{
				// Debug
				switch (argv[i][2])
				{
					case 0:
					case '0':
						cerr << "Full debug on.\n";
						debug = true;
					case '1':
						cerr << "Sbra debug on.\n";
						break;
					default:
						cerr << "Undefined debug mode.\n";
						printHelp();
						exit(1);
				}
			}
			else
			{
				// No other flags yet.
				cerr << "Undefined flag.\n";
				printHelp();
				exit(1);
			}
		}
	}

	if (debug)
		cerr << "Flags parsed. Starting.\n";

	IStuff::Database* db;
	
	try {
		db = new IStuff::Database( dbName, dbDir );
	} catch( IStuff::DBCreationException& e ) {
		cout << e.what() << endl;
		return -1;
	} catch( IStuff::DBLoadingException& e ) {
		cout << e.what() << endl;
		return -1;
	} catch( IStuff::DBSavingException& e ) {
		cout << e.what() << endl;
		return -1;
	}

	Mat match = imread( "match_sample/CIMG3732.JPG" );
	IStuff::Object test = db -> match( match );

	/*namedWindow( "Input", CV_WINDOW_AUTOSIZE );
	namedWindow( "Output", CV_WINDOW_AUTOSIZE );

	VideoCapture capture( CV_CAP_ANY ); //= VideoCapture(CV_CAP_ANY);
	 
	Mat frame, output_frame;

	int key = -1;
	
	// Check if the stream is opened
	if( capture.isOpened() ) {
		// Show the image captured from the camera in the window and repeat
		while( key == -1 )
		{
			capture >> frame;

			if( frame.empty() ) {
				cerr << "Capture error. Exiting" << endl;
				break;
			}

			imshow( "Input", frame );

			// Elaborate frame	
			output_frame = frame;

			imshow( "Output", output_frame );

			key = waitKey( 5 );
		}
	} else
		cerr << "Source interfacing error. Exiting" << endl;

	capture.release();

	destroyWindow("Input");
	destroyWindow("Output");*/

	return 0;
}

/**
 * @brief Function to display the help message.
 */
void printHelp()
{
	cout << "Usage:\n";
	cout << "\t--help\tShow this help and exit.\n";
	cout << "\t-dN\tShow debug messages.\n"
		<< "\t\tWhere N is an optional integer ranging from 0 to SBRA.\n"
		<< "\t\tWith 0 indicating the most verbose debug possible.\n";
	cout << "\t--database databaseName\tLoad databaseName.\n";
	cout << "\t--folder folderName\tIndicates where to find images\n"
		<< "\t\tfor database creation.\n";
}

