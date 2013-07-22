/**
 * @file tracker.h
 * @brief Header file relative to the class IStuff::Tracker.
 * @author Maurizio Zucchelli
 * @version 0.5.0
 * @date 2013-07-17
 */

#ifndef I_STUFF_TRACKER_H__
#define I_STUFF_TRACKER_H__

#include <iostream>
#include <map>

#include <boost/thread.hpp>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/nonfree/nonfree.hpp"

#include "object.h"
#include "fakable_queue.h"

extern bool debug;

namespace IStuff
{
	class Manager;

	class Tracker
	{
		/* Attributes */
		private:
			const static char TAG[];
			const static int NEAREST_FEATURES_COUNT = 10;

			std::auto_ptr<boost::thread> m_thread;
			bool m_running = false;
			boost::shared_mutex m_object_mutex;

			Object m_object;
			cv::Mat m_frame;
			std::vector<cv::Point2f> m_features;

			FakableQueue m_history;

			cv::Ptr<cv::FeatureDetector> m_detector;

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
			void setObject(Object, cv::Mat, std::vector<cv::Point2f>);
			void setRunning(bool);

			/* Other methods */
			Object trackFrame(cv::Mat, cv::Mat, Object,
												std::vector<cv::Point2f>* = new std::vector<cv::Point2f>(),
												std::vector<cv::Point2f>* = new std::vector<cv::Point2f>());
			void actualizeObject(Object);
			bool backgroundTrackFrame(cv::Mat, Manager*);
	};
}

#include "manager.h"

#endif /* defined I_STUFF_TRACKER_H__ */
