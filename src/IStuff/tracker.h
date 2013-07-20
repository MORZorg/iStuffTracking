/**
 * @file tracker.h
 * @brief Header file relative to the class IStuff::Tracker.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-17
 */

#ifndef I_STUFF_TRACKER_H__
#define I_STUFF_TRACKER_H__

#include <iostream>

#include <boost/thread.hpp>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/nonfree/nonfree.hpp"

#include "object.h"
#include "fakable_queue.h"

extern bool debug;

namespace IStuff
{
	class Tracker
	{
		/* Attributes */
		private:
			const static char TAG[];

			boost::shared_mutex object_update;

			Object original_object;
			cv::Mat original_frame;
			cv::Mat future_frame;

			cv::Ptr<cv::FeatureDetector> detector;

		/* Methods */
		public:
			/* Constructors and Destructors */
			Tracker();
			virtual ~Tracker();

			/* Setters */

			/* Getters */

			/* Other methods */
			Object trackFrame(cv::Mat);
			void sendMessage(int, void*, void* = NULL);
		private:
			void setObject(Object);
	};
}

#include "manager.h"

#endif /* defined I_STUFF_TRACKER_H__ */
