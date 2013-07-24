/**
 * @file tracker.cpp
 * @class IStuff::Tracker
 * @brief Class used to track objects in a video stream.
 * @author Maurizio Zucchelli
 * @version 0.8.0
 * @date 2013-07-17
 */

#include "tracker.h"

using namespace std;
using namespace boost;
using namespace cv;
using namespace IStuff;

const char Tracker::TAG[] = "Trk";
const Size Tracker::LK_WINDOW = cv::Size(15, 15);

/* Constructors and Destructors */

/**
 * @brief Constructs a structure used to track 3D objects inside a video stream.
 */
Tracker::Tracker()
{
  m_detector = FeatureDetector::create("GFTT");
  m_matcher = DescriptorMatcher::create("FlannBased");

  if (debug)
    cerr << TAG << " constructed.\n";
}

Tracker::~Tracker()
{}

/* Setters */

/**
 * @brief Method used by this IStuff::Tracker's thread to mark itself as running.
 *
 * @param[in] running The status to assign to the thread.
 */
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
 * @details This method is synchronized for its whole duration, this avoids
 *	other object updates to happen between the calculations and the internal
 *	data update.
 * 
 * @param[in] new_frame	The frame where to track the IStuff::Object.
 *
 * @return The new IStuff::Object tracked in the new frame.
 */
Object Tracker::trackFrame(Mat new_frame)
{
  if (debug)
    cerr << TAG << ": Tracking frame.\n";

  Object new_object;
  Mat small_new_frame;
  Features new_features;

  resize(new_frame, small_new_frame, Size(),
         IMG_RESIZE, IMG_RESIZE, INTER_AREA);

  lock_guard<mutex> lock(m_object_mutex);

  // Syncrhonizing this whole operation ensures no writing occurs
  // during this tracking
  new_features = calcFeatures(m_frame, small_new_frame, &m_features);
  new_object = updateObject(m_features, new_features, m_object);

  m_object = new_object;
  m_frame = small_new_frame;
  m_features = new_features;

  return new_object;
}

/**
 * @brief Method to calculate the IStuff::Features used to track IStuff::Object between frames.
 *
 * @param[in] frame The frame on which calculate the features.
 *
 * @return The IStuff::Features detected on the given frame.
 */
Features Tracker::calcFeatures(cv::Mat frame)
{
  if (debug)
    cerr << TAG << ": calcFeatures (detection).\n";

  vector<KeyPoint> key_points;
  m_detector->detect(frame, key_points);

  if (debug)
    cerr << TAG << ": Found " << key_points.size() << " keypoints.\n";

  Features features;
  for (KeyPoint a_key_point : key_points)
    features.push_back(a_key_point.pt);

  return features;
}

/**
 * @brief Method to track IStuff:Features between frames.
 *
 * @param[in] old_frame					The frame relative to the given IStuff::Features.
 * @param[in] new_frame					The frame where to track the IStuff::Features.
 * @param[in,out] old_features	The old IStuff::Features, returned erased of the untracked features.
 *
 * @return The IStuff::Features of the old frame relative to the new frame.
 */
Features Tracker::calcFeatures(cv::Mat old_frame, cv::Mat new_frame,
															 Features* old_features)
{
  if (debug)
    cerr << TAG << ": calcFeatures (optical flow).\n";

  Features new_features;

  if (old_features->empty() || new_frame.empty())
    return new_features;

  vector<uchar> status;
  vector<float> error;
  calcOpticalFlowPyrLK(old_frame, new_frame,
                       *old_features, new_features,
                       status, error, LK_WINDOW);

  if (debug)
    cerr << TAG << ": Points tracked.\n";

  for (int i = status.size()-1; i >= 0; i--)
    if (!status[i])
    {
      old_features->erase(old_features->begin() + i);
      new_features.erase(new_features.begin() + i);

      m_saved_features.erase(m_saved_features.begin() + i);
    }

  if (debug)
    cerr << TAG << ": " << new_features.size() << " points remained.\n";

  return new_features;
}

/**
 * @brief Function to update an IStuff::Object from an old position to its new one.
 * @details This method calculates the new position by mediating the movement
 *	of the nearest IStuff::Tracker::NEAREST_FEATURES_COUNT features to every
 *	point of every IStuff::Label of the IStuff::Object.
 *
 * @param[in] old_features	The IStuff::Features relative to the IStuff::Object.
 * @param[in] new_features	The IStuff::Features for the new IStuff:Object.
 * @param[in] old_object		The IStuff::Object to be updated.
 *
 * @return	The new IStuff::Object, moved according to the IStuff::Features.
 */
Object Tracker::updateObject(Features old_features, Features new_features,
                             Object old_object)
{
  if (debug)
    cerr << TAG << ": Updating object.\n";

  if (old_object.empty() || old_features.empty())
    return old_object;

  vector<Label> labels = old_object.getLabels();
  vector<Point2f> old_positions;
  for (Label a_label : labels)
    old_positions.push_back(a_label.position * .5);


  // Organize data as DescriptorMatcher wants it
  Mat positions,
      features,
      temp[2];
  split(Mat(old_positions), temp);
  hconcat(temp[0], temp[1], positions);
  split(Mat(old_features), temp);
  hconcat(temp[0], temp[1], features);

  vector< vector<DMatch> > matches;
  m_matcher->knnMatch(positions, features, matches, NEAREST_FEATURES_COUNT);

  Object new_object;
  for (size_t i = 0; i < matches.size(); i++) 
  {
    Point2f movement;
    for (size_t j = 0; j < NEAREST_FEATURES_COUNT; j++)
    {
      size_t feature_index = matches[i][j].trainIdx;
      movement += new_features[feature_index] - old_features[feature_index];
    }
    movement = movement * (1. / NEAREST_FEATURES_COUNT);

    size_t label_index = matches[i][0].queryIdx;
    Label new_label = labels[label_index];
    new_label.position += movement * 2;
    new_object.addLabel(new_label);
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
 *		This message's handling is synchronized.<br />
 *		The frame received is downscaled, then IStuff::Features are calculated and
 *		the actual IStuff::Object is updated according to this frame.
 *		The IStuff::Features are saved for use when the recognition ends.</dd>
 *		<dt>IStuff::Manager::MSG_RECOGNITION_END</dt>
 *		<dd>data: IStuff::Object<br />
 *		This message's handling is synchronized.<br />
 *		This causes the IStuff::Tracker to actualize the new IStuff::Object by
 *		tracking it from the saved IStuff::Features and the current ones.</dd>
 *	</dl>
 *
 * @param[in] msg				The message identifier.
 * @param[in] data			The data related to the message.
 * @param[in] reply_to	The sender of the message (optional).
 */
void Tracker::sendMessage(int msg, void* data, void* reply_to)
{
  Features temp_features;
  switch (msg)
  {
    case Manager::MSG_RECOGNITION_START:
      // Synchronized
      {
        lock_guard<mutex> lock(m_object_mutex);

        Mat frame;
        resize(*(Mat*)data, frame, Size(), IMG_RESIZE, IMG_RESIZE, INTER_AREA);

        // Calcolo le features per il frame,
        // traccio al contrario derivando le features relative al vecchio frame
        // aggiorno l'oggetto tra i due frames
        // salvo il frame
        m_saved_features = calcFeatures(frame);
        temp_features = m_saved_features;
        m_features = calcFeatures(frame, m_frame, &temp_features);
        m_object = updateObject(m_features, m_saved_features, m_object);
        m_frame = frame;
        m_features = m_saved_features;
      }
      break;

    case Manager::MSG_RECOGNITION_END:
      // Synchronized
      {
        lock_guard<mutex> lock(m_object_mutex);

        m_object = updateObject(m_saved_features, m_features, *(Object*)data);
      }
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

