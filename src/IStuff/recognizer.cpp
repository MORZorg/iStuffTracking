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
	//m_thread = auto_ptr<thread>(new thread());
	setRunning(false);

	if (debug)
		cerr << TAG << " constructed.\n";
}

Recognizer::~Recognizer()
{}

/* Setters */

/**
 * @brief Associates a IStuff::Database to this IStuff::Recognizer.
 *
 * @param[in] matcher	The new matcher to be used.
 */
void Recognizer::setDatabase(Database* matcher)
{
	m_matcher = matcher;
}

void Recognizer::setRunning(bool running)
{
	m_running = running;
}

/* Getters */

/**
 * @brief Checks whether this IStuff::Recognizer has a thread up and running.
 *
 * @return `true` if recognizing, `false` otherwise.
 */
bool Recognizer::isRunning() const
{
	return m_running;
}

/* Other methods */

/**
 * @brief Recognizes an IStuff::Object into a frame.
 *
 * @param[in] frame	The frame to be searched for an IStuff::Object.
 *
 * @return The IStuff::Object found inside the given frame.
 */
Object Recognizer::recognizeFrame(Mat frame)
{
	if (debug)
		cerr << TAG << ": Recognizing frame.\n";

	Object result = m_matcher->match(frame);

	if (debug)
		cerr << TAG << ": Frame recognized.\n";

	return result;
}

/**
 * @brief Method to do the recognization process in a separate thread.
 *
 * @param[in] frame			The frame to be searched for an IStuff::Object.
 * @param[in] reference	The reference to the IStuff::Manager to inform of the result.
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
	m_thread = auto_ptr<thread>(new thread([=]()
			{
				setRunning(true);

				Object new_object = recognizeFrame(frame);
				reference->sendMessage(Manager::MSG_RECOGNITION_END, &new_object);

				setRunning(false);
			}));

	return true;
}

/**
 * @brief Method to send messages to this IStuff::Recognizer.
 * @details Managed messages:<br />
 *	<dl>
 *		<dt>IStuff::Manager::MSG_RECOGNITION_START</dt>
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
			//tmp = recognizeFrame(*(Mat*)data);
			//((Manager*)reply_to)->sendMessage(Manager::MSG_RECOGNITION_END, &tmp);
			break;
		case Manager::MSG_RECOGNITION_END:
			break;
		default:
			break;
	}
}

