/**
 * @file main.cpp
 * @brief Main file
 * @author Maurizio Zucchelli
 * @author Mattia Rizzini
 * @version 0.1.2
 * @date 2013-07-13
 */

#include "main.h"

using namespace std;
using namespace cv;
using namespace IStuff;

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

	string dbName,
				 dbDir = "image_sample/clean/";

	// Command line flags parsing, mostly debug level
	while (++i < argc)
	{
    	if (argv[i][0] != '-')
		{
			printHelp();
			exit(1);
		}

		if (argv[i][1] == '-')
		{
			argv[i] += 2;

			if (!strcmp(argv[i], "help")) {
				printHelp();
				return 0;
			}
			else if(!strcmp(argv[i], "database"))
			{
				dbName = argv[++i];
			}
			else if(!strcmp(argv[i], "folder"))
			{
				dbDir = argv[++i];
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
	
	try
	{
		db = new IStuff::Database(dbName, dbDir);
	}
	catch ( IStuff::DBCreationException& e )
	{
		cout << e.what() << endl;
		exit(2);
	}
	catch (IStuff::DBLoadingException& e)
	{
		cout << e.what() << endl;
		exit(2);
	}
	catch (IStuff::DBSavingException& e)
	{
		cout << e.what() << endl;
		exit(2);
	}

  namedWindow("Camera", CV_WINDOW_AUTOSIZE);

  VideoCapture capture = VideoCapture(CV_CAP_ANY);

	Manager manager;
	manager.setDatabase(db);
  
  // Show the image captured from the camera in the window and repeat
  while (true)
  {
    // Get one frame
    Mat frame;
    capture >> frame;

		manager.elaborateFrame(frame);
		waitKey(0);
		frame = manager.paintObject(frame);

    imshow("Camera", frame);
  }

  capture.release();
  destroyWindow("Camera");

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
	cout << "\t--database databaseName\tLoad databaseName. (necessary)\n";
	cout << "\t--folder folderName\tIndicates where to find images\n"
		<< "\t\tfor database creation.\n";
}

