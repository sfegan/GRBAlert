//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GCNAcquisitionLoop.hpp
 * \ingroup GRB
 * \brief Communication with GCN, runs as seprrate thread.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: jperkins $
 * $Date: 2008/12/02 21:25:43 $
 * $Revision: 1.5 $
 * $Tag$
 *
 **/

#ifndef GCNACQUISITIONLOOP_HPP
#define GCNACQUISITIONLOOP_HPP

#include<zthread/Runnable.h>

#include<Logger.hpp>
#include<GRBTriggerRepository.hpp>

class GCNAcquisitionLoop: public ZThread::Runnable
{
public:
  GCNAcquisitionLoop(GRBTriggerRepository* grbs, Logger* logger,
		     unsigned short port, unsigned conn_timeout_sec = 600,
		     bool pass_test_messages = false);

  virtual ~GCNAcquisitionLoop();

  // Functions from ZThread::Runnable -----------------------------------------
  
  virtual void run();

private:
  GRBTrigger* handle_gcn_packets(const int32_t* buffer);

  GRBTrigger* T_test(const int32_t* buffer);
  GRBTrigger* T_hete(const int32_t* buffer);
  GRBTrigger* T_integral(const int32_t* buffer);
  GRBTrigger* T_swift(const int32_t* buffer);
  GRBTrigger* T_fermi(const int32_t* buffer);
  GRBTrigger* T_agile(const int32_t* buffer);
  GRBTrigger* T_usertest(const int32_t* buffer);
 

  GRBTriggerRepository*     m_grbs;
  Logger*                   m_logger;

  unsigned short            m_port;
  unsigned                  m_conn_timeout_sec;

  bool                      m_test_messages;
};

#endif // defined GCNACQUISITIONLOOP_HPP
