//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GRBMonitorServant.cpp
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

#include<VATime.h>

#include<GRBTriggerManip.hpp>
#include<GRBMonitorServant.hpp>

using namespace VERITAS;

GRBMonitorServant::~GRBMonitorServant()
{
  // nothing to see here
}

void GRBMonitorServant::nAlive()
{
  // nothing to see here
}

void GRBMonitorServant::
nGetStatus(CORBA::Boolean& gcn_connection_is_up,
	   CORBA::ULong& time_since_last_gcn_receipt_sec,
	   CORBA::ULong& server_uptime_sec)
{
  VATime now = VATime::now();
  gcn_connection_is_up = m_grbs->isConnectionUp();
  VATime last_gcn_receipt = m_grbs->getLastGCNReceiptTime();
  time_since_last_gcn_receipt_sec = 
    CORBA::ULong((now-last_gcn_receipt)/INT64_C(1000000000));
  server_uptime_sec =
    CORBA::ULong((now-m_server_start)/INT64_C(1000000000));
}

CORBA::ULong GRBMonitorServant::
nGetNextTriggerSequenceNumber(CORBA::ULong first_sequence_number)
{
  unsigned ntrig = m_grbs->getNumTriggers();
  if(first_sequence_number>=ntrig)return 0;
  unsigned itrig = first_sequence_number+1;
#if 0
  std::cout << __PRETTY_FUNCTION__ << ' ' 
	    << first_sequence_number << ' ' << ntrig << ' ' << itrig << '\n';
#endif
  return itrig;
}

GRBTrigger* GRBMonitorServant::
nGetOneTrigger(CORBA::ULong unique_sequence_number)
{
  unsigned ntrig = m_grbs->getNumTriggers();
  if(unique_sequence_number>ntrig)throw GRBNoSuchTrigger();
#if 0
  std::cout << __PRETTY_FUNCTION__ << ' ' 
	    << unique_sequence_number << ' ' << ntrig << '\n';
#endif
  return m_grbs->getTrigger(unique_sequence_number-1);
}

void GRBMonitorServant::nInsertOneTrigger(const GRBTrigger& trigger)
{
  m_grbs->putTrigger(&trigger);
}
