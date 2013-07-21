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
	m_detector = FeatureDetector::create("FAST");

	setRunning(false);

	if (debug)
		cerr << TAG << " constructed.\n";
}

Tracker::~Tracker()
{}

/* Setters */

void Tracker::setObject(Object new_object)
{
	Mat new_frame = m_history.getStarter();
	Features new_features = calcFeatures(new_object, new_frame);

	setObject(new_object, new_frame, new_features);
}

void Tracker::setObject(Object new_object, Mat new_frame, Features new_features)
{
	unique_lock<shared_mutex> lock(m_object_mutex);

	m_object = new_object;
	m_frame = new_frame.clone();
	m_features = new_features;
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
	Features old_features,
					 new_features;

	m_history.enqueue(new_frame);

	// Synchronized
	{
		shared_lock<shared_mutex> lock(m_object_mutex);

		old_frame = m_frame.clone();
		old_object = m_object;
		old_features = m_features;
	}

	new_object = trackFrame(old_frame, new_frame, old_object, old_features);
	new_features = calcFeatures(new_object, new_frame);
	setObject(new_object, new_frame, new_features);

	return new_object;
}

Object Tracker::trackFrame(Mat old_frame, Mat new_frame,
													 Object old_object, Features old_features)
{
	if (debug)
		cerr << TAG << ": Tracking object.\n";

	Object new_object;
	for (Label label : old_object.getLabels())
	{
		vector<Point2f> old_mask = old_object.getMask(label),
										new_mask,
										tracked_pts;
		vector<uchar> status;
		vector<float> error;

		if (old_mask.empty() || old_features[label].empty())
			break;

		calcOpticalFlowPyrLK(old_frame, new_frame,
												 old_features[label], tracked_pts,
												 status, error);

		vector<Point2f> old_pts,
										new_pts;
		for (size_t i = 0; i < tracked_pts.size(); i++)
			if (status[i])
			{
				old_pts.push_back(old_features[label][i]);
				new_pts.push_back(tracked_pts[i]);
			}

		if (new_pts.empty())
			break;
		
		Mat H = estimateRigidTransform(old_pts, new_pts, true);
		if (H.empty())
			break;

		transform(old_mask, new_mask, H);

		if (debug)
			cerr << TAG << ": Mask obtained: " << new_mask << endl;

		new_object.setLabel(label, new_mask, old_object.getColor(label));
	}

	return new_object;
}

Features Tracker::calcFeatures(Object new_object, Mat new_frame)
{
	if (debug)
		cerr << TAG << ": Calculating features.\n";

	Features new_features;
	for (Label label : new_object.getLabels())
	{
		vector<Point2f> contour_mask = new_object.getMask(label),
										adjusted_mask;
		vector<Point> vertexes;
		Mat(contour_mask).copyTo(vertexes);

		Mat mask = Mat(new_frame.size(), CV_8UC1, Scalar(0));
		fillConvexPoly(mask, &vertexes[0], vertexes.size(), Scalar(255));

		vector<KeyPoint> key_pts;
		m_detector->detect(new_frame, key_pts, mask);

		if (debug)
			cerr << TAG << ": Found " << key_pts.size() << " keypoints.\n";

		vector<Point2f> features;
		for (KeyPoint a_kp : key_pts)
			features.push_back(a_kp.pt);
		new_features[label] = features;
	}

	return new_features;
}	

void Tracker::actualizeObject(Object new_object)
{
	if (debug)
		cerr << TAG << ": Actualizing frame.\n";

	m_history.discard();

	Mat old_frame,
			new_frame = m_history.getStarter();
	Object old_object;
	Features old_features,
					 new_features = calcFeatures(new_object, new_frame);

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

		new_object = trackFrame(old_frame, new_frame, old_object, old_features);
		new_features = calcFeatures(new_object, new_frame);
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
			m_history.start( (*(Mat*)data).clone() );
			break;

		case Manager::MSG_RECOGNITION_END:
			//actualizeObject(*(Object*)data);
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

