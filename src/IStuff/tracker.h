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
#include <queue>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/nonfree/nonfree.hpp"

#include "object.h"

extern bool debug;

namespace IStuff
{
	class Tracker
	{
		/* Attributes */
		private:
			const static char TAG[];

			std::auto_ptr<boost::thread> running;
			boost::shared_mutex object_update,
													history_update;

			Object actual_object;
			cv::Mat last_frame;
			std::queue<cv::Mat> frame_history;
			int frames_tracked_count;

		/* Methods */
		public:
			/* Constructors and Destructors */
			Tracker();
			virtual ~Tracker();

			/* Setters */

			/* Getters */
			Object getObject();
			bool isRunning() const;

			/* Other methods */
			void trackFrame(cv::Mat);
			void sendMessage(int, void*, void* = NULL);

		private:
			/* Setters */
			void setObject(Object, cv::Mat);

			/* Getters */
			cv::Mat getLastFrame();

			/* Other methods */
			Object trackFrame(cv::Mat, cv::Mat, Object);

			void actualizeObject(Object);
			bool backgroundActualizeObject(Object);
	};
}

#include "manager.h"

#endif /* defined I_STUFF_TRACKER_H__ */
