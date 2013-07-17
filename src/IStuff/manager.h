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

#include <boost/thread/shared_mutex.hpp>

#include "opencv2/imgproc/imgproc.hpp"

#include "object.h"
#include "recognizer.h"
#include "tracker.h"

extern bool debug;

namespace IStuff
{
	class Database;

	class Manager
	{
		/* Attributes */
		private:
			const static char TAG[];
			const static int RECOGNITION_PERIOD = 20;

			/**
			 * @brief When this reaches RECOGNITION_PERIOD, a new recognition is done.
			 */
			int frames_tracked_count;
			boost::shared_mutex object_update;

			Object actual_object;
			Database* database;
			Recognizer recognizer;
			Tracker tracker;

		/* Methods */
		public:
			/* Constructors and Destructors */
			Manager();
			virtual ~Manager();

			/* Setters */
			void setDatabase(Database*);
			void setObject(Object);

			/* Getters */
			Object getObject();

			/* Other methods */
			void elaborateFrame(cv::Mat);
			cv::Mat paintObject(cv::Mat);
	};
}

#endif /* defined I_STUFF_MANAGER_H__ */

