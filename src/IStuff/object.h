/**
 * @file object.h
 * @brief Header file relative to the class IStuff::Object.
 * @author Maurizio Zucchelli
 * @author Mattia Rizzini
 * @version 0.1.1
 * @date 2013-07-16
 */

#ifndef OBJECT_RECOGNIZER_H__
#define OBJECT_RECOGNIZER_H__

#include <iostream>
#include <map>
#include <vector>

#include "opencv2/core/core.hpp"

extern bool debug;

namespace IStuff
{
	/**
	 * @brief Label relative to a view of an Object.
	 */
	typedef struct Label {
		std::string name;
		cv::Point2f position;
		cv::Scalar color;

		Label( std::string _name, cv::Point2f _position, cv::Scalar _color ) : name( _name ), position( _position ), color( _color ) {}

		inline bool operator == ( const Label &o ) const {
			return strcmp( name, o.name ) == 0;
		}
	};

	class Object
	{
		/* Attributes */
		private:
			const static char TAG[];
			
			std::vector< Label > labels;

		/* Methods */
		public:
			/* Constructors and Destructors */
			Object();
			virtual ~Object();
			
			/* Setters */
			void setLabel(const std::string, const cv::Point2f, const cv::Scalar);
			void setLabel(const Label);

			/* Getters */
			bool empty() const;
			std::vector<Label> getLabels() const;

			/* Other methods */
			cv::Mat paint(cv::Mat);
	};
}

#endif /* defined OBJECT_RECOGNIZER_H__ */
