/**
 * @file manager.h
 * @brief Header file relative to the class IStuff::Manager
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-16
 */

#ifndef I_STUFF_MANAGER_H__
#define I_STUFF_MANAGER_H__

#include <iostream>

#include "opencv2/core/core.hpp"

#include <boost/thread.hpp>

#include "object.h"
#include "database.h"
#include "recognizer.h"
#include "tracker.h"

extern bool debug,
						hl_debug;

namespace IStuff
{
	class Manager
	{
		/* Attributes */
		public:
			const static int MSG_RECOGNITION_START = 1;
			const static int MSG_RECOGNITION_END	 = 2;
			const static int MSG_TRACKING_START		 = 3;
			const static int MSG_TRACKING_END			 = 4;

		private:
			const static char TAG[];
			const static int RECOGNITION_PERIOD = 50;

			/**
			 * @brief When this reaches RECOGNITION_PERIOD, a new recognition is done.
			 */
			int frames_tracked_count;
			boost::shared_mutex object_update;

			Object actual_object;
			Recognizer recognizer;
			Tracker tracker;

		/* Methods */
		public:
			/* Constructors and Destructors */
			Manager();
			virtual ~Manager();

			/* Setters */
			void setDatabase(Database*);

			/* Getters */
			Object getObject();

			/* Other methods */
			void elaborateFrame(cv::Mat);
			cv::Mat paintObject(cv::Mat);

			void sendMessage(int, void*, void* = NULL);

		private:
			/* Setters */
			void setObject(const Object);
	};
}

#endif /* defined I_STUFF_MANAGER_H__ */

