/**
 * @file main.h
 * @brief Main file's header
 * @author Maurizio Zucchelli
 * @version 0.1.0
 * @date 2013-07-13
 */

#ifndef MAIN_H__
#define MAIN_H__

#include <iostream>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "objdb/object_database.h"
#include "objrec/object_recognizer.h"

bool debug;

int main(int, char**);

void printHelp();

#endif /* defined MAIN_H__ */

