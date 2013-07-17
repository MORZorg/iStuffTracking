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

			cv::Mat last_frame;

		/* Methods */
		public:
			/* Constructors and Destructors */
			Tracker();
			virtual ~Tracker();

			/* Setters */
			void reset();

			/* Other methods */
			Object trackFrame(cv::Mat, Object);
	};
}

#endif /* defined I_STUFF_TRACKER_H__ */
