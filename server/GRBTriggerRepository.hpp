//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GRBTriggerRepository.hpp
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
 * $Revision: 1.3 $
 * $Tag$
 *
 **/

#ifndef GRBTRIGGERTEPOSITORY_HPP
#define GRBTRIGGERTEPOSITORY_HPP

#include<string>
#include<vector>

#include<zthread/RecursiveMutex.h>

#include<NET_VGRBMonitor_data.h>
#include<Logger.hpp>
#include<VATime.h>

class GRBTriggerRepository
{
public:
  GRBTriggerRepository(const std::string filename);
  ~GRBTriggerRepository();
  
  // Clear list ---------------------------------------------------------------

  void clear();

  // Load/save GRB list to/from file. Append one trigger to file --------------

  void loadFromFile();
  void overwriteFile();
  void appendOneTriggerToFile(const GRBTrigger* trigger, Logger* logger=0);

  // Handle retrieval and storage of connection status ------------------------
  
  bool isConnectionUp();
  VERITAS::VATime getLastGCNReceiptTime();
  void setConnectionUp(bool is_up = true);
  void setLastGCNReceiptTime(const VERITAS::VATime& time);

  // Store and retrieve trigger on list ---------------------------------------

  unsigned getNumTriggers();
  GRBTrigger* getTrigger(unsigned itrig);
  void putTrigger(const GRBTrigger* trigger, Logger* logger=0);

  std::string makeName(const GRBTrigger* grb);

private:
  std::string              m_filename;
  ZThread::RecursiveMutex  m_mtx;

  std::vector<GRBTrigger*> m_grbs;
  bool                     m_is_connection_up;
  VERITAS::VATime          m_last_gcn_receipt_time;
};

#endif // defined GRBTRIGGERTEPOSITORY_HPP
