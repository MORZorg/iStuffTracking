/**
* @file object.cpp
* @class IStuff::Object
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
Object::Object() {
}

Object::~Object() {
}

/* Setters */

/**
* @brief Changes a Label of this Object to represent a different mask.
* @details If the Label isn't currently part of the Object, it is added.
*
* @param[in] label The label to be changed or added.
* @param[in] mask The new mask to be associated.
*/
void Object::setLabel(const Label label, const vector<Point2f> mask)
{
	description[label] = mask;
}

/**
* @brief Removes a Label from this Object.
*
* @param[in] label
*/
void Object::removeLabel(const Label label)
{
	description.erase(label);
}

/* Getters */

/**
* @brief Returns the mask associated to a Label.
* @throw out_of_range If the Label searched does not exist.
*
* @param[in] label The Label to search for.
*
* @return The mask associated to the input Label.
*/
vector<Point2f> Object::getMask(const Label label) const
{
	return description.at(label);
}
