/**
 * @file fakable_queue.h
 * @brief Header file for IStuff::FakableQueue.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-18
 */

#ifndef I_STUFF_FAKABLE_QUEUE_H__
#define I_STUFF_FAKABLE_QUEUE_H__

#include <iostream>
#include <queue>

#include <boost/thread.hpp>

#include "opencv2/core/core.hpp"

extern bool debug;

namespace IStuff
{
	class FakableQueue
	{
		/* Attributes */
		private:
			const static char TAG[];

			std::queue<cv::Mat> real_queue,
													saved_queue;
			boost::mutex queue_mutex;

		/* Methods */
		public:
			/* Constructors and Destructors */
			FakableQueue();
			virtual ~FakableQueue();

			/* Setters */
			void enqueue(cv::Mat);
			void start(cv::Mat);
			void discard();

			/* Getters */
			cv::Mat dequeue();
			cv::Mat getStarter();
	};
}

#endif
