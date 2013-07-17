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
 * @brief Updates the Object used by the tracker.
 * @todo Change to private, if possible.
 *
 * @param new_object
 */
void Tracker::setObject(Object new_object)
{
	object_update.lock();

	actual_object = new_object;

	object_update.unlock();

	if (debug)
		cerr << TAG << ": Object updated.\n";
}

/* Getters */

/**
 * @brief Returns the Object tracked.
 *
 * @return The Object.
 */
Object Tracker::getObject()
{
	object_update.lock_shared();

	Object copy = actual_object;

	object_update.unlock_shared();

	return copy;
}

/**
 * @brief Checks whether this Tracker is actualizing an Object.
 *
 * @return 
 */
bool Tracker::isRunning() const
{
	// HACK: Bad way around to check if a thread has finished running.
	return running->try_join_for(chrono::nanoseconds(0)) && running->joinable();
}

/* Other methods */

/**
 * @brief Method to inform this Tracker that a recognition has started.
 * @details This causes the other methods to start saving the frames,
 *	if they aren't already. This also resets a counter used to discard possible
 *	unactualized frames when the recognition ends.
 *
 * @param[in] current_frame	The frame used for the recognition.
 */
void Tracker::recognitionStarted(Mat current_frame)
{
	history_update.lock();

	frame_history.push(current_frame);
	frames_tracked_count = 0;

	history_update.unlock();
}

/**
 * @brief Method to inform this Tracker that the last recognition has ended.
 * @details This causes the Tracker to start actualizing the new Object using
 *	the frames captured during the recognition process.
 *	If the actualization was already happening, the process is interrupted,
 *	the unactualized frames are discarded and the current object is substituted
 *	before restarting the actualization.
 *
 * @param[in] new_object	The Object obtained from the recognition.
 */
void Tracker::recognitionEnded(Object new_object)
{
	if (isRunning())
	{
		if (debug)
			cerr << TAG << ": Stopping current actualization.\n";

		// I know the thread will quit when it reaches the interruption point,
		// so I can let it finish its execution.
		running->interrupt();
		running = auto_ptr<thread>(new thread());


		history_update.lock();

		while (frame_history.size() > frames_tracked_count)
			frame_history.pop();

		history_update.unlock();


		setObject(new_object);
	}

	backgroundActualizeObject(new_object);
}

/**
 * @brief Tracks the current object between the last frame and this one.
 * @details This method changes the the actual object and the last frame held
 *	by the class. Also, if there's a recognition is in progress, the new frame
 *	is stored.
 *
 * @param[in] new_frame	The frame where to track the Object.
 */
void Tracker::trackFrame(Mat new_frame)
{
	if (debug)
		cerr << TAG << ": Tracking external object.\n";

	Mat old_frame = last_frame;

	last_frame = new_frame;

	history_update.lock();

	if (frame_history.size())
		frame_history.push(new_frame);

	history_update.unlock();

	setObject(trackFrame(old_frame, new_frame, getObject()));
}

/**
 * @brief Tracks the given object between the two given frames.
 *
 * @param[in] old_frame		The frame whose the Object is referred to.
 * @param[in] new_frame		The frame where to track the Object.
 * @param[in] old_object	The Object to be tracked.
 *
 * @return The new Object tracked in the new frame.
 */
Object Tracker::trackFrame(Mat old_frame, Mat new_frame, Object old_object)
{
	if (debug)
		cerr << TAG << ": Tracking object.\n";

	// TODO
	usleep(1000);

	return old_object;
}

/**
 * @brief Makes the tracking for a given Object on every frame stored.
 * @details This method is thread-ready, containing interruption points.
 *	At every step, a frame is removed from the queue and the class' Object is
 *	replaced.
 *
 * @param old_object The object to be actualized.
 */
void Tracker::actualizeObject(Object old_object)
{
	history_update.lock();

	if (frame_history.empty())
		return;

	Mat old_frame = frame_history.front(),
			new_frame;
	frame_history.pop();

	history_update.unlock();

	while (true)
	{
		try
		{
			history_update.lock();

			if (frame_history.size())
			{
				new_frame = frame_history.front();
				frame_history.pop();

				history_update.unlock();
			}
			else
			{
				history_update.unlock();

				if (debug)
					cerr << TAG << ": Actualization finished.\n";

				return;
			}

			// For concurrent execution:
			// Avoid doing the computationally expensive part if there's been an
			// interruption_request.
			this_thread::interruption_point();

			old_object = trackFrame(old_frame, new_frame, old_object);

			// For concurrent execution:
			// Avoid saving if there's been an interruption_request.
			this_thread::interruption_point();
			
			setObject(old_object);

			old_frame = new_frame;
		}
		catch (const boost::thread_interrupted&)
		{
			if (debug)
				cerr << TAG << ": thread interrupted.\n";

			return;
		}
	}
}

/**
 * @brief Method to do the actualization process in a separate thread.
 *
 * @param[in] old_object	The Object to be actualized.
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
