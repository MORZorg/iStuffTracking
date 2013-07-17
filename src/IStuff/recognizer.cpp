/**
 * @file recognizer.cpp
 * @class	IStuff::Recognizer
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
	running = auto_ptr<thread>(new thread());

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
 * @brief Checks whether this Recognizer has a thread up and running.
 *
 * @return `true` if recognizing, `false` otherwise.
 */
bool Recognizer::isRunning() const
{
	// HACK: Bad way around to check if a thread has finished running.
	return running->try_join_for(chrono::nanoseconds(0)) && running->joinable();
}

/* Other methods */

/**
 * @brief Recognizes an Object into a frame.
 *
 * @param[in] frame	The frame to be searched for an Object.
 *
 * @return The object found inside the given frame.
 */
Object Recognizer::recognizeFrame(Mat frame)
{
	if (debug)
		cerr << TAG << ": Recognizing frame.\n";

	// TODO
	sleep(1);

	if (debug)
		cerr << TAG << ": Frame recognized.\n";

	return Object();
}

/**
 * @brief Method to do the recognization process in a separate thread.
 *
 * @param[in] frame			The frame to be searched for an Object.
 * @param[in] reference	The reference to the Manager to inform of the result.
 *
 * @return `true` if the thread is started, `false` if it was already running.
 */
bool Recognizer::backgroundRecognizeFrame(Mat frame, Manager* reference)
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
	running = auto_ptr<thread>(new thread([=]()
			{
				Object new_object = recognizeFrame(frame);
				reference->sendMessage(Manager::MSG_RECOGNITION_END, &new_object);
			}));

	return true;
}

/**
 * @brief Method to send messages to this Recognizer.
 * @details Managed messages:<br />
 *	<dl>
 *		<dt>MSG_RECOGNITION_START</dt>
 *		<dd>data: cv::Mat<br />
 *		This causes the recognization process to start.</dd>
 *	</dl>
 *
 * @param[in] msg				The message identifier.
 * @param[in] data			The data related to the message.
 * @param[in] reply_to	The sender of the message (optional).
 */
void Recognizer::sendMessage(int msg, void* data, void* reply_to)
{
	switch (msg)
	{
		case Manager::MSG_RECOGNITION_START:
			backgroundRecognizeFrame(*(Mat*)data, (Manager*)reply_to);
			break;
		case Manager::MSG_RECOGNITION_END:
			break;
		default:
			break;
	}
}

