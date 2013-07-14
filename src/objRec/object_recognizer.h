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
#include <thread>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

extern debug;

class ObjectDatabase;
class ObjectTracker;

class ObjectRecognizer
{
	/* Attributes */
	private:
		std::thread::id running;

		VideoCapture*		source;
		ObjectDatabase*	matcher;
		ObjectTracker*	callback;

	/* Methods */
	public:
		/* Constructors and Destructors */
		ObjectRecognizer(VideoCapture*, ObjectDatabase*);
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
		void recognizeFrame(Mat);
}

#endif /* Defined OBJECT_RECOGNIZER_H__ */
