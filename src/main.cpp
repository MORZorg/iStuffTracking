/**
 * @file main.cpp
 * @brief Main file
 * @author Maurizio Zucchelli
 * @version 0.1.0
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
int main(int argc, char * argv[])
{
  int i = 0;
  while (++i < argc)
  {
    if ( argv[i][0] != '-' )
    {
      printHelp();
      exit(1);
    }

    if ( argv[i][1] == '-' )
    {
      if (!strcmp(argv[i] + 2, "help"))
      {
        printHelp();
        return 0;
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

  namedWindow("Camera", CV_WINDOW_AUTOSIZE);

  VideoCapture capture = VideoCapture(CV_CAP_ANY);
	
	Manager manager;
	//manager.setDatabase(...);
  
  // Show the image captured from the camera in the window and repeat
  while (true)
  {
    // Get one frame
    Mat frame;
    capture >> frame;

		manager.elaborateFrame(frame);
		
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
}

