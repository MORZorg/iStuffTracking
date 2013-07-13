/**
 * @file main.cpp
 * @brief Main file
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-13
 */

#include "main.h"

using namespace cv;
using namespace std;

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

  namedWindow("Input", CV_WINDOW_AUTOSIZE);
  namedWindow("Output", CV_WINDOW_AUTOSIZE);

  VideoCapture capture = VideoCapture(CV_CAP_ANY);
  // FIXME: Check if the object has really been instantiated
  //if (capture == NULL)
  //{
  //  cerr << "ERROR: No capture device found.\n";
  //  exit( 2 );
  //}
  
  // Show the image captured from the camera in the window and repeat
  while (true)
  {
    // Get one frame
    Mat frame;
    capture >> frame;
    // FIXME: Check if the object has really been instantiated
    //if (frame == NULL)
    //{
    //  cerr << "ERROR: frame is null.\n";
    //  break;
    //}

    imshow("Input", frame);

    // Parse frame...
    // [...]
    Mat output_frame = frame;
    
    imshow("Output", output_frame);
  }

  capture.release();
  destroyWindow("Input");
  destroyWindow("Output");

  return 0;
}

void printHelp()
{
  cout << "Usage:\n";
  cout << "\t--help\tShow this help and exit.\n";
  cout << "\t-dN\tShow debug messages.\n"
       << "\t\tWhere N is an optional integer ranging from 0 to SBRA.\n"
       << "\t\tWith 0 indicating the most verbose debug possible.\n";
}

