/**
 * @file recognizer.h
 * @brief Header file relative to the class IStuff::Recognizer.
 * @author Maurizio Zucchelli
 * @version 0.2.0
 * @date 2013-07-14
 */

#ifndef OBJECT_RECOGNIZER_H__
#define OBJECT_RECOGNIZER_H__

#include <iostream>

#include <boost/thread.hpp>

#include "opencv2/imgproc/imgproc.hpp"

extern bool debug;

namespace IStuff
{
	class Manager;
	class Database;
	
	class Recognizer
	{
		/* Attributes */
		private:
			const static char TAG[];

			boost::thread* running;

			Database* matcher;

		/* Methods */
		public:
			/* Constructors and Destructors */
			Recognizer();
			virtual ~Recognizer();
			
			/* Setters */
			void setDatabase(Database*);

			/* Getters */
			bool isRunning() const;

			/* Other methods */
			void recognizeFrame(cv::Mat, Manager*);
			bool backgroundRecognizeFrame(cv::Mat, Manager*);

		private:
	};
}

#endif /* defined OBJECT_RECOGNIZER_H__ */
