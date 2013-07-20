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
 * @brief Changes the IStuff::Database used to identify the IStuff::Object.
 * @details This means that with high probability a different IStuff::Object
 *	will be searched for: the next elaboration must be a recognition.
 *
 * @param[in] database	The new IStuff::Database to be used.
 */
void Manager::setDatabase(Database* database)
{
	frames_tracked_count = RECOGNITION_PERIOD;
	recognizer.setDatabase(database);
}

/* Getters */

/**
 * @brief Returns the current description of the IStuff::Object.
 *
 * @return The current description of IStuff::the Object.
 */
Object Manager::getObject()
{
	return actual_object;
}

/* Other methods */

/**
 * @brief Elaborates a frame, searching for the IStuff::Object.
 * @details This function alternates the recognition to the tracking, making a
 *	new recognition every IStuff::Manager::RECOGNITION_PERIOD frames.
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

	actual_object = tracker.trackFrame(frame);
	//sendMessage(MSG_TRACKING_START, &frame);
}

/**
 * @brief Paints the various masks of the IStuff::Object on the frame.
 *
 * @param[in] frame	The frame on which the IStuff::Object must be painted.
 *
 * @return A copy of the input frame, with the IStuff::Object painted on it.
 */
Mat Manager::paintObject(Mat frame)
{
	unique_lock<shared_mutex> lock(object_update);

	return actual_object.paint(frame);
}

/**
 * @brief Method to send messages to this IStuff::Manager.
 * @details Managed messages:<br />
 *	<dl>
 *		<dt>IStuff::Manager::MSG_RECOGNITION_START</dt>
 *		<dd>data: cv::Mat<br />
 *		This message is forwarded to both the Recognizer (to make it start the
 *		recognization) and the Tracker (to alert it).<br />
 *		This also resets the counter of frames tracked from last recognition.</dd>
 *		<dt>IStuff::Manager::MSG_RECOGNITION_END</dt>
 *		<dd>data: IStuff::Object<br />
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

		case MSG_TRACKING_START:
			tracker.sendMessage(msg, data, this);
			break;

		case MSG_TRACKING_END:
			// Syncrhonized
			{
				shared_lock<shared_mutex> lock(object_update);

				actual_object = *(Object*)data;
			}
			break;
			
		default:
			break;
	}
}

