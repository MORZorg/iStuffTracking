/**
 * @file object.cpp
 * @class	IStuff::Object
 * @brief Class used to represent a three dimensional object.
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-16
 */

#include "object.h"

using namespace std;
using namespace cv;
using namespace IStuff;

const char Object::TAG[] = "Obj";

/* Constructors and Destructors */

/**
 * @brief Constructs a new object.
 */
Object::Object()
{}

Object::~Object()
{}

/* Setters */

/**
 * @brief Changes a IStuff::Label of this IStuff::Object to represent a different mask.
 * @details If the IStuff::Label isn't currently part of the IStuff::Object,
 *	it is added.
 *
 * @param[in] label	the label name
 * @param[in] mask	the label position
 * @param[in] color	the label color
 */
void Object::setLabel(const string label, const Point2f mask, const Scalar color)
{
	labels.push_back( Label( label, mask, color ) );
}

/* Getters */

bool Object::empty() const
{
	return labels.empty();
}

/**
 * @brief Returns a list of all the IStuff::Label associated to this object.
 *
 * @return A std::vector containing the IStuff::Label.
 */
vector<Label> Object::getLabels() const
{
	return labels;
}

/* Other methods */

/**
 * @brief Paints the various masks of the IStuff::Object on the frame.
 *
 * @param[in] frame The frame on which the IStuff::Object must be painted.
 *
 * @return A copy of the input frame, with the IStuff::Object painted on it.
 */
Mat Object::paint(Mat frame)
{
	if (labels.empty())
		return frame;

	int vertCount = 1;

	//for (auto label : description)
	for( vector< Label >::iterator label = labels.begin(); label != labels.end(); label++ )
	{
		circle( frame, (*label).position, 5, (*label).color, 2 );

		putText( frame, ( *label ).name, (*label).position, FONT_HERSHEY_PLAIN, 1, ( *label ).color, 2 );
	}

	return frame;
}

