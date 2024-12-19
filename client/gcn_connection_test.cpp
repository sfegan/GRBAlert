#include<iostream>
#include<string>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<cstdlib>
#include<memory.h>
#include<netdb.h>

#include<VSOptions.hpp>
#include<VATime.h>

#include<gcn_defs.h>

using namespace VERITAS;

int hete_test[40] = { 44, 14, 0, 7212400, 262158, 14141, 7212400, 1080000, -350000, -1, 1000, 1000, 101, 1234, 4234, 16842754, 1080500, -349499, 1079500, -349499, 1079500, -350500, 1080500, -350500, 65538, 196612, 1080100, -349899, 1079900, -349899, 1079900, -350100, 1080100, -350100, 327686, 458760, 268435455, -65, 287454020, 10 };

int bat_real[40] = { 61, 2, 0, 7208400, 260304, 14143, 7173423, 2999507, 349139, 0, 2101, 500, -654, 5564, 80000, 0, 71395038, 20000, 1041, 0, 825, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100663552, 16842497, 455, 10 };

int xrt_real[40] = { 67, 1, 0, 7217900, 260304, 14143, 7216972, 2999345, 348987, 695862, 0, 13, 32766, 23716, 26120, 24360, 0, 646, 1, 8388608, 0, 1307, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10 };

in_addr_t lookup_ip(const std::string& ip_or_name)
{
  if(ip_or_name.empty())return INADDR_NONE;

  struct in_addr ip;
  ip.s_addr = inet_addr(ip_or_name.c_str());
  if(ip.s_addr == INADDR_NONE)
    {
      struct hostent *h_ent = gethostbyname(ip_or_name.c_str());
      if(h_ent == NULL)return INADDR_NONE;
      bcopy(h_ent->h_addr, &ip.s_addr, h_ent->h_length);
    }

  return ip.s_addr;
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

  std::string               host                 = "127.0.0.1";
  int                       port                 = 5226;

  std::string               trigger_type         = "bat";
  std::string               trigger_time         = "now";
  int                       trigger_number       = 0;
  float                     trigger_ra           = 0;
  float                     trigger_dec          = 0;
  int                       trigger_flags        = 0;

  // Fill variables from command line parameters ------------------------------

  if(options.find("h","Print this message.")!=VSOptions::FS_NOT_FOUND)
    print_usage=true;
  if(options.find("help","Print this message.")!=VSOptions::FS_NOT_FOUND)
    print_usage=true;

  options.findWithValue("host",host,
			"Set the name of the host to connect to.");
  options.findWithValue("port",port,
			"Set the port number to connect to.");

  options.findWithValue("type",trigger_type,
			"Set the type of GRB to send. Valid options are "
			"\"hete_test\", \"bat\", \"xrt\".");
  options.findWithValue("time",trigger_time,
			"Set the trigger time. Should be the string \"now\" "
			"or a valid UTC time.");
  options.findWithValue("number",trigger_number,
			"Set the trigger number");
  options.findWithValue("ra",trigger_ra,
			"Set the RA of the trigger");
  options.findWithValue("dec",trigger_dec,
			"Set the DEC of the trigger");
  options.findWithValue("flags",trigger_flags,
			"Set the trigger flags");

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
  // Compose the message
  // --------------------------------------------------------------------------

  int* data = bat_real;
  if(trigger_type == "hete_test")
    data=hete_test;
  else if(trigger_type == "bat")
    data=bat_real;
  else if(trigger_type == "xrt")
    data=xrt_real;

  VATime t = VATime::now();
  if(trigger_time != "now")t.setFromString(trigger_time);

  data[BURST_TJD]  = t.getMJDInt()-40000;
  data[BURST_SOD]  = t.getDayNS()/10000000;
  data[BURST_TRIG] = trigger_number;
  data[BURST_RA]   = int(trigger_ra*10000);
  data[BURST_DEC]  = int(trigger_dec*10000);
  data[TRIGGER_ID] = trigger_flags;

  // --------------------------------------------------------------------------
  // Set up the connection
  // --------------------------------------------------------------------------

  struct sockaddr_in sin;
  bzero(&sin, sizeof(sin));
  sin.sin_family       = AF_INET;
  sin.sin_addr.s_addr  = lookup_ip(host);
  sin.sin_port         = htons(port);

  int client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(client_fd < 0)
    {
      std::cerr << "could not open socket: " << strerror(errno) << std::endl;
      exit(EXIT_FAILURE);
    }

  if(connect(client_fd,reinterpret_cast<const struct sockaddr*>(&sin),
	     sizeof(sin))<0)
    {
      std::cerr << "could not connect: " << strerror(errno) << std::endl;
      exit(EXIT_FAILURE);
    }


  int test[40];
  for(unsigned i=0;i<40;i++)test[i] = htonl(data[i]);

  write(client_fd,test,sizeof(test));
  shutdown(client_fd,SHUT_WR);
  close(client_fd);
}
