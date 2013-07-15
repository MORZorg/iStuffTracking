/**
 * @file object_recognize.cpp
 * @class	ObjectRecognizer
 * @brief Class used to recognize objects in a video stream.
 * @details	This class manages a thread taking a frame from an input stream,
 *	analyzing it to find some kind of 3D object and then updating the data used 
 *	by an ObjectTracker to track it.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-14
 */

#include "object_recognizer.h"

using namespace std;
using namespace boost;
using namespace cv;

const char ObjectRecognizer::TAG[] = "ObjRec";

/* Constructors and Destructors */

/**
 * @brief Constructs a structure used to find 3D objects inside a video stream.
 * @details The recognizer is based on a VideoCapture used as source and
 *	on an ObjectDatabase, used as database and, then, matcher.<br />
 *
 * @param[in] src	The input video stream.
 * @param[in] db	The database used for the matchings.
 */
ObjectRecognizer::ObjectRecognizer(VideoCapture* src, ObjectDatabase* db):
	source( src ), matcher( db ), running()
{}

/**
 * @brief Dummy constructor
 *
 * @param src
 */
ObjectRecognizer::ObjectRecognizer(VideoCapture* src):
	source( src )
{
	running = new thread();

	if (debug)
		cerr << TAG << " constructed.\n";
}

ObjectRecognizer::~ObjectRecognizer()
{}

/* Setters */

/**
 * @brief Associates this ObjectRecognizer to an ObjectTracker.
 * @details	This allows it to be updated regularly with new detections data.
 *
 * @param[in] tracker	The tracker to be updated.
 */
void ObjectRecognizer::setTracker(ObjectTracker* tracker)
{
	callback = tracker;
}

/* Getters */

/**
 * @brief Checks whether this ObjectRecognizer has a thread up and running.
 *
 * @return `true` if recognizing, `false` otherwise.
 */
bool ObjectRecognizer::isRunning()
{
	return *running != thread();
}

/* Other methods */

/**
 * @brief Starts the execution of the thread entitled to recognize objects.
 */
void ObjectRecognizer::start()
{
	if (isRunning())
		return;

	if (debug)
		cerr << TAG << ": starting.\n";

	running = new thread([this]() { run(); });
}

/**
 * @brief Stops the execution of the thread entitled to recognize objects.
 * @note	If the thread is stopped, the execution will still continue until 
 *	the current frame is recognized.
 */
void ObjectRecognizer::stop()
{
	if (!isRunning())
		return;

	if (debug)
		cerr << TAG << ": stopping.\n";

	running->interrupt();
	running->join();
	running = new thread();
}

/**
 * @brief Internal function used to detect each frame continuously.
 * @note	If the thread is stopped, the execution will still continue until 
 *	the current frame is recognized.
 */
void ObjectRecognizer::run()
{
	while (true)
	{
		this_thread::interruption_point();

		if (debug)
			cerr << TAG << ": Recognizing frame.\n";

		// Oggetto: insieme di label e maschera relativa al frame di rilevamento
		// Match: rileva oggetto in un frame
		// Da fare:
		//	Chiedere/prendere dalla struttura superiore un frame;
		//	Rilevare la presenza di un oggetto;
		//	Informare la struttura superiore,
		//		la quale informerà il tracker;

		sleep(5);
	}
}

