//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Logger.hpp
 * \brief Logging class
 *
 * Original Author: Stephen Fegan
 * $Author: jperkins $
 * $Date: 2007/01/30 21:39:12 $
 * $Revision: 1.1 $
 * $Tag$
 *
 **/

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include<zthread/RecursiveMutex.h>

#include<string>

class Logger
{
public:
  virtual ~Logger();
  virtual void logMessage(unsigned level, const std::string& message) = 0;
};

class SystemLogger: public Logger
{
public:
  SystemLogger(const std::string& filename, unsigned max_level=0);
  SystemLogger(std::ostream& stream, unsigned max_level=0);
  virtual ~SystemLogger();
  virtual void logMessage(unsigned level, const std::string& message);
  virtual void logSystemMessage(unsigned level, const std::string& message);
private:
  std::ostream*              m_stream;
  std::ofstream*             m_my_stream;
  unsigned                   m_max_level;
  ZThread::RecursiveMutex    m_mutex;
};

#endif // defined LOGGER_HPP
