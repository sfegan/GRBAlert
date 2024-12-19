//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GRBTriggerRepository.cpp
 * \ingroup GRB
 * \brief Thread safe storage class for GRB triggers
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/12/05 21:17:20 $
 * $Revision: 1.5 $
 * $Tag$
 *
 **/

#include<zthread/Guard.h>

//#include<VDBBurst.h>
#include<VDB/VDBArrayControl.h>

#include<Logger.hpp>
#include<GRBTriggerRepository.hpp>
#include<GRBTriggerManip.hpp>
#include<Angle.h>
#include<VATime.h>
#include<VSDataConverter.hpp>

using namespace VERITAS;
using namespace SEphem;

GRBTriggerRepository::GRBTriggerRepository(const std::string filename):
  m_filename(filename), m_mtx(),
  m_grbs(), m_is_connection_up(false), m_last_gcn_receipt_time()
{
  loadFromFile();
}

GRBTriggerRepository::~GRBTriggerRepository()
{
  clear();
}

// ----------------------------------------------------------------------------
// Clear list
// ----------------------------------------------------------------------------

void GRBTriggerRepository::clear()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  for(std::vector<GRBTrigger*>::iterator igrb=m_grbs.begin();
      igrb!=m_grbs.end();igrb++)GRBTriggerManip::free(*igrb);
}

// ----------------------------------------------------------------------------
// Load/save GRB list to/from file. Append one trigger to file
// ----------------------------------------------------------------------------

// Coordinate changes with tracking progran

std::string GRBTriggerRepository::makeName(const GRBTrigger* grb)
{
  //RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  VATime t(grb->trigger_time_mjd_int,
	   uint64_t(grb->trigger_msec_of_day_int)*UINT64_C(1000000));
  std::string name = 
    std::string("GRB ")
    + t.getString().substr(0,19)
    + std::string(" ")
    + std::string(grb->trigger_instrument)
    + std::string(" ")
    + VSDataConverter::toString(grb->trigger_gcn_sequence_number)
    + std::string(" ")
    + std::string(grb->trigger_type);
  return name;
}

void GRBTriggerRepository::loadFromFile()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // fill me in...
}

void GRBTriggerRepository::overwriteFile()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // fill me in...
}

void GRBTriggerRepository::appendOneTriggerToFile(const GRBTrigger* trigger,
						  Logger* logger)
{
  // no need to lock

#if 0
  VDBGRB::BurstInfo bi;
  bi.db_time     = 0;
  bi.ra          = 0;
  bi.decl        = 0;
  bi.epoch       = 0;
  bi.instrument  = "";
  bi.trig_type   = "";
  bi.trig_no     = 0;
  bi.trig_time   = 0;
  bi.uncertainty = 0;
#endif

  if((trigger->veritas_should_observe)
     &&(strncmp(trigger->trigger_type,"RETRACTION",10)!=0))
    {
      // Coordinate changes to the fields here with the tracking program !!
      // See GUIGRBMonitor::makeName
      VDBAC::SourceInfo si;

      VATime t(trigger->trigger_time_mjd_int,
	       uint64_t(trigger->trigger_msec_of_day_int)*UINT64_C(1000000));

      std::string source_id = makeName(trigger);

      try
	{
	  VDBAC::putSourceInfo(source_id,
			       Angle::frDeg(trigger->coord_ra_deg),
			       Angle::frDeg(trigger->coord_dec_deg),
			       trigger->coord_epoch_J,
			       "GRBAlert");
	  VDBAC::putSourceinCollection(source_id, "GRB");
	  VDBAC::deleteSourcefromCollection(source_id, "all");
	}
      catch(const VDBException& x)
	{
	  if(logger)
	    {
	      std::ostringstream stream;
	      stream << "Database exception: " << x;
	      logger->logMessage(2,stream.str());
	    }
	}
    }

  // fill me in...
}

// ----------------------------------------------------------------------------
// Handle retrieval and storage of connection status
// ----------------------------------------------------------------------------
  
bool GRBTriggerRepository::isConnectionUp()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return m_is_connection_up;
}

VERITAS::VATime GRBTriggerRepository::getLastGCNReceiptTime()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return m_last_gcn_receipt_time;
}

void GRBTriggerRepository::setConnectionUp(bool is_up)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_is_connection_up = is_up;
}

void GRBTriggerRepository::setLastGCNReceiptTime(const VATime& time)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_last_gcn_receipt_time = time;
}

// ----------------------------------------------------------------------------
// Store and retrieve trigger on list
// ----------------------------------------------------------------------------

unsigned GRBTriggerRepository::getNumTriggers()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return m_grbs.size();
}

GRBTrigger* GRBTriggerRepository::getTrigger(unsigned itrig)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(itrig<m_grbs.size())return GRBTriggerManip::copy(m_grbs[itrig]);
  else return 0;
}

void GRBTriggerRepository::
putTrigger(const GRBTrigger* trigger, Logger* logger)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  GRBTrigger* trigger_copy = GRBTriggerManip::copy(trigger);
  m_grbs.push_back(trigger_copy);
  trigger_copy->veritas_unique_sequence_number = m_grbs.size();
  appendOneTriggerToFile(trigger, logger);
}

