/**
 * @file tracker.h
 * @brief Header file relative to the class IStuff::Tracker.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-17
 */

#ifndef I_STUF_TRACKER_H__
#define I_STUFF_TRACKER_H__

#include <iostream>
#include <queue>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/shared_mutex.hpp>

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
			//boost::thread* running;
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
			void setObject(Object);

			/* Getters */
			Object getObject();
			bool isRunning() const;

			/* Other methods */

			// BOZZE
			// traccia l'oggetto interno tra l'ultimo frame e quello passato
			void trackFrame(cv::Mat);

			// avverte il tracker che è iniziato un riconoscimento su questo frame
			void recognitionStarted(cv::Mat);
			// avverte il tracker che è finisto il riconoscimento
			void recognitionEnded(Object);

		private:
			// traccia l'oggetto passato tra i frames passati
			Object trackFrame(cv::Mat, cv::Mat, Object);

			// lavora per attualizzare l'oggetto 
			void actualizeObject(Object);
			bool backgroundActualizeObject(Object);
	};
}

#endif /* defined I_STUFF_TRACKER_H__ */
