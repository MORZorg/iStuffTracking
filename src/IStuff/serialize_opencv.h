/**
* @file serialize_opencv.h
* @brief Serialization support for various opencv classes thought boost
* @author Mattia Rizzini
* @version 0.1.1
* @date 2013-07-15
*/
#include "boost/serialization/serialization.hpp"
#include "boost/serialization/split_free.hpp"
#include "boost/serialization/vector.hpp"

BOOST_SERIALIZATION_SPLIT_FREE( cv::Mat )
namespace boost {
	namespace serialization {
		// Serialization for the cv::Mat class
		template< class Archive >
		void save( Archive & ar, const cv::Mat& m, const unsigned int version )
		{
			size_t elemSize = m.elemSize(), elemType = m.type();

			ar & m.cols;
			ar & m.rows;
			ar & elemSize;
			ar & elemType;
			size_t dataSize = m.cols * m.rows * m.elemSize();

			for( size_t dc = 0; dc < dataSize; ++dc )
				ar & m.data[ dc ];
		}

		template< class Archive >
		void load( Archive & ar, cv::Mat& m, const unsigned int version )
		{
			int cols, rows;
			size_t elemSize, elemType;

			ar & cols;
			ar & rows;
			ar & elemSize;
			ar & elemType;

			m.create( rows, cols, elemType );
			size_t dataSize = m.cols * m.rows * elemSize;

			for( size_t dc = 0; dc < dataSize; ++dc )
				ar & m.data[ dc ];
		}
	}
}

// If you can get this to work then I hate you
/*BOOST_SERIALIZATION_SPLIT_FREE( cv::Point_<float> )
namespace boost {
	namespace serialization {
		// Serialization for the cv::Point2f class
		template< class Archive >
		void save( Archive& ar, const cv::Point_<float>& p, const unsigned int version )
		{
			ar & p.x;
			ar & p.y;
		}

		template< class Archive >
		void load( Archive& ar, const cv::Point_<float>& p, const unsigned int version )
		{
			ar & p.x;
			ar & p.y;
		}
	}
}

BOOST_SERIALIZATION_SPLIT_FREE( cv::KeyPoint )
namespace boost {
	namespace serialization {
		// Serialization for the cv::Keypoint class
		template< class Archive >
		void save( Archive& ar, const cv::KeyPoint& p, const unsigned int version )
		{
			ar & p.pt.x;
			ar & p.pt.y;
			ar & p.size;
			ar & p.angle;
			ar & p.response;
			ar & p.octave;
			ar & p.class_id;
		}

		template< class Archive >
		void load( Archive& ar, const cv::KeyPoint& p, const unsigned int version )
		{
			ar & p.pt.x;
			ar & p.pt.y;
			ar & p.size;
			ar & p.angle;
			ar & p.response;
			ar & p.octave;
			ar & p.class_id;
		}
	}
}*/
