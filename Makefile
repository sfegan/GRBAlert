# -----------------------------------------------------------------------------
#
# GRB monitor makefile
#
# Original Author: Stephen Fegan
# $Author: sfegan $
# $Date: 2007/08/14 22:32:02 $
# $Revision: 1.10 $
# $Tag$
#
# -----------------------------------------------------------------------------

include Makefile.common

TARGETS = grb_monitor list_grb gcn_connection_test grb_alert

all: $(TARGETS)

LIBDEPS = 

LIBS = 

CXXFLAGS += -Iserver -Iclient -Icommon -Iutility -I.
LDFLAGS += 

VPATH = server:client:common:utility:idl

# ALL IDL FILES
IDLFILE = NET_VGRBMonitor.idl NET_VGRBMonitor_data.idl
IDLHEAD = $(IDLFILE:.idl=.h)

# All SOURCES
BINSRCS = list_grb.cpp gcn_connection_test.cpp grb_monitor.cpp grb_alert.cpp
SRVSRCS = GCNAcquisitionLoop.cpp GRBTriggerRepository.cpp                     \
          GRBMonitorServant.cpp                                               \
	  Angle.cpp Logger.cpp Daemon.cpp VSDataConverter.cpp VSOptions.cpp   \
	  VATime.cpp Debug.cpp Exception.cpp VOmniORBHelper.cpp
CLISRCS = VSDataConverter.cpp VSOptions.cpp PhaseLockedLoop.cpp Logger.cpp    \
	  VATime.cpp Debug.cpp Exception.cpp VOmniORBHelper.cpp Daemon.cpp
IDLSRCS = $(IDLHEAD:.h=.cpp) $(IDLHEAD:.h=_dyn.cpp)

# All OBJECTS
BINOBJS = $(BINSRCS:.cpp=.o)
SRVOBJS = $(SRVSRCS:.cpp=.o)
CLIOBJS = $(CLISRCS:.cpp=.o)
IDLOBJS = $(IDLSRCS:.cpp=.o)

clean: 
	$(RM) $(BINOBJS) $(SRVOBJS) $(CLIOBJS) $(IDLOBJS) $(TARGETS)          \
              *~ *.bak test                                                   \
	      $(IDLHEAD) $(IDLSRCS) $(IDLOBJS)

distclean: clean

$(BINOBJS): $(IDLHEAD)
$(SRVOBJS): $(IDLHEAD)

grb_monitor: grb_monitor.o $(SRVOBJS) $(IDLOBJS) $(LIBDEPS)
	$(CXX) -o $@ $^ $(LDFLAGS) 

list_grb: list_grb.o $(CLIOBJS) $(IDLOBJS) $(LIBDEPS)
	$(CXX) -o $@ $^ $(LDFLAGS) 

gcn_connection_test: gcn_connection_test.o $(CLIOBJS) $(LIBDEPS)
	$(CXX) -o $@ $^ $(LDFLAGS) 

grb_alert: grb_alert.o $(CLIOBJS) $(IDLOBJS) $(LIBDEPS) 
	$(CXX) -o $@ $^ $(LDFLAGS) 

$IDLFILE:.idl=.h: %.h: %.idl

$IDLFILE:.idl=.cpp: %.cpp: %.idl

$IDLFILE:.idl=_dyn.cpp: %_dyn.cpp: %.idl

$(IDLOBJS): %.o: %.cpp

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.h: %.idl
	$(IDL) $(IDLFLAGS) $<

%.cpp: %.idl
	$(IDL) $(IDLFLAGS) $<
