/**
 * @file recognizer.h
 * @brief Header file relative to the class IStuff::Recognizer.
 * @author Maurizio Zucchelli
 * @version 0.2.1
 * @date 2013-07-14
 */

#ifndef I_STUFF_RECOGNIZER_H__
#define I_STUFF_RECOGNIZER_H__

#include <iostream>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include "opencv2/imgproc/imgproc.hpp"

#include "object.h"
#include "database.h"

extern bool debug;

namespace IStuff
{
  class Manager;

  class Recognizer
  {
    /* Attributes */
    private:
      const static char TAG[];

      std::auto_ptr<boost::thread> m_thread;
      bool m_running;

      Database* m_matcher;

      /* Methods */
    public:
      /* Constructors and Destructors */
      Recognizer();
      virtual ~Recognizer();

      /* Setters */
      void setDatabase(Database*);

      /* Getters */
      bool isRunning() const;

      /* Other methods */
      Object recognizeFrame(cv::Mat);
      bool backgroundRecognizeFrame(cv::Mat, Manager*);

      void sendMessage(int, void*, void* = NULL);

    private:
      /* Setters */
      void setRunning(bool);
  };
}

#include "manager.h"

#endif /* defined I_STUFF_RECOGNIZER_H__ */

