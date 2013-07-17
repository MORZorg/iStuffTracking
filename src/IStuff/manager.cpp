/**
 * @file manager.cpp
 * @class IStuff::Manager
 * @brief Class to manage the joint 3D Object recognition and tracking.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-16
 */

#include "manager.h"

using namespace std;
using namespace boost;
using namespace cv;
using namespace IStuff;

const char Manager::TAG[] = "Mng";

/* Constructors and Destructors */

/**
 * @brief Constructs the class.
 */
Manager::Manager()
{
	frames_tracked_count = RECOGNITION_PERIOD;
}

Manager::~Manager()
{}

/* Setters */

/**
 * @brief Changes the database used to identify the object.
 * @details This means that with high probability a different object will be
 *	searched for: the next elaboration must be a recognition.
 *
 * @param[in] database	The new database to be used.
 */
void Manager::setDatabase(Database* database)
{
	frames_tracked_count = RECOGNITION_PERIOD;
	this->database = database;
}

/* Getters */

/**
 * @brief Returns the current description of the object.
 * @details Since it is used by the recognition thread, it is syncrhonized
 *	using the RWL pattern.
 *
 * @return The current description of the object.
 */
Object Manager::getObject()
{
	return tracker.getObject();
}

/* Other methods */

/**
 * @brief Elaborates a frame, searching for the object.
 * @details This function alternates the recognition to the tracking, making a
 *	new recognition every RECOGNITION_PERIOD frames.
 *
 * @param frame	The frame to be analyzed.
 */
void Manager::elaborateFrame(Mat frame)
{
	if (frames_tracked_count >= RECOGNITION_PERIOD && !recognizer.isRunning())
	{
		if (debug)
			cerr << TAG << ": Recognizing.\n";

		sendMessage(MSG_RECOGNITION_START, &frame);
	}

	if (debug)
		cerr << TAG << ": Tracking " << frames_tracked_count << ".\n";

	if (frames_tracked_count < RECOGNITION_PERIOD)
		frames_tracked_count++;

	// TODO
	tracker.trackFrame(frame);
}

/**
 * @brief Paints the various masks of the Object on the frame.
 *
 * @param[in] frame	The frame on which che Object must be painted.
 *
 * @return A copy of the input frame, with the Object painted on it.
 */
Mat Manager::paintObject(Mat frame)
{
	return frame;
}

/**
 * @brief Method to send messages to this Manager.
 * @details Managed messages:<br />
 *	<dl>
 *		<dt>MSG_RECOGNITION_START</dt>
 *		<dd>data: cv::Mat<br />
 *		This message is forwarded to both the Recognizer (to make it start the
 *		recognization) and the Tracker (to alert it).<br />
 *		This also resets the counter of frames tracked from last recognition.</dd>
 *		<dt>MSG_RECOGNITION_END</dt>
 *		<dd>data: Object<br />
 *		This message is forwarded to the Tracker, to update its Object.</dd>
 *	</dl>
 *
 * @param[in] msg				The message identifier.
 * @param[in] data			The data related to the message.
 * @param[in] reply_to	The sender of the message (optional).
 */
void Manager::sendMessage(int msg, void* data, void* reply_to)
{
	switch (msg)
	{
		case MSG_RECOGNITION_START:
			frames_tracked_count = 0;

			recognizer.sendMessage(msg, data, this);
			tracker.sendMessage(msg, data);

			break;
		case MSG_RECOGNITION_END:
			tracker.sendMessage(msg, data);
			break;
		default:
			break;
	}
}

