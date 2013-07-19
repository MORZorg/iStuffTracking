/**
 * @file fakable_queue.cpp
 * @brief Class used to manage a synchronized double queue.
 * @details This class is used by IStuff::Tracker and it allows to address
 *	problems caused by alternated recognizations:<br />
 *	This queue manages two queues: one "real", used normally, from where frames
 *	are enqueued and dequeued and one "fake", or "saved", used to store the
 *	frames regarding only the last recognization, where frames are just enqueued
 *	and which is substituted to the real queue when the last recognization ends.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-18
 */

#include "fakable_queue.h"

using namespace std;
using namespace boost;
using namespace cv;
using namespace IStuff;

const char FakableQueue::TAG[] = "Fkq";

/* Constructors and Destructors */

/**
 * @brief Constructor of this class.
 */
FakableQueue::FakableQueue()
{}

FakableQueue::~FakableQueue()
{}

/* Setters */

/**
 * @brief Adds a frame to the queue.
 * @details If the queue has been started, the frame is added to both the 'real'
 *	and the 'saved' queue; otherwise, nothing happens.
 *
 * @param[in] frame	The frame to be inserted.
 */
void FakableQueue::enqueue(Mat frame)
{
	if (debug)
		cerr << TAG << ": enqueue.\n";
	
	lock_guard<mutex> lock(queue_mutex);

	if (real_queue.empty())
		return;
	else
	{
		real_queue.push(frame);
		saved_queue.push(frame);
	}
}

/**
 * @brief Starts the queue, enabiling the enqueuement.
 * @details This operation resets the 'saved' queue and then performs an
 *	enqueue with the given frame.
 *
 * @param[in] frame	The frame starter of the queue.
 */
void FakableQueue::start(Mat frame)
{
	if (debug)
		cerr << TAG << ": start.\n";
	
	lock_guard<mutex> lock(queue_mutex);
	
	saved_queue = queue<Mat>();

	real_queue.push(frame);
	saved_queue.push(frame);
}

/**
 * @brief Replaces the 'real' queue with the 'saved' one.
 * @todo Find a more explicative name.
 */
void FakableQueue::discard()
{
	if (debug)
		cerr << TAG << ": discard.\n";
	
	lock_guard<mutex> lock(queue_mutex);
	
	real_queue = saved_queue;
}

/* Getters */

/**
 * @brief Returns and removes a frame from the 'real' queue.
 * @throw out_of_range If the queue is empty.
 *
 * @return The frame in front of the 'real' queue.
 */
Mat FakableQueue::dequeue()
{
	if (debug)
		cerr << TAG << ": dequeue.\n";
	
	Mat result;

	lock_guard<mutex> lock(queue_mutex);
	
	if (real_queue.empty())
		throw out_of_range("FakableQueue empty");

	result = real_queue.front();
	real_queue.pop();

	return result;
}

/**
 * @brief Returns the frame that started the queue.
 *
 * @return The frame that started the queue.
 */
Mat FakableQueue::getStarter()
{
	if (debug)
		cerr << TAG << ": getStarter.\n";
	
	// Could be shared...
	lock_guard<mutex> lock(queue_mutex);

	return saved_queue.front();
}

