/**
 * @file recognizer.cpp
 * @class	Recognizer
 * @brief Class used to recognize objects in a video stream.
 * @details	This class manages a thread receiving a frame from an input stream,
 *	analyzing it to find some kind of 3D object and then updating the data used 
 *	by the requester to track it.
 * @author Maurizio Zucchelli
 * @version 0.2.0
 * @date 2013-07-14
 */

#include "recognizer.h"

using namespace std;
using namespace boost;
using namespace cv;
using namespace IStuff;

const char Recognizer::TAG[] = "Rec";

/* Constructors and Destructors */

/**
 * @brief Constructs a structure used to find 3D objects inside a video stream.
 */
Recognizer::Recognizer()
{
	running = new thread();

	if (debug)
		cerr << TAG << " constructed.\n";
}

Recognizer::~Recognizer()
{}

/* Setters */

/**
 * @brief Associates a Database to this Recognizer.
 *
 * @param[in] matcher	The matcher to be updated.
 */
void Recognizer::setDatabase(Database* matcher)
{
	this->matcher = matcher;
}

/* Getters */

/**
 * @brief Checks whether this ObjectRecognizer has a thread up and running.
 *
 * @return `true` if recognizing, `false` otherwise.
 */
bool Recognizer::isRunning() const
{
	return *running != thread();
}

/* Other methods */

/**
 * @brief Function 
 */
void Recognizer::recognizeFrame(Mat frame, Manager* callback)
{
	if (debug)
		cerr << TAG << ": Recognizing frame.\n";

	sleep(5);
}

bool Recognizer::backgroundRecognizeFrame(Mat frame, Manager* callback)
{
	if (isRunning())
	{
		if (debug)
			cerr << TAG << ": Already started in background!\n";

		return false;
	}

	if (debug)
		cerr << TAG << ": Starting in background.\n";

	// NOTE: "[=]" means "all used variables are captured in the lambda".
	running = new thread([=]() { recognizeFrame(frame, callback); });
	return true;
}

