//-*-mode:c++; mode:font-lock;-*-

/**
 * \file grb_alert.cpp
 * \ingroup GRB
 * \brief Client (daemon!) program to run some arbitrary command on 
 * receipt of an observable GRB
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/12/05 18:45:29 $
 * $Revision: 1.5 $
 * $Tag$
 *
 **/

#include<iostream>
#include<sstream>
#include<cerrno>
#include<signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include<zthread/Thread.h>

#include<VSOptions.hpp>
#include<VOmniORBHelper.h>
#include<Logger.hpp>
#include<Daemon.h>
#include<PhaseLockedLoop.h>

#include<GRBTriggerRepository.hpp>
#include<GCNAcquisitionLoop.hpp>
#include<GRBMonitorServant.hpp>
#include<NET_VGRBMonitor.h>

static const char*const VERSION =
  "$Id: grb_alert.cpp,v 1.5 2007/12/05 18:45:29 sfegan Exp $";

using namespace VCorba;
using namespace VTaskNotification;
using namespace VERITAS;

class GRBAlerter: public PhaseLockedLoop
{
public:
  GRBAlerter(VOmniORBHelper* orb, Logger* logger, const std::string& command,
	     bool daemon, bool test);
  virtual ~GRBAlerter();
  virtual void iterate();
private:
  static void childHandler(int signum);

  VOmniORBHelper*          m_orb;
  Logger*                  m_logger;
  const std::string&       m_command;
  bool                     m_daemon;
  bool                     m_test;
  VGRBMonitor::Command_ptr m_monitor;
  CORBA::ULong             m_seq_no;
};

GRBAlerter::
GRBAlerter(VOmniORBHelper* orb, Logger* logger, const std::string& command,
	   bool daemon, bool test):
  PhaseLockedLoop(1000), m_orb(orb), m_logger(logger), m_command(command),
  m_daemon(daemon), m_test(test), m_monitor(), m_seq_no()
{
  struct sigaction chldact;
  chldact.sa_handler   = childHandler;
  sigemptyset(&chldact.sa_mask);
  chldact.sa_flags     = SA_NOCLDSTOP;
  sigaction(SIGCHLD, &chldact, static_cast<struct sigaction *>(0));

  // nothing to see here
}

void GRBAlerter::childHandler(int signum)
{
  while(waitpid(-1,0,WNOHANG)>0);
}

GRBAlerter::~GRBAlerter()
{
  // nothing to see here
}

void GRBAlerter::iterate()
{
  try
    {
      bool just_connected = false;
       if(m_monitor == 0)
	{
	  just_connected = true;

	  m_monitor =
	    m_orb->nsGetNarrowedObject<VGRBMonitor::Command>
	    (VGRBMonitor::progName, VGRBMonitor::Command::objName);
	}

      // ----------------------------------------------------------------------
      // Loop getting indices until we are caught up
      // ----------------------------------------------------------------------

      bool alert = false;

      CORBA::ULong next_seq_no =
	m_monitor->nGetNextTriggerSequenceNumber(m_seq_no);

      while(next_seq_no != 0)
	{
	  m_seq_no = next_seq_no;

	  if((!just_connected)&&(!alert))
	    {
	      GRBTrigger_var trigger = m_monitor->nGetOneTrigger(next_seq_no);
	      if(trigger->veritas_should_observe)alert=true;
	    }
	  
	  next_seq_no = 
	    m_monitor->nGetNextTriggerSequenceNumber(next_seq_no);
	}

      // ----------------------------------------------------------------------
      // Do alert if necessary (if alert issued or test requested at start)
      // ----------------------------------------------------------------------

      if(just_connected)
	m_logger->logMessage(0,"Testing Alert System");

      if(alert || (just_connected && m_test) )
	{
	  m_logger->logMessage(0,"GRB Audible Alert Issued!");
	  int pid = fork();
	  if(pid<0)
	    {
	      m_logger->logMessage(1,std::string("fork: ")
				   + std::string(strerror(errno)));
	    }
	  else if(pid==0)
	    {
	      if(m_daemon)
		{
		  int nfd = sysconf(_SC_OPEN_MAX);
		  if(nfd<0)nfd=256;
		  for(int ifd=0; ifd<nfd; ifd++)close(ifd);
		  int fd = open("/dev/null",O_RDWR);
		  if(fd!=0)dup2(fd,0);
		  if(fd!=1)dup2(fd,1);
		  if(fd!=2)dup2(fd,2);
		}
	      execl("/bin/sh",
		    "sh","-c",m_command.c_str(),static_cast<char*>(0));
	      m_logger->logMessage(0,"GRB Audible Alert Issued!");
	      exit(EXIT_SUCCESS);
	    }
	}
    }
  catch(const CosNaming::NamingContext::NotFound)
    {
      m_logger->logMessage(2,"caught: CosNaming::NamingContext::NotFound");
      if(m_monitor)CORBA::release(m_monitor), m_monitor=0, m_seq_no=0;	  
    }
  catch(const CORBA::TRANSIENT& x)
    {
      m_logger->logMessage(2,"caught: CORBA::TRANSIENT");
      if(m_monitor)CORBA::release(m_monitor), m_monitor=0, m_seq_no=0;	  
    }
  catch(const CORBA::Exception& x)
    {
      m_logger->logMessage(2,std::string("caught: CORBA::Exception ")
			   + std::string(x._name()));
      if(m_monitor)CORBA::release(m_monitor), m_monitor=0, m_seq_no=0;	  
    }
  catch(...)
    {
      m_logger->logMessage(2,"caught: unknown exception");
      if(m_monitor)CORBA::release(m_monitor), m_monitor=0, m_seq_no=0;	  
    }
}

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
  unsigned                  logger_level         = 1;
  std::string               logger_filename      = "log.dat";

  std::string               corba_nameserver     = "corbaname::db.vts";
  std::vector<std::string>  corba_args;

  std::string               command              = "echo grb alert!";

  bool                      test                 = false;

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

  options.findWithValue("command",command,
			"Specify the command to run on receipt of a GRB.");

  options.findWithValue("test",test,
			"Test audio with initial fake alert.");

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

      if(!daemon_lockfile.empty())
	{
	  int fd;
	  if(!VTaskNotification::Daemon::lock_file(daemon_lockfile,&fd))
	    {
	      std::cerr << "Could lock: " << daemon_lockfile 
			<< ", terminating" << std::endl;
	      exit(EXIT_FAILURE);
	    }
	  close(fd);
	}

      // The daemon_init function will change directory to / so watch
      // out for the file names given on the command line. Put CWD at
      // the beginning if necessary.

      char cwd_buffer[1000];
      assert(getcwd(cwd_buffer,sizeof(cwd_buffer)) != NULL);
      std::string cwd = std::string(cwd_buffer)+std::string("/");
      if(daemon_lockfile[0]!='/')daemon_lockfile.insert(0,cwd);
      if(logger_filename[0]!='/')logger_filename.insert(0,cwd);

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

  if((daemon)&&(!daemon_lockfile.empty()))
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
  orb->orbInit(omni_argc, omni_argv, corba_nameserver.c_str());

  // --------------------------------------------------------------------------
  // Create the GRB alerter
  // --------------------------------------------------------------------------

  GRBAlerter* alerter = new GRBAlerter(orb, logger, command, daemon, test);

  // --------------------------------------------------------------------------
  // Run the alerter (this never finishes)
  // --------------------------------------------------------------------------

  alerter->run();

  // --------------------------------------------------------------------------
  // Clean everything up (this never happens)
  // --------------------------------------------------------------------------

  // somehow terminate the GCN acq loop

  delete alerter;
  delete orb;

  for(unsigned iarg=0;iarg<omni_args.size();iarg++)delete[] omni_args[iarg];
  delete[] omni_argv;

  delete logger;
}
