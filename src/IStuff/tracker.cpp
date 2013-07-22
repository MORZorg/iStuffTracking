/**
 * @file tracker.cpp
 * @class IStuff::Tracker
 * @brief Class used to track objects in a video stream.
 * @author Maurizio Zucchelli
 * @version 0.5.0
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
	m_detector = FeatureDetector::create("GFTT");

	if (debug)
		cerr << TAG << " constructed.\n";
}

Tracker::~Tracker()
{}

/* Setters */

void Tracker::setObject(Object new_object, Mat new_frame,
												vector<Point2f> new_features)
{
	unique_lock<shared_mutex> lock(m_object_mutex);

	m_object = new_object;
	m_frame = new_frame.clone();
}

void Tracker::setRunning(bool running)
{
	m_running = running;
}

/* Getters */

/**
 * @brief Checks whether this IStuff::Tracker has a thread up and running.
 *
 * @return `true` if tracking, `false` otherwise.
 */
bool Tracker::isRunning() const
{
	return m_running;
}

/* Other methods */

/**
 * @brief Tracks the current IStuff::Object between the last frame and this one.
 * 
 * @param[in] new_frame	The frame where to track the IStuff::Object.
 *
 * @return The new IStuff::Object tracked in the new frame.
 */
Object Tracker::trackFrame(Mat new_frame)
{
	m_history.enqueue(new_frame);

	Object old_object,
				 new_object;
	Mat old_frame;
	vector<Point2f> old_features,
									new_features;

	unique_lock<shared_mutex> lock(m_object_mutex);

	old_frame = m_frame.clone();
	old_object = m_object;
	old_features = m_features;

	// Syncrhonizing this whole operation ensures no writing occurs
	// during this tracking
	new_object = trackFrame(old_frame, new_frame, old_object,
													&old_features, &new_features);

	m_object = new_object;
	m_frame = new_frame.clone();
	m_features = new_features;

	return new_object;
}

Object Tracker::trackFrame(Mat old_frame, Mat new_frame, Object old_object,
													 vector<Point2f>* old_features,
													 vector<Point2f>* new_features)
{
	if (debug)
		cerr << TAG << ": Tracking object.\n";

	if (old_features->empty())
	{
		vector<KeyPoint> key_points;
		m_detector->detect(old_frame, key_points);

		if (debug)
			cerr << TAG << ": Found " << key_points.size() << " keypoints.\n";

		if (key_points.empty())
		{
			m_frame = new_frame.clone();
			return old_object;
		}

		for (KeyPoint a_key_point : key_points)
			old_features->push_back(a_key_point.pt);
	}

	vector<uchar> status;
	vector<float> error;
	calcOpticalFlowPyrLK(old_frame, new_frame,
											 *old_features, *new_features,
											 status, error);

	if (debug)
		cerr << TAG << ": Points tracked.\n";

	for (int i = status.size()-1; i >= 0; i--)
		if (!status[i])
		{
			old_features->erase(old_features->begin() + i);
			new_features->erase(new_features->begin() + i);
		}

	if (debug)
		cerr << TAG << ": " << new_features->size() << "points remained.\n";

	if (old_object.empty())
		return old_object;

	vector<Label> labels = old_object.getLabels();
	vector<Point2f> old_positions;
	for (Label a_label : labels)
		old_positions.push_back(a_label.position);

	vector<size_t> nearest_indexes = vector<size_t>(labels.size(), 0);
	for (size_t i = 1; i < old_features->size(); i++)
		for (size_t j = 0; j < labels.size(); j++)
			if (norm((*old_features)[i] - old_positions[j])
					< norm((*old_features)[nearest_indexes[j]] - old_positions[j]))
				nearest_indexes[j] = i;

	Object new_object;
	for (size_t i = 0; i < labels.size(); i++)
	{
		Label new_label = labels[i];
		new_label.position += (*new_features)[nearest_indexes[i]]
												- (*old_features)[nearest_indexes[i]];
		new_object.addLabel(new_label);
	}

	return new_object;
}

void Tracker::actualizeObject(Object new_object)
{
	if (debug)
		cerr << TAG << ": Actualizing frame.\n";

	m_history.discard();

	Object old_object;
	Mat old_frame,
			new_frame = m_history.getStarter();
	vector<Point2f> old_features,
									new_features = vector<Point2f>();

	while (true)
	{
		try
		{
			old_frame = new_frame;
			new_frame = m_history.dequeue();
		}
		catch (const out_of_range&)
		{
			if (debug)
				cerr << ": Actualization ended.\n";

			break;
		}

		if (debug)
			cerr << ": New actualization step.\n";

		old_object = new_object;
		old_features = new_features;

		new_object = trackFrame(old_frame, new_frame, old_object,
														&old_features, &new_features);
	}

	setObject(new_object, new_frame, new_features);
}

/**
 * @brief Method to do the tracking process in a separate thread.
 *
 * @param[in] frame			The frame to be tracked for an IStuff::Object.
 * @param[in] reference	The reference to the IStuff::Manager to inform of the result.
 *
 * @return `true` if the thread is started, `false` if it was already running.
 */
bool Tracker::backgroundTrackFrame(Mat frame, Manager* reference)
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
				
				Object new_object = trackFrame(frame);
				reference->sendMessage(Manager::MSG_TRACKING_END, &new_object);

				setRunning(false);
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
			if (debug)
				cerr << TAG << ": Starting history saving.\n";

			m_history.start(*(Mat*)data);
			break;

		case Manager::MSG_RECOGNITION_END:
			if (debug)
				cerr << TAG << ": Finished history saving.\n";

			actualizeObject(*(Object*)data);
			break;

		case Manager::MSG_TRACKING_START:
			backgroundTrackFrame(*(Mat*)data, (Manager*)reply_to);
			break;

		case Manager::MSG_TRACKING_END:
			break;

		default:
			break;
	}
}

