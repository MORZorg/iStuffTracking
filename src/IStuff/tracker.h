/**
 * @file tracker.h
 * @brief Header file relative to the class IStuff::Tracker.
 * @author Maurizio Zucchelli
 * @version 0.8.0
 * @date 2013-07-17
 */

#ifndef I_STUFF_TRACKER_H__
#define I_STUFF_TRACKER_H__

#include <iostream>
#include <map>

#include <boost/thread.hpp>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/nonfree/nonfree.hpp"

#include "object.h"
#include "fakable_queue.h"

extern bool debug;

namespace IStuff
{
  class Manager;

  /**
   * @brief An alias for a std::vector of cv::Point, used for tracking.
   */
  typedef std::vector<cv::Point2f> Features;

  class Tracker
  {
    /* Attributes */
    private:
      const static char TAG[];

      const static int NEAREST_FEATURES_COUNT = 10;
      const static float constexpr IMG_RESIZE = .5;
      const static cv::Size LK_WINDOW;

      std::auto_ptr<boost::thread> m_thread;
      bool m_running = false;
      boost::mutex m_object_mutex;

      Object m_object;
      cv::Mat m_frame;
      Features m_features,
               m_saved_features;

      cv::Ptr<cv::FeatureDetector> m_detector;
      cv::Ptr<cv::DescriptorMatcher> m_matcher;

      /* Methods */
    public:
      /* Constructors and Destructors */
      Tracker();
      virtual ~Tracker();

      /* Setters */

      /* Getters */
      bool isRunning() const;

      /* Other methods */
      Object trackFrame(cv::Mat);
      void sendMessage(int, void*, void* = NULL);
    private:
      /* Setters */
      void setRunning(bool);

      /* Other methods */
      Features calcFeatures(cv::Mat);
      Features calcFeatures(cv::Mat, cv::Mat, Features*);
      Object updateObject(Features, Features, Object);
      bool backgroundTrackFrame(cv::Mat, Manager*);
  };
}

#include "manager.h"

#endif /* defined I_STUFF_TRACKER_H__ */
