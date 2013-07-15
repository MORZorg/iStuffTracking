/**
 * @file object_recognizer.h
 * @brief Header file relative to the class ObjectRecognizer.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-14
 */

#ifndef OBJECT_RECOGNIZER_H__
#define OBJECT_RECOGNIZER_H__

#include <iostream>
#include <boost/thread.hpp>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

extern bool debug;

class ObjectDatabase;
class ObjectTracker;

class ObjectRecognizer
{
	/* Attributes */
	private:
		const static char TAG[];

		boost::thread* running;

		cv::VideoCapture*		source;
		ObjectDatabase*	matcher;
		ObjectTracker*	callback;

	/* Methods */
	public:
		/* Constructors and Destructors */
		ObjectRecognizer(cv::VideoCapture*, ObjectDatabase*);
		ObjectRecognizer(cv::VideoCapture*);
		virtual ~ObjectRecognizer();
		
		/* Setters */
		void setTracker(ObjectTracker*);

		/* Getters */
		bool isRunning();

		/* Other methods */
		void start();
		void stop();

	private:
		void run();
		void recognizeFrame(cv::Mat);
};

#endif /* Defined OBJECT_RECOGNIZER_H__ */
