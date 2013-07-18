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
 * @param[in] label	the IStuff::Label to be changed or added.
 * @param[in] mask	The new mask to be associated.
 */
void Object::setLabel(const Label label, const vector<Point2f> mask)
{
	description[label] = mask;
}

/**
 * @brief Removes a IStuff::Label from this IStuff::Object.
 *
 * @param[in] label
 */
void Object::removeLabel(const Label label)
{
	description.erase(label);
}

/* Getters */

/**
 * @brief Returns the mask associated to a IStuff::Label.
 * @throw std::out_of_range If the IStuff::Label searched does not exist.
 *
 * @param[in] label	The IStuff::Label to search for.
 *
 * @return The mask associated to the input IStuff::Label.
 */
vector<Point2f> Object::getMask(const Label label) const
{
	return description.at(label);
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
	if (description.empty())
		return frame;

	for (auto label : description)
	{
		vector<Point2f> mask = label.second;

		for (int i = 0; i < mask.size() - 1; i++ )
			line(frame, mask[i], mask[i+1], Scalar(0, 255, 0), 4);

		line(frame, mask.back(), mask.front(), Scalar(0, 255, 0), 4);
	}

	return frame;
}

