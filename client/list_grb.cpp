//-*-mode:c++; mode:font-lock;-*-

/**
 * \file list_grb.cpp
 * \ingroup GRB
 * \brief Connect to server and list GRBs
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/02/09 21:23:16 $
 * $Revision: 1.1 $
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
#include<VATime.h>

#include<NET_VGRBMonitor.h>

static const char*const VERSION =
  "$Id: list_grb.cpp,v 1.1 2007/02/09 21:23:16 sfegan Exp $";

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

  std::string               corba_nameserver     = "corbaname::db.vts";
  std::vector<std::string>  corba_args;

  // Fill variables from command line parameters ------------------------------

  if(options.find("h","Print this message.")!=VSOptions::FS_NOT_FOUND)
    print_usage=true;
  if(options.find("help","Print this message.")!=VSOptions::FS_NOT_FOUND)
    print_usage=true;

  options.findWithValue("corba_nameserver",corba_nameserver,
			"Set the IOR of the CORBA nameserver. The most "
			"convenient form to give this in is as a CORBANAME, "
			"e.g. \"corbaname::db.vts\".");
  options.findWithValue("corba_args",corba_args,
			"Specifiy a comma seperated list of arguments to "
			"send to OmniORB. See the OmniORB manual for the "
			"list of command line arguements it accepts");

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

  try
    {
      // ----------------------------------------------------------------------
      // Get a reference to the server
      // ----------------------------------------------------------------------

      VGRBMonitor::Command_var
	grb_monitor =
	orb->nsGetNarrowedObject<VGRBMonitor::Command>
	(VGRBMonitor::progName, VGRBMonitor::Command::objName);

#if 0
      grb_monitor->nGetStatus(gcn_connection_is_up,
                                time_since_last_gcn_receipt_sec,
                                server_uptime_sec);
#endif

      // ----------------------------------------------------------------------
      // Loop getting GRB data and printing it
      // ----------------------------------------------------------------------

      CORBA::ULong next_id =
        grb_monitor->nGetNextTriggerSequenceNumber(0);
      while(next_id != 0)
        {
          GRBTrigger_var trigger = grb_monitor->nGetOneTrigger(next_id);

	  VATime r(trigger->veritas_receipt_mjd_int,
		   uint64_t(trigger->veritas_receipt_msec_of_day_int)
		   * UINT64_C(1000000));

	  VATime t(trigger->trigger_time_mjd_int,
		   uint64_t(trigger->trigger_msec_of_day_int)
		   * UINT64_C(1000000));

	  std::cout << std::left
		    << std::setw(10) << std::setprecision(10) 
		    << trigger->trigger_instrument << ' '
		    << std::setw(16) << std::setprecision(16) 
		    << trigger->trigger_type << ' '
		    << std::setw(7) << std::setprecision(7)
		    << trigger->trigger_gcn_sequence_number << ' '
		    << t.toString() << ' '
		    << std::right
		    << std::fixed << std::setw(7) << std::setprecision(2)
		    << double(r-t)/1000000000.0 << ' '
		    << std::fixed << std::setw(8) << std::setprecision(4)
		    << trigger->coord_ra_deg << ' '
		    << std::fixed << std::setw(8) << std::setprecision(4)
		    << trigger->coord_dec_deg << ' '
		    << std::fixed << std::setw(6) << std::setprecision(1)
		    << trigger->coord_epoch_J << ' '
		    << std::fixed << std::setw(6) << std::setprecision(4)
		    << trigger->coord_error_circle_deg << ' '
		    << (trigger->veritas_should_observe?"YES":"NO") << '\n';
	  
          next_id = grb_monitor->nGetNextTriggerSequenceNumber(next_id);
        }
    }
  catch(const CosNaming::NamingContext::NotFound)
    {
      std::cerr << "caught: CosNaming::NamingContext::NotFound" << std::endl;
    }
  catch(const CORBA::TRANSIENT& x)
    {
      std::cerr << "caught: CORBA::TRANSIENT" << std::endl;
    }
  catch(const CORBA::Exception& x)
    {
      std::cerr << "caught: CORBA::Exception " << x._name() << std::endl;
    }
  catch(...)
    {
      std::cerr << "caught: unknown" << std::endl;
    }
}
