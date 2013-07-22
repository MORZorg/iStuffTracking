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
 * @todo Update description.
 * @brief Changes a IStuff::Label of this IStuff::Object to represent a different mask.
 * @details If the IStuff::Label isn't currently part of the IStuff::Object,
 *	it is added.
 *
 * @param[in] label	the label name
 * @param[in] mask	the label position
 * @param[in] color	the label color
 */
void Object::addLabel(const Label label)
{
	labels.push_back(label);
}

/* Getters */

/**
 * @brief 
 * @todo Describe.
 *
 * @return 
 */
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
 * @brief Paints the various IStuff::Label of the IStuff::Object on the frame.
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

	Mat result = frame.clone();
	for(Label a_label : labels)
	{
		circle( result, a_label.position, 5, a_label.color, 1 );

		putText( result, a_label.name,
						 Point2f( a_label.position.x + 10, a_label.position.y + 10 ),
						 FONT_HERSHEY_DUPLEX, 2, a_label.color, 3 );
	}

	return result;
}

