//-*-mode:c++; mode:font-lock;-*-

/**
 * \file grb_monitor.cpp
 * \ingroup GRB
 * \brief Main program for GRB monitor server
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/02/12 18:04:42 $
 * $Revision: 1.4 $
 * $Tag$
 *
 **/

#include<iostream>
#include<sstream>

#include<zthread/Thread.h>

#include<VSOptions.hpp>
#include<VOmniORBHelper.h>
#include<Logger.hpp>
#include<Daemon.h>

#include<GRBTriggerRepository.hpp>
#include<GCNAcquisitionLoop.hpp>
#include<GRBMonitorServant.hpp>
#include<NET_VGRBMonitor.h>

static const char*const VERSION =
  "$Id: grb_monitor.cpp,v 1.4 2007/02/12 18:04:42 sfegan Exp $";

using namespace VCorba;
using namespace VERITAS;

void usage(const std::string& progname, const VSOptions& options,
           std::ostream& stream)
{
  stream << "Usage: " << progname
         << " [options]"
         << std::endl
         << std::endl;
  stream << "Options:" << std::endl;
  options.printUsage(stream);
}

int main(int argc, char** argv)
{
  std::string program(*argv);

  // --------------------------------------------------------------------------
  // Command line options
  // --------------------------------------------------------------------------

  VSOptions options(argc,argv,true);

  // Set up variables ---------------------------------------------------------

  bool                      print_usage          = false;

  bool                      daemon               = true;
  std::string               daemon_lockfile      = "lock.dat";
  unsigned                  logger_level         = 5;
  std::string               logger_filename      = "log.dat";

  std::string               corba_nameserver     = "corbaname::db.vts";
  std::vector<std::string>  corba_args;
  int                       corba_port           = -1;

  unsigned                  gcn_port             = 5226;
  unsigned                  gcn_timeout_sec      = 600;
  bool                      gcn_test_messages    = false;

  std::string               repository_filename  = "grb.dat";

  // Fill variables from command line parameters ------------------------------

  if(options.find("h","Print this message.")!=VSOptions::FS_NOT_FOUND)
    print_usage=true;
  if(options.find("help","Print this message.")!=VSOptions::FS_NOT_FOUND)
    print_usage=true;

  if(options.find("no_daemon","Do not go into daemon mode.")
     !=VSOptions::FS_NOT_FOUND)daemon=false;
  options.findWithValue("daemon_lockfile",daemon_lockfile,
			"Set the name of the lock file, if in daemon mode");
  options.findWithValue("logger_level",logger_level,
			"Set the maximum message level to log. Higher values "
			"produce more verbose logs.");
  options.findWithValue("logger_filename",logger_filename,
			"Set the name of the log file, if in daemon mode");

  options.findWithValue("corba_nameserver",corba_nameserver,
			"Set the IOR of the CORBA nameserver. The most "
			"convenient form to give this in is as a CORBANAME, "
			"e.g. \"corbaname::db.vts\".");
  options.findWithValue("corba_args",corba_args,
			"Specifiy a comma seperated list of arguments to "
			"send to OmniORB. See the OmniORB manual for the "
			"list of command line arguements it accepts");
  options.findWithValue("corba_port",corba_port,
			"Try to bind to a specific port for CORBA "
			"communication.");

  options.findWithValue("gcn_port",gcn_port,
			"Try to bind to a specific port for connections from "
			"the GCN.");
  options.findWithValue("gcn_timeout_sec",gcn_timeout_sec,
			"Timeout in seconds while eaiting for data from GCN.");
  if(options.find("gcn_test_messages","Process GCN test messages.")
     !=VSOptions::FS_NOT_FOUND)gcn_test_messages=true;

  options.findWithValue("repository_filename",repository_filename,
			"Set the name of the GRB repository file name.");

  corba_args.insert(corba_args.begin(), program);

  // Ensure all options have been handled -------------------------------------

  if(!options.assertNoOptions())
    {
      std::cerr << program << ": unknown options: ";
      for(int i=1;i<argc;i++)
        if(*(argv[i])=='-') std::cerr << ' ' << argv[i];
      std::cerr << std::endl;
      usage(program, options, std::cerr);
      exit(EXIT_FAILURE);
    }

  argv++,argc--;

  // Print usage if requested -------------------------------------------------

  if(print_usage)
    {
      usage(program, options, std::cout);
      exit(EXIT_SUCCESS);
    }

  // --------------------------------------------------------------------------
  // Go daemon
  // --------------------------------------------------------------------------

  if(daemon)
    {
      // Get a temporary lock on the lockfile before daemon_init so we
      // can print out an error - the lock is lost at the fork in
      // daemon_init, so we get it again after the logger is started

      int fd;
      if(!VTaskNotification::Daemon::lock_file(daemon_lockfile,&fd))
	{
	  std::cerr << "Could lock: " << daemon_lockfile 
		    << ", terminating" << std::endl;
	  exit(EXIT_FAILURE);
	}
      close(fd);

      // The daemon_init function will change directory to / so watch
      // out for the file names given on the command line. Put CWD at
      // the beginning if necessary.

      char cwd_buffer[1000];
      assert(getcwd(cwd_buffer,sizeof(cwd_buffer)) != NULL);
      std::string cwd = std::string(cwd_buffer)+std::string("/");
      if(daemon_lockfile[0]!='/')daemon_lockfile.insert(0,cwd);
      if(logger_filename[0]!='/')logger_filename.insert(0,cwd);
      if(repository_filename[0]!='/')repository_filename.insert(0,cwd);

      // Go daemon...

      VTaskNotification::Daemon::daemon_init("/",true);
    }

  // --------------------------------------------------------------------------
  // Set up the debug logger
  // --------------------------------------------------------------------------

  SystemLogger* logger;
  if(daemon)logger = new SystemLogger(logger_filename,logger_level);
  else logger = new SystemLogger(std::cout,logger_level);

  logger->logSystemMessage(0,"GRB monitor starting");
  logger->logSystemMessage(0,VERSION);

  // --------------------------------------------------------------------------
  // Try to acquire lock on daemon_lockfile
  // --------------------------------------------------------------------------

  if(daemon)
    {
      if(VTaskNotification::Daemon::lock_file(daemon_lockfile))
	{
	  logger->logSystemMessage(0,std::string("Locked: ")+daemon_lockfile);
	}
      else
	{
	  logger->
	    logSystemMessage(0,std::string("Could not lock: ")+
			     daemon_lockfile+std::string(", terminating"));
	  exit(EXIT_FAILURE);
	}
    }

  // --------------------------------------------------------------------------
  // Create the GRB repository
  // --------------------------------------------------------------------------

  GRBTriggerRepository* repository = 
    new GRBTriggerRepository(repository_filename);

  // --------------------------------------------------------------------------
  // Fire up CORBA
  // --------------------------------------------------------------------------

  int omni_argc = corba_args.size();
  char** omni_argv = new char*[omni_argc+1];
  std::vector<char*> omni_args(omni_argc+1);

  for(int iarg=0;iarg<omni_argc;iarg++)
    {
      omni_args[iarg] = omni_argv[iarg] =
        new char[corba_args[iarg].length()+1];
      strcpy(omni_argv[iarg], corba_args[iarg].c_str());
    }

  omni_args[omni_argc] = omni_argv[omni_argc] = 0;

  VOmniORBHelper* orb = new VOmniORBHelper;
  orb->orbInit(omni_argc, omni_argv, corba_nameserver.c_str(), corba_port);

  // --------------------------------------------------------------------------
  // Create the CORBA servant, activate it and register it with the NS
  // --------------------------------------------------------------------------

  GRBMonitorServant* grb_monitor_servant = new GRBMonitorServant(repository);
  CORBA::Object_var net_grb_monitor;
  
  try
    {
      net_grb_monitor =
	orb->poaActivateObject(grb_monitor_servant, 
			       VGRBMonitor::progName,
			       VGRBMonitor::Command::objName);
    }
  catch(const CORBA::Exception& x)
    {
      std::ostringstream stream;
      stream << "CORBA exception while trying to activate the server: "
	     << x._name();
      logger->logMessage(1,stream.str());
      exit(EXIT_FAILURE);
    }

  try
    {
      orb->nsRegisterObject(net_grb_monitor,
			    VGRBMonitor::progName,
			    VGRBMonitor::Command::objName);
    }
  catch(const CORBA::Exception& x)
    {
      std::ostringstream stream;
      stream << "CORBA exception while trying to register the nameserver: "
	     << x._name();
      logger->logMessage(1,stream.str());
      exit(EXIT_FAILURE);
    }

  // --------------------------------------------------------------------------
  // Start the GCN acquisition thread
  // --------------------------------------------------------------------------
  
  GCNAcquisitionLoop* gcn_acq = 
    new GCNAcquisitionLoop(repository,logger,gcn_port,gcn_timeout_sec,
			   gcn_test_messages);
  ZThread::Thread* gcn_acq_thread = new ZThread::Thread(gcn_acq);

  // --------------------------------------------------------------------------
  // Run the ORB - this call never ends
  // --------------------------------------------------------------------------

  orb->orbRun();

  // --------------------------------------------------------------------------
  // Clean everything up (this never happens)
  // --------------------------------------------------------------------------

  // somehow terminate the GCN acq loop
  gcn_acq_thread->wait();

  delete grb_monitor_servant;
  delete orb;

  // the acqusistion object is automatically deleted 

  delete repository;

  for(unsigned iarg=0;iarg<omni_args.size();iarg++)delete[] omni_args[iarg];
  delete[] omni_argv;

  delete logger;
}
