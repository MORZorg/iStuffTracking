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
	m_detector = FeatureDetector::create("FAST");
	m_matcher = DescriptorMatcher::create("FlannBased");

	if (debug)
		cerr << TAG << " constructed.\n";
}

Tracker::~Tracker()
{}

/* Setters */

void Tracker::setObject(Object new_object)
{
	vector<Point2f> history;
	Mat frame;

	// Syncrhonized
	{
		unique_lock<shared_mutex> lock(m_object_mutex);

		history = m_history;
		frame = m_frame.clone();
	}

	Point2f mean;
	for (Point2f a_point : history)
		mean + a_point;
	mean = mean * (1. / history.size());

	Object actualized_object;
	for (Label a_label : new_object.getLabels())
	{
		a_label.position += mean;
		actualized_object.addLabel(a_label);
	}

	setObject(actualized_object, frame);
}

void Tracker::setObject(Object new_object, Mat new_frame)
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
	Mat old_frame;
	Object old_object,
				 new_object;

	// Synchronized
	{
		shared_lock<shared_mutex> lock(m_object_mutex);

		old_frame = m_frame.clone();
		old_object = m_object;
	}

	if (debug)
		cerr << TAG << ": Tracking object.\n";

	vector<KeyPoint> key_pts;
	m_detector->detect(old_frame, key_pts);

	if (debug)
		cerr << TAG << ": Found " << key_pts.size() << " keypoints.\n";

	vector<Point2f> old_features,
									new_features;
	KeyPoint::convert(key_pts, old_features);

	vector<uchar> status;
	vector<double> error;
	calcOpticalFlowPyrLK(old_frame, new_frame,
											 old_features, new_features,
											 status, error);

	for (int i = status.size()-1; i >= 0; i--)
		if (!status[i])
		{
			old_features.erase(old_features.begin() + i);
			new_features.erase(new_features.begin() + i);
		}

	vector<Label> labels = old_object.getLabels();
	vector<Point2f> old_positions;
	for (Label a_label : labels)
		old_positions.push_back(a_label.position);

	vector< vector<DMatch> > matches;
	m_matcher->knnMatch(Mat(old_features), Mat(old_positions),
											matches, NEAREST_FEATURES_COUNT);

	for (vector<DMatch> a_label_matches : matches)
	{
		Point2f mean;
		for (DMatch a_match : a_label_matches)
			mean += new_features[a_match.queryIdx]
						- old_features[a_match.trainIdx];
		mean = mean * (1. / a_label_matches.size());

		Label new_label = labels[a_label_matches[0].trainIdx];
		new_label.position += mean;
		new_object.addLabel(new_label);
	}

	Point2f mean;
	for (size_t i = 0; i < old_features.size(); i++)
		mean += new_features[i] - old_features[i];
	mean = mean * (1. / old_features.size());

	// Synchronized
	{
		unique_lock<shared_mutex> lock(m_object_mutex);

		if (m_history.size())
			m_history.push_back(mean);
	}

	return new_object;
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
			// Synchronized
			{
				unique_lock<shared_mutex> lock(m_object_mutex);

				m_track_history = true;
				m_history.clear();
			}
			break;

		case Manager::MSG_RECOGNITION_END:
			setObject(*(Object*)data);
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

