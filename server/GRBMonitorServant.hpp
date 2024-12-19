//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GRBMonitorServant.hpp
 * \ingroup GRB
 * \brief Servant to distribute GRB information ofver CORBA
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: jperkins $
 * $Date: 2007/01/30 21:39:12 $
 * $Revision: 1.1 $
 * $Tag$
 *
 **/

#ifndef GRBMONITORSERVANT_HPP
#define GRBMONITORSERVANT_HPP

#include<NET_VGRBMonitor.h>
#include<GRBTriggerRepository.hpp>

class GRBMonitorServant: 
  public POA_VGRBMonitor::Command,   
  public PortableServer::RefCountServantBase
{
public:
  GRBMonitorServant(GRBTriggerRepository* grbs):
    POA_VGRBMonitor::Command(), PortableServer::RefCountServantBase(),
    m_grbs(grbs), m_server_start(VERITAS::VATime::now()) 
  { /* nothing to see here */ }

  virtual ~GRBMonitorServant();

  // Functions from CORBA IDL file --------------------------------------------

  virtual void nAlive();

  virtual void nGetStatus(CORBA::Boolean& gcn_connection_is_up,
			  CORBA::ULong& time_since_last_gcn_receipt_sec,
			  CORBA::ULong& server_uptime_sec);

  virtual CORBA::ULong 
  nGetNextTriggerSequenceNumber(CORBA::ULong first_sequence_number);

  virtual GRBTrigger* nGetOneTrigger(CORBA::ULong unique_sequence_number);

  virtual void nInsertOneTrigger(const GRBTrigger& trigger);

private:
  GRBTriggerRepository*     m_grbs;
  VERITAS::VATime           m_server_start;
};

#endif // defined GRBMONITORSERVANT_HPP
