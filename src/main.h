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

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "IStuff/manager.h"

bool debug,
		 hl_debug;

int main( int, char** );

void printHelp();

#endif /* defined MAIN_H__ */

