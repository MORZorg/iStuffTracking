/**
 * @file tracker.cpp
 * @class IStuff::Tracker
 * @brief Class used to track objects in a video stream.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-17
 */

#include "tracker.h"

using namespace std;
using namespace boost;
using namespace cv;
using namespace IStuff;

const char Tracker::TAG[] = "Trk";

/* Constructors and Destructors */

/**
 * @brief Constructs a structure used to track 3D objects inside a video stream.
 */
Tracker::Tracker()
{
	running = auto_ptr<thread>(new thread());

	if (debug)
		cerr << TAG << " constructed.\n";
}

Tracker::~Tracker()
{}

/* Setters */

/**
 * @brief Updates the IStuff::Object used by the IStuff::Tracker.
 * @details This is operation is done as an atomic operation.
 *
 * @param[in] new_object	The new IStuff::Object.
 * @param[in] frame				The frame the IStuff::Object is referred to.
 */
void Tracker::setObject(Object new_object, Mat frame)
{
	unique_lock<shared_mutex> lock(object_update);

	actual_object = new_object;
	last_frame = frame;

	if (debug)
		cerr << TAG << ": Object updated.\n";
}

/* Getters */

/**
 * @brief Returns the IStuff::Object tracked.
 *
 * @return The IStuff::Object.
 */
Object Tracker::getObject()
{
	shared_lock<shared_mutex> lock(object_update);

	return actual_object;
}

/**
 * @brief Returns the frame related to the IStuff::Object tracked.
 *
 * @return The last frame tracked.
 */
Mat Tracker::getLastFrame()
{
	shared_lock<shared_mutex> lock(object_update);

	return last_frame;
}

/**
 * @brief Checks whether this IStuff::Tracker is actualizing an IStuff::Object.
 *
 * @return `true` if actualizing, `false` otherwise.
 */
bool Tracker::isRunning() const
{
	// HACK: Bad way around to check if a thread has finished running.
	return running->try_join_for(chrono::nanoseconds(0)) && running->joinable();
}

/* Other methods */

/**
 * @brief Tracks the current IStuff::Object between the last frame and this one.
 * @details This method changes the the actual IStuff::Object and the last
 *	frame held by the class. Also, if there's a recognition is in progress,
 *	the new frame is stored.
 * 
 * @param[in] new_frame	The frame where to track the IStuff::Object.
 */
void Tracker::trackFrame(cv::Mat new_frame)
{
	if (debug)
		cerr << TAG << ": Tracking external object.\n";

	frame_history.enqueue(new_frame);

	Mat old_frame = getLastFrame();
	Object old_object = getObject(),
				 new_object = trackFrame(old_frame, new_frame, old_object);

	setObject(new_object, new_frame);
}

/**
 * @brief Tracks the given IStuff::Object between the two given frames.
 * 
 * @param[in] old_frame		The frame whose the IStuff::Object is referred to.
 * @param[in] new_frame		The frame where to track the IStuff::Object.
 * @param[in] old_object	The IStuff::Object to be tracked.
 * 
 * @return The new IStuff::Object tracked in the new frame.
 */
Object Tracker::trackFrame(cv::Mat old_frame, cv::Mat new_frame, Object old_object)
{
	if (debug)
		cerr << TAG << ": Tracking object.\n";

	// TODO
	usleep(1000);

	return old_object;
}

/**
 * @brief Makes the tracking for a given IStuff::Object on every frame stored.
 * @details This method is thread-ready, containing interruption points.
 *	At every step, a frame is removed from the queue and the class'
 *	IStuff::Object is replaced.
 * 
 * @param old_object The object to be actualized.
 */
void Tracker::actualizeObject(Object old_object)
{
	Mat old_frame,
			new_frame;

	try
	{
		old_frame = frame_history.dequeue();
	}
	catch (const out_of_range&)
	{
		// Nothing to actualize
		return;
	}

	while (true)
	{
		try
		{
			new_frame = frame_history.dequeue();
		}
		catch (const out_of_range&)
		{
			if (debug)
				cerr << TAG << ": Actualization finished.\n";

			return;
		}

		try
		{
			// For concurrent execution:
			// Avoid doing the computationally expensive part if there's been an
			// interruption_request.
			this_thread::interruption_point();

			old_object = trackFrame(old_frame, new_frame, old_object);

			// For concurrent execution:
			// Avoid saving if there's been an interruption_request.
			this_thread::interruption_point();
			
			setObject(old_object, new_frame);
		}
		catch (const boost::thread_interrupted&)
		{
			if (debug)
				cerr << TAG << ": thread interrupted.\n";

			return;
		}

		old_frame = new_frame;
	}
}

/**
 * @brief Method to do the actualization process in a separate thread.
 *
 * @param[in] old_object	The IStuff::Object to be actualized.
 *
 * @return `true` if the thread is started, `false` if it was already running.
 */
bool Tracker::backgroundActualizeObject(Object old_object)
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
					actualizeObject(old_object);
				}));

	return true;
}

/**
 * @brief Method to send messages to this IStuff::Tracker.
 * @details Managed messages:<br />
 *	<dl>
 *		<dt>IStuff::Manager::MSG_RECOGNITION_START</dt>
 *		<dd>data: cv::Mat<br />
 *		This causes the other methods to start saving the frames,
 *		if they aren't already. This also resets a counter used to discard
 *		possible unactualized frames when the recognition ends.</dd>
 *		<dt>IStuff::Manager::MSG_RECOGNITION_END</dt>
 *		<dd>data: IStuff::Object<br />
 *		This causes the IStuff::Tracker to start actualizing the new
 *		IStuff::Object using the frames captured during the recognition
 *		process.<br />
 *		If the actualization was already happening, the process is interrupted,
 *		the old unactualized frames are discarded and the current IStuff::Object
 *		is substituted before restarting the actualization.</dd>
 *	</dl>
 *
 * @param[in] msg				The message identifier.
 * @param[in] data			The data related to the message.
 * @param[in] reply_to	The sender of the message (optional).
 */
void Tracker::sendMessage(int msg, void* data, void* reply_to)
{
	switch (msg)
	{
		case Manager::MSG_RECOGNITION_START:
			// Synchronized
			{
				unique_lock<shared_mutex> lock(history_update);

				frame_history.start(*(Mat*)data);
			}
			break;
		case Manager::MSG_RECOGNITION_END:
			if (isRunning())
			{
				if (debug)
					cerr << TAG << ": Stopping current actualization.\n";

				// I know the thread will quit when it reaches the interruption point,
				// so I can let it finish its execution.
				running->interrupt();
				running = auto_ptr<thread>(new thread());

				setObject(*(Object*)data, frame_history.getStarter());
			}

			frame_history.discard();
			backgroundActualizeObject(*(Object*)data);
			break;
		default:
			break;
	}
}

