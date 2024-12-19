//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Logger.cpp
 * \brief Logging class
 *
 * Original Author: Stephen Fegan
 * $Author: jperkins $
 * $Date: 2007/01/30 21:39:12 $
 * $Revision: 1.1 $
 * $Tag$
 *
 **/

#include<iostream>
#include<fstream>

#include<zthread/Guard.h>

#include<Logger.hpp>
#include<VATime.h>

using namespace VERITAS;

Logger::~Logger()
{
  // nothing to see here
};

SystemLogger::SystemLogger(const std::string& filename, unsigned max_level):
  m_stream(), m_my_stream(), m_max_level(max_level), m_mutex()
{
  m_my_stream = new std::ofstream(filename.c_str(),
				  std::ios_base::app|std::ios_base::out);
  m_stream = m_my_stream;
}

SystemLogger::SystemLogger(std::ostream& stream, unsigned max_level):
  m_stream(), m_my_stream(), m_max_level(max_level), m_mutex()
{
  m_stream = &stream;
}

SystemLogger::~SystemLogger()
{
  delete(m_my_stream);
}

void SystemLogger::logMessage(unsigned level, const std::string& message)
{
  if(level==0)level=1;
  logSystemMessage(level, message);
}

void SystemLogger::logSystemMessage(unsigned level, const std::string& message)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  if(level<=m_max_level)
    {
      (*m_stream) << VATime::now().toString() << " - " << message << std::endl;
      m_stream->flush();
    }
}
