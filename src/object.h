/**
* @file object.h
* @brief Header file relative to the class IStuff::Object.
* @author Maurizio Zucchelli
* @version 0.1.0
* @date 2013-07-16
*/

#ifndef OBJECT_RECOGNIZER_H__
#define OBJECT_RECOGNIZER_H__

#include <iostream>
#include <map>
#include <vector>

#include "opencv2/imgproc/imgproc.hpp"

extern bool debug;

namespace IStuff
{
	/**
	* @brief Label relative to a view of an Object.
	*/
	typedef std::string Label;

	class Object
	{
		/* Attributes */
		private:
		const static char TAG[];

		std::map< Label, std::vector<cv::Point2f> > description;

		/* Methods */
		public:
			/* Constructors and Destructors */
			Object();
			virtual ~Object();

			/* Setters */
			void setLabel(const Label, const std::vector<cv::Point2f>);
			void removeLabel(const Label);

			/* Getters */
			std::vector<cv::Point2f> getMask(const Label) const;
	};
}

#endif /* defined OBJECT_RECOGNIZER_H__ */
