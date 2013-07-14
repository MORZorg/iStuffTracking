/**
 * @file object_recognize.cpp
 * @class	ObjectRecognizer
 * @brief Class used to recognize objects in a video stream.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-14
 */

#include "object_recognizer.h"

using namespace std;

/* Constructors and Destructors */

ObjectRecognizer::ObjectRecognizer( VideoCapture* src, ObjectRecognizer* db ):
	source( src ), matcher( db ), running()
{
}

/* Setters */

void ObjectRecognizer::setTracker(ObjectTracker* tracker)
{
	callback = tracker;
}

/* Getters */

bool ObjectRecognizer::isRunning()
{
	return running != thread::id();
}

/* Other methods */

void ObjectRecognizer::start()
{
	if (running)
		return;

	thread executor = thread( run );
	running = executor.get_id();
}

void ObjectRecognizer::stop()
{
	if (running)
		running = false;
}

void ObjectRecognizer::run()
{
	if (running)
		return;
}


