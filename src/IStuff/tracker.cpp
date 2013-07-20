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
	detector = FeatureDetector::create("FAST");

	running = auto_ptr<thread>(new thread());

	if (debug)
		cerr << TAG << " constructed.\n";
}

Tracker::~Tracker()
{}

/* Setters */
void Tracker::setObject(Object new_object)
{
	map<Label, vector<Point2f> > new_features;
	for (Label label : new_object.getLabels())
	{
		vector<Point2f> contour_mask = new_object.getMask(label),
										adjusted_mask;
		vector<Point> vertexes;
		Mat(contour_mask).copyTo(vertexes);

		Mat mask = Mat(future_frame.size(), CV_8UC1, Scalar(0));
		fillConvexPoly(mask, &vertexes[0], contour_mask.size(), Scalar(255));

		vector<KeyPoint> key_pts;
		detector->detect(future_frame, key_pts, mask);

		if (debug)
			cerr << TAG << ": Found " << key_pts.size() << " keypoints.\n";

		vector<Point2f> features;
		for (KeyPoint a_kp : key_pts)
			features.push_back(a_kp.pt);
		new_features[label] = features;
	}

	unique_lock<shared_mutex> lock(object_update);

	original_object = new_object;
	original_frame = future_frame;
	original_features = new_features;
}

/* Getters */

/**
 * @brief Checks whether this IStuff::Tracker has a thread up and running.
 *
 * @return `true` if tracking, `false` otherwise.
 */
bool Tracker::isRunning() const
{
	// HACK: Bad way around to check if a thread has finished running.
	return running->try_join_for(chrono::nanoseconds(0)) && running->joinable();
}

/* Other methods */

/**
 * @brief Tracks the current IStuff::Object between the last frame and this one.
 * 
 * @param[in] new_frame	The frame where to track the IStuff::Object.
 *
 * @return The new IStuff::Object tracked in the new frame.
 */
Object Tracker::trackFrame(cv::Mat new_frame)
{
	if (debug)
		cerr << TAG << ": Tracking object.\n";

	Mat old_frame;
	Object old_object,
				 new_object;
	map<Label, vector<Point2f> > features;

	// Synchronized
	{
		shared_lock<shared_mutex> lock(object_update);

		old_frame = original_frame.clone();
		old_object = original_object;
		features = original_features;
	}

	for (Label label : old_object.getLabels())
	{
		vector<Point2f> old_mask = old_object.getMask(label),
										new_mask,
										tracked_pts;
		vector<uchar> status;
		vector<float> error;

		if (old_mask.empty())
			break;

		calcOpticalFlowPyrLK(old_frame, new_frame,
												 features[label], tracked_pts,
												 status, error);

		vector<Point2f> old_pts,
										new_pts;
		for (size_t i = 0; i < tracked_pts.size(); i++)
			if (status[i])
			{
				old_pts.push_back(features[label][i]);
				new_pts.push_back(tracked_pts[i]);
			}

		if (new_pts.empty())
			break;
		
		Mat H = findHomography(old_pts, new_pts, CV_RANSAC);
		perspectiveTransform(old_mask, new_mask, H);

		if (debug)
			cerr << TAG << ": Mask obtained: " << new_mask << endl;

		new_object.setLabel(label, new_mask, old_object.getColor(label));
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
	running = auto_ptr<thread>(new thread([=]()
			{
				Object new_object = trackFrame(frame);
				reference->sendMessage(Manager::MSG_TRACKING_END, &new_object);
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
				unique_lock<shared_mutex> lock(object_update);

				future_frame = (*(Mat*)data).clone();
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

