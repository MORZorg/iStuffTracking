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
	detector = FeatureDetector::create("GFTT");

	if (debug)
		cerr << TAG << " constructed.\n";
}

Tracker::~Tracker()
{}

/* Setters */
void Tracker::setObject(Object new_object)
{
	vector<KeyPoint> key_pts;
	detector->detect(original_frame, key_pts);

	if (debug)
		cerr << TAG << ": Found " << key_pts.size() << " keypoints.\n";


	if (key_pts.empty())
	{
		unique_lock<shared_mutex> lock(object_update);

		original_object = new_object;
		original_frame = future_frame;

		return;
	}

	for (Label label : new_object.getLabels())
	{
		vector<Point2f> mask = new_object.getMask(label),
										adjusted_mask;

		for (Point2f a_pt : mask)
		{
			float min_dist = FLT_MAX;
			Point2f min;
			for (KeyPoint a_kp : key_pts)
			{
				float dist = norm(a_pt - a_kp.pt);
				if (dist < min_dist)
				{
					min = a_kp.pt;
					min_dist = dist;
				}
			}

			adjusted_mask.push_back(min);		
		}

		new_object.setLabel(label, adjusted_mask, new_object.getColor(label));

		if (debug)
		{
			cerr << TAG << ": Mask changed from " << mask
					 << " to " << adjusted_mask << endl;
		}
	}

	unique_lock<shared_mutex> lock(object_update);

	original_object = new_object;
	original_frame = future_frame;
}

/* Getters */

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

	// Synchronized
	{
		shared_lock<shared_mutex> lock(object_update);

		old_frame = original_frame.clone();
		old_object = original_object;
	}

	for (Label label : old_object.getLabels())
	{
		vector<Point2f> old_mask = old_object.getMask(label),
										new_mask,
										tracked_pts;
		vector<uchar> status;
		vector<float> error;

		calcOpticalFlowPyrLK(old_frame, new_frame,
												 old_mask, tracked_pts,
												 status, error);

		for (size_t i = 0; i < tracked_pts.size(); i++)
			if (status[i])
			{
				new_mask.push_back(tracked_pts[i]);

				if (debug)
				{
					line(new_frame, old_mask[i], tracked_pts[i], Scalar(0, 0, 255));
					circle(new_frame, old_mask[i], 5, Scalar(255, 0, 0));
					circle(new_frame, tracked_pts[i], 5, Scalar(0, 255, 0));
				}
			}

		if (new_mask.size())
			new_object.setLabel(label, new_mask, old_object.getColor(label));

		if (debug)
		{
			cerr << TAG << ": Mask obtained from " << old_mask
					 << ": " << new_mask << endl;
		}
	}

	return new_object;
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
		default:
			break;
	}
}

