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
int main(int argc, char* argv[])
{
  bool video = false,
	   notrack = false;
  string dbName,
         dbDir,
         videoSrc,
         videoDst;

  // Command line flags parsing, mostly debug level
  if (argc == 1)
  {
    printHelp();
    exit(1);
  }

  int i = 0;
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

      if (!strcmp(argv[i], "help"))
      {
        printHelp();
        return 0;
      }
      else if(!strcmp(argv[i], "video" ))
      {
        video = true;
        videoSrc = argv[++i];
      }
      else if(!strcmp(argv[i], "database"))
      {
        dbName = argv[++i];
      }
      else if(!strcmp(argv[i], "folder"))
      {
        dbDir = argv[++i];
      }
	  else if( !strcmp( argv[ i ], "notrack" ) )
	  {
        notrack = true;
	  }
      else if (!strcmp(argv[i], "output"))
      {
        videoDst = argv[++i];
      }
    }
    else
    {
      string extended_command;
      switch (argv[i][1])
      {
        case 'd':
          // Debug
          switch (argv[i][2])
          {
            case 0:
            case '0':
              cerr << "Full debug on.\n";
              debug = true;
            case '1':
              cerr << "High level debug on.\n";
              hl_debug = true;
              break;
            default:
              cerr << "Undefined debug mode.\n";
              printHelp();
              exit(1);
          }
          break;
        case 'f':
          extended_command = "--folder";
          argv[i--] = &extended_command[0];
          break;
        case 'v':
          extended_command = "--video";
          argv[i--] = &extended_command[0];
          break;
		case 't':
  		  extended_command = "--notrack";
		  argv[ i-- ] = &extended_command[ 0 ];
		  break;
        case 'o':
          extended_command = "--output";
          argv[i--] = &extended_command[0];
          break;
        default:
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
  catch (IStuff::DBCreationException& e)
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

  vector<Mat> output_history;

  Manager manager;
  manager.setDatabase(db);

  time_t start = time( NULL );
  size_t frames = 0;

  int key = -1;
  if (!video)
  {
    namedWindow("Camera", CV_WINDOW_AUTOSIZE);

    VideoCapture capture = VideoCapture(0);

    // Show the image captured from the camera in the window and repeat
    while (key != 'q')
    {
      // Get one frame
      Mat frame;
      capture >> frame;
	  frames++;

	  if( notrack )
	  {
        Object o = db->match(frame);
        frame = o.paint(frame);
	  }
	  else
	  {
        manager.elaborateFrame(frame);
        frame = manager.paintObject(frame);
	  }

      imshow("Camera", frame);

      if (!videoDst.empty())
        output_history.push_back(frame);

      key = waitKey(1);
    }

    capture.release();
    destroyWindow("Camera");
  }
  else
  {
    namedWindow( "Video", CV_WINDOW_AUTOSIZE );

    VideoCapture capture = VideoCapture( videoSrc );

    // Show the image captured from the camera in the window and repeat
    while( key == -1 )
    {
      // Get one frame
      Mat frame;
      capture >> frame;
	  frames++;

      if( frame.empty() ) {
        cerr << "Capture error or video ended. Exiting..\n";
        break;
      }

      //Object o = db -> match( frame );
      //frame = o.paint( frame );
      manager.elaborateFrame(frame);
      frame = manager.paintObject(frame);

      imshow( "Video", frame );

      if (!videoDst.empty())
        output_history.push_back(frame);

      key = waitKey( 10 );
    }

    capture.release();
    destroyWindow( "Video" );
  }

  time_t end = time( NULL );
  double duration = difftime( end, start );
  double fps = (double) frames / duration;

  cout << "Frames: " << frames << endl;
  cout << "Time: " << duration << endl;
  cout << "Frame rate: " << fps << endl;

  if (!videoDst.empty())
  {
    float min_fps = 10,
    int fps_multiplier;
    if (fps < min_fps)
      fps_multiplier = ceil(min_fps / fps);
    
    VideoWriter output(videoDst,
                       CV_FOURCC('M', 'J', 'P', 'G'),
                       fps * fps_multiplier,
                       output_history[0].size());

    for (Mat frame : output_history)
      for (size_t i = 0; i < fps_multiplier; i++)
        output << frame;
  }

  return 0;
}

/**
 * @brief Function to display the help message.
 */
void printHelp()
{
  cout << "Usage:\n";
  cout << "\t--help\tShow this help and exit.\n";
  cout << "\t-dN\t\tShow debug messages.\n"
    << "\t\t\tWhere N is an optional integer ranging from 0 to SBRA.\n"
    << "\t\t\tWith 0 indicating the most verbose debug possible.\n";
  cout << "\t--notrack\tUse just pure recognition. (Also -t)\n";
  cout << "\t--database name\tLoad the database called `name`. (necessary)\n";
  cout << "\t--folder path\tIndicates where to find images\n"
    << "\t\t\tfor database creation. (Also -f)\n";
  cout << "\t--video path\tUse video instead of camera. (Also -v)\n";
  cout << "\t--output path\tOutput result to video. (Also -o)\n";
}

