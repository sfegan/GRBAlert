//-*-mode:c++; mode:font-lock;-*-
/**
 * \file GCNAcquisitionLoop.cpp
 * \ingroup GRB
 * \brief Communication with GCN, runs as separate thread.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2009/06/01 19:12:29 $
 * $Revision: 1.24 $
 * $Tag$
 *
 **/

#include<stdint.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fstream>

#include<zthread/Thread.h>

#include<VATime.h>
#include<VSDataConverter.hpp>

#include<gcn_defs.h>
#include<GRBTriggerManip.hpp>
#include<GCNAcquisitionLoop.hpp>

using namespace VERITAS;

#define N(x) (sizeof(x)/sizeof(*x))

#define CENT2DEG(x) (float(x)/100.0)
#define DEG(x) (float(x)/10000.0)
#define MJD(x) (x+40000)
#define MS(x) (x*10)

#define OBS_WINDOW_HRS 3.0

GCNAcquisitionLoop::
GCNAcquisitionLoop(GRBTriggerRepository* grbs, Logger* logger,
		   unsigned short port, unsigned conn_timeout_sec,
		   bool pass_test_messages):
  m_grbs(grbs), m_logger(logger), m_port(port),
  m_conn_timeout_sec(conn_timeout_sec), m_test_messages(pass_test_messages)
{
  /* nothing to see here */ 
}

GCNAcquisitionLoop::~GCNAcquisitionLoop()
{
  // notthing to see here
}

void GCNAcquisitionLoop::run()
{
  //signal(SIGPIPE, SIG_IGN);

  // Set the connection state to DOWN
  m_grbs->setConnectionUp(false);

  int one = 1;
  struct sockaddr_in sin;
  bzero(&sin, sizeof(sin));
  sin.sin_family       = AF_INET;
  sin.sin_addr.s_addr  = htonl(INADDR_ANY);
  sin.sin_port         = htons(m_port);

  // Open the socket
  int serv_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(serv_fd < 0)
    {
      std::ostringstream stream;
      stream << "could not open socket: " << strerror(errno);
      m_logger->logMessage(1,stream.str());
      exit(EXIT_FAILURE);
    }

  // Set the REUSE ADDRESS flag
  if(setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))<0)
    {
      std::ostringstream stream;
      stream << "could not set socket options" << strerror(errno);
      m_logger->logMessage(1,stream.str());
      exit(EXIT_FAILURE);
    }

  // Bind the name to the socket.
  if(bind(serv_fd, reinterpret_cast<const struct sockaddr*>(&sin), 
	  sizeof(sin)))
    {
      std::ostringstream stream;
      stream << "could not bind to port " << m_port << ": " << strerror(errno);
      m_logger->logMessage(1,stream.str());
      exit(EXIT_FAILURE);
    }

  // Listen for the socket connection from the GCN machine.
  if(listen(serv_fd, 1))
    {
      std::ostringstream stream;
      stream << "could not listen: " << strerror(errno);
      m_logger->logMessage(1,stream.str());
      exit(EXIT_FAILURE);
    }

  int conn_fd = -1;
  while(1)
    {
      if(conn_fd<0)
	{
	  socklen_t sizeof_sin = sizeof(sin);
	  conn_fd = 
	    accept(serv_fd, reinterpret_cast<struct sockaddr*>(&sin), 
		   &sizeof_sin);
	  
	  if(conn_fd<0)
	    {
	      if(errno != EINTR)
		{
		  std::ostringstream stream;
		  stream << "connection could not be accepted: " 
			 << strerror(errno);
		  m_logger->logMessage(2,stream.str());
		}
	    }
	  else
	    {
	      // If we need to filter addresses use these to shutdown
	      // bad connections
              //shutdown(client_fd,2);
              //close(client_fd);

	      std::ostringstream stream;
	      stream << "connection accepted from: " 
		     << inet_ntoa(sin.sin_addr);
	      m_logger->logMessage(2,stream.str());
	      m_grbs->setConnectionUp(true);
	    }
	}
      else
	{
	  fd_set ibits;
	  fd_set obits; // not used
	  fd_set ebits;

	  FD_ZERO (&ibits);
	  FD_ZERO (&obits); // not used
	  FD_ZERO (&ebits);

	  FD_SET(conn_fd, &ibits);
	  FD_SET(conn_fd, &ebits);

	  struct timeval timeout;
          timeout.tv_sec  = m_conn_timeout_sec;
          timeout.tv_usec = 0;

	  // Wait for action on the PORT
	  int nready = select(conn_fd+1,&ibits,0,&ebits,&timeout);

	  if(nready<0)
	    {
	      // An error occurred

	      if(errno != EINTR)
		{
		  std::ostringstream stream;
		  stream << "error occurred while waiting for data from GCN: " 
			 << strerror(errno);
		  m_logger->logMessage(2,stream.str());

		  shutdown(conn_fd,SHUT_RDWR);
		  close(conn_fd);
		  conn_fd = -1;

		  m_grbs->setConnectionUp(false);
		}

	      continue;
	    }
	  else if(nready == 0)
	    {
	      // Timeout went off

	      std::ostringstream stream;
	      stream << m_conn_timeout_sec 
		     << " second timeout while waiting for data from GCN";
	      m_logger->logMessage(2,stream.str());

	      shutdown(conn_fd,SHUT_RDWR);
	      close(conn_fd);
	      conn_fd = -1;

	      m_grbs->setConnectionUp(false);

	      continue;
	    }
	  else if(!FD_ISSET(conn_fd,&ibits))
	    {
	      // Select returned without timeout but file descriptor is not
	      // ready for reading. Probably indicates connection was closed.

	      std::ostringstream stream;
	      stream << "connection closed by GCN";
	      m_logger->logMessage(2,stream.str());
	      
	      shutdown(conn_fd,SHUT_RDWR);
	      close(conn_fd);
	      conn_fd = -1;

	      m_grbs->setConnectionUp(false);

	      continue;
	    }

	  // Set up space to hold the GCN packet
	  int32_t buffer[SIZ_PKT];
	  bzero(buffer,sizeof(buffer));

	  // Try to read the data from GCN
	  int bytes = read(conn_fd, buffer,sizeof(buffer));
	  if(bytes<0)
	    {
	      if(errno != EINTR)
		{
		  std::ostringstream stream;
		  stream << "error reading data from GCN: " 
			 << strerror(errno);
		  m_logger->logMessage(2,stream.str());

		  shutdown(conn_fd,SHUT_RDWR);
		  close(conn_fd);
		  conn_fd = -1;

		  m_grbs->setConnectionUp(false);
		}

	      continue;
	    }
	  else if(bytes==0)
	    {
	      std::ostringstream stream;
	      stream << "connection closed by GCN";
	      m_logger->logMessage(2,stream.str());
	      
	      shutdown(conn_fd,SHUT_RDWR);
	      close(conn_fd);
	      conn_fd = -1;

	      m_grbs->setConnectionUp(false);	      

	      continue;
	    }
	  else if(bytes != sizeof(buffer))
	    {
	      std::ostringstream stream;
	      stream << "packet has only " << bytes << " bytes (expected "
		     << sizeof(buffer) << ')';
	      m_logger->logMessage(2,stream.str());

	      continue;
	    }
	
	  VATime receipt_time = VATime::now();
	  m_grbs->setLastGCNReceiptTime(receipt_time);

	  int32_t packet_type = ntohl(buffer[PKT_TYPE]);

	  if(packet_type == TYPE_KILL_SOCKET)
	    {
	      std::ostringstream stream;
	      stream << "GCN kill packet received... closing connection";
	      m_logger->logMessage(2,stream.str());
 
	      shutdown(conn_fd,SHUT_RDWR);
	      close(conn_fd);
	      conn_fd = -1;

	      m_grbs->setConnectionUp(false);
	      continue;
	    }

	  // Write data back to GCN so they can measure the round-trip time
	  write(conn_fd,buffer,sizeof(buffer));

	  // Swap bytes
	  for(unsigned iword=0; iword<N(buffer); iword++)
	    buffer[iword] = ntohl(buffer[iword]);
	  
	  // Function to handle GCN packets
	  GRBTrigger* T = handle_gcn_packets(buffer);

	  if(T)
	    {
	      T->veritas_unique_sequence_number   = 0;
	      T->veritas_receipt_mjd_int          = receipt_time.getMJDInt();
	      T->veritas_receipt_msec_of_day_int  = 
		CORBA::ULong(receipt_time.getDayNS()/UINT64_C(1000000));

	      m_grbs->putTrigger(T, m_logger);

	      GRBTriggerManip::free(T);
	    }
	}
    }
}

#define UNHANDLEDCASE(x) \
case x: \
  logData(m_logger,5,buffer); \
  m_logger->logMessage(4, std::string("packet type: " #x "(") \
                          + VSDataConverter::toString(buffer[PKT_TYPE]) \
                          + std::string(") (unsupported)")); \
  break

#define HANDLEDCASE(x,t) \
case x: \
  logData(m_logger,5,buffer); \
  m_logger->logMessage(4, std::string("packet type: " #x " (") \
                          + VSDataConverter::toString(buffer[PKT_TYPE]) \
                          + std::string(")")); \
  T = t(buffer); \
  break;

#define TESTCASE(x,t) \
case x: \
  logData(m_logger,m_test_messages?5:7,buffer);	\
  m_logger->logMessage(m_test_messages?4:6, \
                       std::string("packet type: " #x " (") \
                       + VSDataConverter::toString(buffer[PKT_TYPE]) \
                       + std::string(")")); \
  if(m_test_messages)T = t(buffer); \
  break;

void logData(Logger* logger, unsigned priority, const int32_t* buffer)
{
  std::ostringstream stream;
  stream << "packet data:";
  for(unsigned iword=0;iword<SIZ_PKT;iword++)stream << ' ' << buffer[iword];
  logger->logMessage(priority,stream.str());
}

GRBTrigger* GCNAcquisitionLoop::handle_gcn_packets(const int32_t* buffer)
{
  GRBTrigger* T = 0;
  std::ifstream infile;
  std::string line;

  switch(buffer[PKT_TYPE])
    {
    // IM ALIVE ---------------------------------------------------------------

    case TYPE_IM_ALIVE:
      logData(m_logger,6,buffer);
      m_logger->logMessage(6,std::string("packet type: TYPE_IM_ALIVE (")
			   + VSDataConverter::toString(buffer[PKT_TYPE])
			   + std::string(")"));
      //Put a user test routine here
      infile.open("/usr/local/veritas/share/grb_user_test.txt");
      if(infile){
	m_logger->logMessage(4,std::string("packet type: USER_TEST"));
	getline(infile,line,'\n');
	infile.close();
	unlink("/usr/local/veritas/share/grb_user_test.txt");
	T = T_usertest(buffer);
      }
      break;

    // TEST -------------------------------------------------------------------

    TESTCASE(TYPE_TEST_COORDS,T_test);

    // ALEXIS -----------------------------------------------------------------

    UNHANDLEDCASE(TYPE_ALEXIS_SRC);

    // XTE --------------------------------------------------------------------

    UNHANDLEDCASE(TYPE_XTE_PCA_ALERT);
    UNHANDLEDCASE(TYPE_XTE_PCA_SRC);
    UNHANDLEDCASE(TYPE_XTE_ASM_SRC);
    UNHANDLEDCASE(TYPE_XTE_ASM_TRANS);

    // IPN --------------------------------------------------------------------

    UNHANDLEDCASE(TYPE_IPN_SEG_SRC);
    UNHANDLEDCASE(TYPE_IPN_POS_SRC);

    // HETE -------------------------------------------------------------------

    UNHANDLEDCASE(TYPE_HETE_ALERT_SRC);
    HANDLEDCASE(TYPE_HETE_UPDATE_SRC,T_hete);
    HANDLEDCASE(TYPE_HETE_FINAL_SRC,T_hete);
    HANDLEDCASE(TYPE_HETE_GNDANA_SRC,T_hete);
    TESTCASE(TYPE_HETE_TEST,T_hete);

    // COUNTER PART -----------------------------------------------------------

    UNHANDLEDCASE(TYPE_GRB_CNTRPART_SRC);

    // INTEGRAL ---------------------------------------------------------------

    UNHANDLEDCASE(TYPE_INTEGRAL_POINTDIR_SRC);
    UNHANDLEDCASE(TYPE_INTEGRAL_SPIACS_SRC);

    HANDLEDCASE(TYPE_INTEGRAL_WAKEUP_SRC,T_integral);
    HANDLEDCASE(TYPE_INTEGRAL_REFINED_SRC,T_integral);
    HANDLEDCASE(TYPE_INTEGRAL_OFFLINE_SRC,T_integral);

    // MILAGRO ----------------------------------------------------------------

    UNHANDLEDCASE(TYPE_MILAGRO_POS_SRC);

    // SWIFT ------------------------------------------------------------------

    UNHANDLEDCASE(TYPE_SWIFT_BAT_GRB_ALERT_SRC); // before coords determined

    HANDLEDCASE(TYPE_SWIFT_BAT_GRB_POS_ACK_SRC, T_swift);
    TESTCASE(TYPE_SWIFT_BAT_GRB_POS_TEST, T_swift);
    UNHANDLEDCASE(TYPE_SWIFT_BAT_TRANS); //, T_swift);

    UNHANDLEDCASE(TYPE_SWIFT_BAT_GRB_POS_NACK_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_BAT_GRB_LC_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_SCALEDMAP_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_FOM_2OBSAT_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_FOSC_2OBSAT_SRC);

    HANDLEDCASE(TYPE_SWIFT_XRT_POSITION_SRC, T_swift);

    UNHANDLEDCASE(TYPE_SWIFT_XRT_SPECTRUM_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_XRT_IMAGE_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_XRT_LC_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_XRT_CENTROID_SRC);

    UNHANDLEDCASE(TYPE_SWIFT_UVOT_IMAGE_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_UVOT_SLIST_SRC);

    UNHANDLEDCASE(TYPE_SWIFT_UVOT_POS_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_POINTDIR_SRC);

    UNHANDLEDCASE(TYPE_SWIFT_UVOT_POS_NACK_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_BAT_SUB_THRESHOLD_SRC);
    UNHANDLEDCASE(TYPE_SWIFT_BAT_SLEW_POS_SRC);

    // AGILE ------------------------------------------------------------------ 
    HANDLEDCASE(TYPE_AGILE_GRB_WAKEUP, T_agile);
    HANDLEDCASE(TYPE_AGILE_GRB_PROMPT, T_agile);
    HANDLEDCASE(TYPE_AGILE_GRB_REFINED, T_agile);
    UNHANDLEDCASE(TYPE_AGILE_POINTDIR);
    UNHANDLEDCASE(TYPE_AGILE_TRANS);
    UNHANDLEDCASE(TYPE_AGILE_GRB_POS_TEST);

    // FERMI ------------------------------------------------------------------
    UNHANDLEDCASE(TYPE_FERMI_GBM_ALERT);
    HANDLEDCASE(TYPE_FERMI_GBM_FLT_POS, T_fermi);
    HANDLEDCASE(TYPE_FERMI_GBM_GND_POS, T_fermi);
    UNHANDLEDCASE(TYPE_FERMI_GBM_LC);
    UNHANDLEDCASE(TYPE_FERMI_GBM_TRANS);
    UNHANDLEDCASE(TYPE_FERMI_GBM_POS_TEST);
    HANDLEDCASE(TYPE_FERMI_LAT_POS_INI, T_fermi);
    HANDLEDCASE(TYPE_FERMI_LAT_POS_UPD, T_fermi);
    UNHANDLEDCASE(TYPE_FERMI_LAT_POS_DIAG);
    UNHANDLEDCASE(TYPE_FERMI_LAT_TRANS);
    UNHANDLEDCASE(TYPE_FERMI_LAT_POS_TEST);
    UNHANDLEDCASE(TYPE_FERMI_OBS_REQ);
    UNHANDLEDCASE(TYPE_FERMI_SC_SLEW);
    UNHANDLEDCASE(TYPE_FERMI_LAT_GND_REF);
    UNHANDLEDCASE(TYPE_FERMI_LAT_GND_TRIG);
    UNHANDLEDCASE(TYPE_FERMI_POINTDIR);

      
    default:
      logData(m_logger,5,buffer); 
      m_logger->logMessage(4,std::string("packet type: unknown (")
			   + VSDataConverter::toString(buffer[PKT_TYPE])
			   + std::string(")"));
      break;
    };

  if(T)
    {

      std::ostringstream Tstream;
      
      //Cut on the declination: don't observe if source is south of -38 lat.
      if((T->veritas_should_observe)&&(T->coord_dec_deg < -38)){
	T->veritas_should_observe = false;
	Tstream.str("");
	Tstream << "GRB " << T->trigger_gcn_sequence_number 
		<< " gets the axe because it is too far south (< -38 deg).";
	m_logger->logMessage(3,Tstream.str());
      }

      //Cut on the error circle being too large. Not sure if this will affect 
      //any notices aside from Fermi GBM
      //Only observe if the 1-sigma radius of the error circle is smaller
      //than 10 degrees.
      if((T->veritas_should_observe)&&(T->coord_error_circle_deg > 10)){
	T->veritas_should_observe = false;
	Tstream.str("");
	Tstream << "GRB " << T->trigger_gcn_sequence_number
		<< "'s error circle is too large (> 10 deg radius)."
		<< " It will not be observed.";
	m_logger->logMessage(3,Tstream.str());
      }

      Tstream.str("");

      Tstream << "packet info: " 
	      << T->trigger_gcn_sequence_number << ' '
	      << T->trigger_instrument << ' '
	      << T->trigger_type << ' '
	      << T->trigger_time_mjd_int << ' '
	      << T->trigger_msec_of_day_int << ' '
	      << T->coord_ra_deg << ' '
	      << T->coord_dec_deg << ' '
	      << T->coord_epoch_J << ' '
	      << T->coord_error_circle_deg << ' '
	      << T->veritas_should_observe << ' '
	      << T->veritas_observation_window_hours;
      m_logger->logMessage(3,Tstream.str());
    }

  return T;
}

GRBTrigger* GCNAcquisitionLoop::T_usertest(const int32_t* buffer)
{

  int id = 0;

  GRBTrigger* T = GRBTriggerManip::allocZero();
  
  T->trigger_gcn_sequence_number      = id>0?unsigned(id):0;
  T->trigger_instrument               = "USER";
  T->trigger_type                     = "TEST";
  T->trigger_time_mjd_int             = MJD(buffer[BURST_TJD]);
  T->trigger_msec_of_day_int          = MS(buffer[BURST_SOD]);
  T->coord_ra_deg                     = 037.9545154;
  T->coord_dec_deg                    = 89.2641094;
  T->coord_epoch_J                    = 2000.0;
  T->coord_error_circle_deg           = 0.01;
  T->veritas_should_observe           = true;
  T->veritas_observation_window_hours = OBS_WINDOW_HRS;

  return T;
}

GRBTrigger* GCNAcquisitionLoop::T_test(const int32_t* buffer)
{
  int id = buffer[BURST_TRIG];

  GRBTrigger* T = GRBTriggerManip::allocZero();

  T->trigger_gcn_sequence_number      = id>0?unsigned(id):0;
  T->trigger_instrument               = "GCN";
  T->trigger_type                     = "TEST";
  T->trigger_time_mjd_int             = MJD(buffer[BURST_TJD]);
  T->trigger_msec_of_day_int          = MS(buffer[BURST_SOD]);
  T->coord_ra_deg                     = CENT2DEG(buffer[BURST_RA]);
  T->coord_dec_deg                    = CENT2DEG(buffer[BURST_DEC]);
  T->coord_epoch_J                    = 2000.0;
  T->coord_error_circle_deg           = CENT2DEG(buffer[BURST_ERROR]);
  T->veritas_should_observe           = false;
  T->veritas_observation_window_hours = OBS_WINDOW_HRS;

  return T;
}

GRBTrigger* GCNAcquisitionLoop::T_hete(const int32_t* buffer)
{
  bool observe = false;
  std::string type;

  switch(buffer[PKT_TYPE])
    {
    case TYPE_HETE_UPDATE_SRC:
      type      = "INITIAL";
      observe   = true;
      break;

    case TYPE_HETE_FINAL_SRC:
      type      = "REFINED";
      observe   = true;
      break;

    case TYPE_HETE_GNDANA_SRC:
      type      = "GROUND";
      observe   = true;
      break;

    case TYPE_HETE_TEST:
      type      = "TEST";
      observe   = false;
      break;

    default:
      return 0;
    }

  if(observe)
    {
      // check the intensity ?
      // check the flags ?
    }

  int id     = (buffer[BURST_TRIG] & H_TRIGNUM_MASK) >>H_TRIGNUM_SHIFT;
  //int seqnum = (buffer[BURST_TRIG] & H_SEQNUM_MASK)  >>H_SEQNUM_SHIFT;
  
  GRBTrigger* T = GRBTriggerManip::allocZero();

  VATime t(MJD(buffer[BURST_TJD]),MS(buffer[BURST_SOD])*UINT64_C(1000000));

  T->trigger_gcn_sequence_number      = id>0?unsigned(id):0;
  T->trigger_instrument               = "HETE";
  T->trigger_type                     = type.c_str();
  T->trigger_time_mjd_int             = MJD(buffer[BURST_TJD]);
  T->trigger_msec_of_day_int          = MS(buffer[BURST_SOD]);
  T->coord_ra_deg                     = DEG(buffer[BURST_RA]);
  T->coord_dec_deg                    = DEG(buffer[BURST_DEC]);
  // NOTE: HETE coordinates are in current epoch, not J2000
  T->coord_epoch_J                    = t.getJulianEpoch();
  T->coord_error_circle_deg           = DEG(buffer[BURST_ERROR]);
  T->veritas_should_observe           = observe;
  T->veritas_observation_window_hours = OBS_WINDOW_HRS;

  return T;
}

GRBTrigger* GCNAcquisitionLoop::T_integral(const int32_t* buffer)
{
  bool observe = false;
  std::string type;

  switch(buffer[PKT_TYPE])
    {
    case TYPE_INTEGRAL_WAKEUP_SRC:
      type      = "INITIAL";
      observe   = true;
      break;

    case TYPE_INTEGRAL_REFINED_SRC:
      type      = "REFINED";
      observe   = true;
      break;

    case TYPE_INTEGRAL_OFFLINE_SRC:
      type      = "GROUND";
      observe   = true;
      break;

    default:
      return 0;
    }

  if((buffer[12]&0x80000000)||(buffer[12]&0x40000000))
    {
      if(!m_test_messages)return 0;
      type      = std::string("TEST/")+type;
      observe   = false;
    }

  int id =     (buffer[BURST_TRIG] & I_TRIGNUM_MASK) >> I_TRIGNUM_SHIFT;
  //int seqnum = (buffer[BURST_TRIG] & I_SEQNUM_MASK)  >> I_SEQNUM_SHIFT;

  GRBTrigger* T = GRBTriggerManip::allocZero();

  T->trigger_gcn_sequence_number      = id>0?unsigned(id):0;
  T->trigger_instrument               = "INTEGRAL";
  T->trigger_type                     = type.c_str();
  T->trigger_time_mjd_int             = MJD(buffer[BURST_TJD]);
  T->trigger_msec_of_day_int          = MS(buffer[BURST_SOD]);
  T->coord_ra_deg                     = DEG(buffer[BURST_RA]);
  T->coord_dec_deg                    = DEG(buffer[BURST_DEC]);
  T->coord_epoch_J                    = 2000.0;
  T->coord_error_circle_deg           = DEG(buffer[BURST_ERROR]);
  T->veritas_should_observe           = observe;
  T->veritas_observation_window_hours = OBS_WINDOW_HRS;

  return T;
}

GRBTrigger* GCNAcquisitionLoop::T_swift(const int32_t* buffer)
{
  std::string detector;
  std::string type;
  bool observe = false;

  switch(buffer[PKT_TYPE])
    {
    case TYPE_SWIFT_BAT_GRB_POS_ACK_SRC:
      detector  = "SWIFT/BAT";
      type      = "TRIGGER";
      observe   = true;
      break;

    case TYPE_SWIFT_BAT_GRB_POS_TEST:
      detector  = "SWIFT/BAT";
      type      = "TEST";
      observe   = false;
      break;

    case TYPE_SWIFT_XRT_POSITION_SRC:
      detector  = "SWIFT/XRT";
      type      = "TRIGGER";
      observe   = true;
      break;

    default:
      return 0;
      break;
    }

  int id     = (buffer[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
  //int segnum = (buffer[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;

  if(observe)
    {
      // Note: This soln_status is valid only for BAT, not XRT
      // BitNum  F-v-G  Item_name      Description
      // ------  ----   ---------      -----------
      // 0        F     point_src      0 = No or 1=Yes, a point source was found
      // 1        F     grb            0 = No or 1=Yes, it is a GRB
      // 2        F     interesting    0 = No or 1=Yes, it is an interesting src (ie a flaring known src)
      // 3        F     flt_cat_src    0 = No or 1=Yes, it is in the flight catalog (else unknown src)
      // 4        F     image_trig     0 = No or 1=Yes, it is an image trigger (else rate trig)
      // 5        G     def_not_grb    0 = No or 1=Yes, it is definitely not a GRB (a Retraction)
      // 6        G     uncert_grb     0 = No or 1=Yes, it is probably not a GRBorTransient (hi bkg level, ie near SAA)
      // 7        G     uncert_grb     0 = No or 1=Yes, it is probably not a GRBorTransient (low image significance)
      // 8        G     gnd_cat_src    0 = No or 1=Yes, it is in the ground catalog
      // 9        G     uncert_grb     0 = No or 1=Yes, it is probably not a GRBorTransient (negative bkg slope, exit SAA)
      // 10       G     st_loss_lock   0 = No or 1=Yes, StarTracker not locked so trigger probably bogus
      // 11       G     not_anything   0 = No or 1=Yes, it is not a GRBorTransient (VERY low image significance)
      // 12       G     blk_cat_src    0 = No or 1=Yes, it is in the catalog of sources to be blocked (internal use only)
      // 13-29    -     spare          spare for future use
      // 30       G     test_submit    0 = No or 1=Yes, this is a test submission (internal use only)
      // 31       -     spare          spare for future use

      if(buffer[TRIGGER_ID] & (1<<5)) // This bit flag is the same for both BAT and XRT
	{	  
	  // what to do in case of a *retraction*

//	  type    = std::string("RETRACTION/")+type;
	  type    = "RETRACTION";
	  observe = false;
	}
      else if((detector == "SWIFT/BAT")
	      &&((!(buffer[TRIGGER_ID] & (1<<1)))
		 ||(buffer[TRIGGER_ID] & (1<<2))
		 ||(buffer[TRIGGER_ID] & (1<<8))
		 ||(id <= 99999)))
	{	  
	  // what to do in case flight software does not flag it as a GRB
	  // what to do if it is a known source or TOO
	  // We're only interested in GRBs here
	  observe = false;
	}
      else if((detector == "SWIFT/BAT")
	      &&((buffer[TRIGGER_ID] & (1<<6))
		 ||(buffer[TRIGGER_ID] & (1<<7))
		 ||(buffer[TRIGGER_ID] & (1<<9))
		 ||(buffer[TRIGGER_ID] & (1<<10))
		 ||(buffer[TRIGGER_ID] & (1<<11))))
	{
	  // what to do in the case of a *probable* non-GRB

	}


      // check the intensity ?
      // check the error ?
    }


  GRBTrigger* T = GRBTriggerManip::allocZero();
  
  T->trigger_gcn_sequence_number      = id>0?unsigned(id):0;
  T->trigger_instrument               = detector.c_str();
  T->trigger_type                     = type.c_str();
  T->trigger_time_mjd_int             = MJD(buffer[BURST_TJD]);
  T->trigger_msec_of_day_int          = MS(buffer[BURST_SOD]);
  T->coord_ra_deg                     = DEG(buffer[BURST_RA]);
  T->coord_dec_deg                    = DEG(buffer[BURST_DEC]);
  T->coord_epoch_J                    = 2000.0;
  T->coord_error_circle_deg           = DEG(buffer[BURST_ERROR]);
  T->veritas_should_observe           = observe;
  T->veritas_observation_window_hours = OBS_WINDOW_HRS;
  
  return T;
}

GRBTrigger* GCNAcquisitionLoop::T_agile(const int32_t* buffer)
{

    std::string type;
    bool observe = false;

    switch(buffer[PKT_TYPE])
	{
	case TYPE_AGILE_GRB_WAKEUP:
	    type      = "WAKEUP";
	    observe   = true;
	    break;

	case TYPE_AGILE_GRB_PROMPT:
	    type      = "PROMPT";
	    observe   = true;
	    break;
	    	    
	case TYPE_AGILE_GRB_REFINED:
	    type      = "REFINED";
	    observe   = true;
	    break; 

	default:
	    return 0;
	    break;
	}


    int id =     buffer[BURST_TRIG];

    GRBTrigger* T = GRBTriggerManip::allocZero();
    
    T->trigger_gcn_sequence_number      = id>0?unsigned(id):0;
    T->trigger_instrument               = "AGILE";
    T->trigger_type                     = type.c_str();
    T->trigger_time_mjd_int             = MJD(buffer[BURST_TJD]);
    T->trigger_msec_of_day_int          = MS(buffer[BURST_SOD]);
    T->coord_ra_deg                     = DEG(buffer[BURST_RA]);
    T->coord_dec_deg                    = DEG(buffer[BURST_DEC]);
    T->coord_epoch_J                    = 2000.0;
    T->coord_error_circle_deg           = DEG(buffer[BURST_ERROR]);
    T->veritas_should_observe           = observe;
    T->veritas_observation_window_hours = OBS_WINDOW_HRS;

    return T;
}



GRBTrigger* GCNAcquisitionLoop::T_fermi(const int32_t* buffer)
{
    
    std::string detector;
    std::string type;
    std::ostringstream Tstream;
    bool observe = false;
    
    switch(buffer[PKT_TYPE])
	{
	case TYPE_FERMI_GBM_FLT_POS:
	    detector  = "FERMI/GBM";
	    type      = "FLT_POS";
	    observe   = true;
	    break;
	    
	case TYPE_FERMI_GBM_GND_POS:
	    detector  = "FERMI/GBM";
	    type      = "GND_POS";
	    observe   = true;
	    break;

	case TYPE_FERMI_LAT_POS_INI:
	    detector  = "FERMI/LAT";
	    type      = "INITIAL";
	    observe   = true;
	    break;

	case TYPE_FERMI_LAT_POS_UPD:
	    detector  = "FERMI/LAT";
	    type      = "UPDATED";
	    observe   = true;
	    break;
	    	    
	default:
	    return 0;
	    break;
	}
  
    int id =     buffer[BURST_TRIG];

    GRBTrigger* T = GRBTriggerManip::allocZero();
    
    T->trigger_gcn_sequence_number      = id>0?unsigned(id):0;
    T->trigger_instrument               = detector.c_str();
    T->trigger_type                     = type.c_str();
    T->trigger_time_mjd_int             = MJD(buffer[BURST_TJD]);
    T->trigger_msec_of_day_int          = MS(buffer[BURST_SOD]);
    T->coord_ra_deg                     = DEG(buffer[BURST_RA]);
    T->coord_dec_deg                    = DEG(buffer[BURST_DEC]);
    T->coord_epoch_J                    = 2000.0;
    T->coord_error_circle_deg           = DEG(buffer[BURST_ERROR]);
    T->most_likely_source_obj           = buffer[MOST_LIKELY] & 0xffff;
    T->source_obj_confidence_level      = (buffer[MOST_LIKELY] >> 16) & 0xffff;
    T->veritas_should_observe           = observe;
    T->veritas_observation_window_hours = OBS_WINDOW_HRS;
    
    //Only observe if the GBM Flt Software thinks its more than 50%
    //likely to be a GRB.
    if( ( strcmp(T->trigger_instrument,"FERMI/GBM") == 0 ) && ( strcmp(T->trigger_type,"FLT_POS") == 0 ) ){
      if( (T->most_likely_source_obj != 4) || (T->source_obj_confidence_level < 50) ){
	T->veritas_should_observe = false;
	Tstream.str("");
	Tstream << "GRB " << T->trigger_gcn_sequence_number 
		<< " does not pass muster. GBM Flight software is"
		<< " not more than 50% confident this is actually a GRB.";
	m_logger->logMessage(3,Tstream.str());
      }
    }
        
    return T;
}
