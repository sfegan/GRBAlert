/**
 * \file GCNSocket.h
 * \ingroup GRB
 * \brief Socket header for GCN.  Based on socket_demo.c.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Jeremy Perkins
 * $Author: jperkins $
 * $Date: 2008/12/02 17:27:49 $
 * $Revision: 1.2 $
 * $Tag$
 *
 **/

#ifndef GCNSocket_H
#define GCNSocket_H

using namespace std;

/** 
 * The following definitions are straight from socket_demo.c 
 * I haven't modified them at all.
 **/

#include <stdio.h>              // Standard i/o header file
#include <sys/types.h>          // File typedefs header file
#include <sys/socket.h>         // Socket structure header file
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

char	*version = "socket_demo     Ver: 3.32   29 May 06";

#define	TRUE	1
#define	FALSE	0

#define LINUXPC

/* Indices into the socket packet array of 40 longs.  The naming and
 * definition of these 40 is based on the contents for the first packet
 * defined (the grb_coords packets; type=1).  The packing for the other packet
 * types reuses some of these 40, but they have different meaning/content.    */
#define PKT_TYPE      0		// Packet type (see below)
#define PKT_SERNUM    1		// Packet serial number (1 to 4billion)
#define PKT_HOP_CNT   2		// Packet hop counter
#define PKT_SOD       3		// Packet Sec-of-Day [centi-secs] (sssss.ss*100)
#define BURST_TRIG    4		// BATSE Trigger Number
#define BURST_TJD     5		// Truncated Julian Day
#define BURST_SOD     6		// Sec-of-Day [centi-secs] (sssss.ss*100)
#define BURST_RA      7		// RA  [centi-deg] (0.0 to 359.99 *100)
#define BURST_DEC     8		// Dec [centi-deg] (-90.0 to +90.0 *100)
#define BURST_INTEN   9		// Burst intensity/fluence [cnts]
#define BURST_PEAK   10		// Burst intensity [cnts]
#define BURST_ERROR  11		// Position uncertainty [deg] (radius)
#define SC_AZ        12		// Burst SC Az [centi-deg] (0.0 to 359.99 *100)
#define SC_EL        13		// Burst SC El [centi-deg] (-90.0 to +90.0 *100)
#define SC_X_RA      14		// SC X-axis RA [centi-deg] (0.0 to 359.99 *100)
#define SC_X_DEC     15		// SC X-axis Dec [centi-deg] (-90.0 to +90.0 *100)
#define SC_Z_RA      16		// SC Z-axis RA [centi-deg] (0.0 to 359.99 *100)
#define SC_Z_DEC     17		// SC Z-axis Dec [centi-deg] (-90.0 to +90.0 *100)
#define TRIGGER_ID   18		// Trigger type identification flags
#define MISC         19		// Misc indicators
#define E_SC_AZ      20		// Earth's center in SC Az
#define E_SC_EL      21		// Earth's center in SC El
#define SC_RADIUS    22		// Orbital radius of the GRO SC
#define BURST_T_PEAK 23		// Peak intensity time [centi-sec] (sssss.sss*100)
#define PKT_SPARE23  23		// Beginning of spare section
#define PKT_SPARE38  38		// End of spare section
#define PKT_TERM     39		// Packet termination char
/* NOTE:  Some of the fields above are used in different ways
 * for the various different packet Types.
 * You should see the socket_packet_definition_document for the details 
 * for each field within each packet type.  The various pr_*() routines
 * can also be of use in determining the packing contents & quantization.
 * The socket packet definition document is located at:
 * http://gcn.gsfc.nasa.gov/gcn/sock_pkt_def_doc.html                       */

#define SIZ_PKT      40		// Number of longwords in socket packet

// Masks for extractions for the GRO-BATSE-based TRIGGER_ID in the packet:
#define SUSP_GRB            0x00000001	// Suspected GRB
#define DEF_GRB             0x00000002	// Definitely a GRB
#define NEAR_SUN            0x00000004	// Coords is near the Sun (<15deg)
#define SOFT_SF             0x00000008	// Spectrum is soft, h_ratio > 2.0
#define SUSP_PE             0x00000010	// Suspected Particle Event
#define DEF_PE              0x00000020	// Definitely a Particle Event
#define X_X_X_X_XXX         0x00000040	// spare
#define DEF_UNK             0x00000080	// Definitely an Unknown
#define EARTHWARD           0x00000100	// Location towards Earth center
#define SOFT_FLAG           0x00000200	// Small hardness ratio (>1.5)
#define NEAR_SAA            0x00000400	// S/c is near/in the SAA region
#define DEF_SAA             0x00000800	// Definitely an SAA region
#define SUSP_SF             0x00001000	// Suspected Solar Flare
#define DEF_SF              0x00002000	// Definitely a Solar Flare
#define OP_FLAG             0x00004000	// Op-dets flag set
#define DEF_NOT_GRB         0x00008000	// Definitely not a GRB event
#define ISO_PE              0x00010000	// Datelowe Iso param is small (PE)
#define ISO_GRB             0x00020000	// D-Iso param is large (GRB/SF)
#define NEG_H_RATIO         0x00040000	// Negative h_ratio
#define NEG_ISO_BC          0x00080000	// Negative iso_part[1] or iso_part[2]
#define NOT_SOFT            0x00100000	// Not soft flag, GRB or PE
#define HI_ISO_RATIO        0x00200000	// Hi C3/C2 D-Iso ratio
#define LOW_INTEN           0x00400000	// Inten too small to be a real GRB

// Masks for extractions for the GRO-BATSE-based MISC in the packet:
#define TM_IND_MASK         0x00000001	// TM Indicator mask
#define BAD_CALC_MASK       0x00000002	// Singular matrix (bad calculation)
#define C_FOV_MASK          0x00000004	// COMPTEL FOV indicator mask
#define E_FOV_MASK          0x00000008	// EGRET FOV indicator mask
#define O_FOV_MASK          0x00000010	// OSSE FOV indicator mask
#define PROG_LEV_MASK       0x000000E0	// Program Algorithm Level mask
#define NOTICE_TYPE_MASK    0x00000300	// Notification Type mask
#define A_ASD_MASK          0x00000400	// ALEXIS Anti-Solar Dir ind mask
#define IPN_PEAK_MASK       0x00000800	// Passed the IPN Peak Inten thresh
#define IPN_FLUE_MASK       0x00001000	// Passed the IPN Fluence thresh
#define XTE_CRITERIA        0x00002000	// Meets XTE-PCA follow-up criteria
#define OBS_STATUS          0x00004000	// Won't/will obs & Didn't/did see
//#define SPARE2_MASK       0x00007800	// spare bits
#define NO_TRIG_MASK        0x00008000	// BATSE is not in a triggerable mode
#define VER_MINOR_MASK      0x0FFF0000	// Program Minor Version Number mask
#define VER_MAJOR_MASK      0xF0000000	// Program Major Version Number mask


// The HETE-based packet word definitions:
// The definitions for 0-8 are the same as in the BATSE-based packet.
#define H_TRIG_FLAGS	9	// See trigger flag bits below
#define H_GAMMA_CTS_S	10	// cts/s defined by gamma, can be 0
#define H_WXM_S_N 	11	// Trigger score [S/N]
#define H_SXC_CTS_S	12	// cts/s defined by SXC, can be 0
#define H_GAMMA_TSCALE	13	// Timescale in milliseconds
#define H_WXM_TSCALE	14	// Timescale in milliseconds
#define H_POINTING	15	// RA/DEC in upper/lower word [deg]
#define H_WXM_CNR1_RA	16	// 1st corner of WXM error box
#define H_WXM_CNR1_DEC	17	// 1st corner of WXM error box
#define H_WXM_CNR2_RA	18	// 2nd corner of WXM error box
#define H_WXM_CNR2_DEC	19	// 2nd corner of WXM error box
#define H_WXM_CNR3_RA	20	// 3rd corner of WXM error box
#define H_WXM_CNR3_DEC	21	// 3rd corner of WXM error box
#define H_WXM_CNR4_RA	22	// 4th corner of WXM error box
#define H_WXM_CNR4_DEC	23	// 4th corner of WXM error box
#define H_WXM_ERRORS	24	// Arcsec, stat high, sys low
#define H_WXM_DIM_NSIG	25	// Dim [arcsec] << 16 | significance
#define H_SXC_CNR1_RA	26	// 1st corner of SXC error box
#define H_SXC_CNR1_DEC	27	// 1st corner of SXC error box
#define H_SXC_CNR2_RA	28	// 2nd corner of SXC error box
#define H_SXC_CNR2_DEC	29	// 2nd corner of SXC error box
#define H_SXC_CNR3_RA	30	// 3rd corner of SXC error box
#define H_SXC_CNR3_DEC	31	// 3rd corner of SXC error box
#define H_SXC_CNR4_RA	32	// 4th corner of SXC error box
#define H_SXC_CNR4_DEC	33	// 4th corner of SXC error box
#define H_SXC_ERRORS	34	// Arcsec, stat high, sys low
#define H_SXC_DIM_NSIG	35	// Dim [arcsec] << 16 | significance
#define H_POS_FLAGS	36	// Collection of various info bits
#define H_VALIDITY	37	// Tells whether burst is valid

// HETE word and bit mask and shift definitions:
#define H_MAXDIM_MASK	0xffff0000	// Dimension in upper bits
#define H_MAXDIM_SHIFT	16		// Dimension in upper bits
#define H_NSIG_MASK	0x0000ffff	// Significance in lower bits
#define H_NSIG_SHIFT	0		// Significance in lower bits
#define H_P_DEC_MASK	0x0000ffff	// Dec in lower word
#define H_P_DEC_SHIFT	0		// Dec in lower word
#define H_P_RA_MASK	0xffff0000	// RA in upper word
#define H_P_RA_SHIFT	16		// RA in upper word
#define H_SEQNUM_MASK	0xffff0000	// Message number in low bits
#define H_SEQNUM_SHIFT	16		// Message number in low bits
#define H_STATERR_MASK	0xffff0000	// Statistical error in upper bits
#define H_STATERR_SHIFT	16		// Statistical error in upper bits
#define H_SYSERR_MASK	0x0000ffff	// Systematic error in lower bits
#define H_SYSERR_SHIFT	0		// Systematic error in lower bits
#define H_TRIGNUM_MASK	0x0000ffff	// Trigger number in low bits
#define H_TRIGNUM_SHIFT	0		// Trigger number in low bits
#define H_WHO_TRIG_MASK	0x00000007	// Gamma, XG, or SXC
#define H_X_LC_SN	0xff000000	// WXM X LC (aka Virtue) value in slot 38
#define H_Y_LC_SN	0x00ff0000	// WXM Y LC (aka Virtue) value in slot 38
#define H_X_IMAGE_SN	0x0000ff00	// WXM X (aka Goodness) value in slot 38
#define H_Y_IMAGE_SN	0x000000ff	// WXM Y (aka Goodness) value in slot 38

// HETE H_TRIG_FLAGS bit_flags definitions:
#define H_GAMMA_TRIG	0x00000001	// Did FREGATE trigger? 1=yes
#define H_WXM_TRIG	0x00000002	// Did WXM trigger?
#define H_SXC_TRIG	0x00000004	// Did SXC trigger?
#define H_ART_TRIG	0x00000008	// Artificial trigger
#define H_POSS_GRB	0x00000010	// Possible GRB
#define H_DEF_GRB 	0x00000020	// Definite GRB
#define H_DEF_NOT_GRB	0x00000040	// Definitely NOT a GRB
#define H_NEAR_SAA	0x00000080	// S/c is in or near the SAA
#define H_POSS_SGR	0x00000100	// Possible SGR
#define H_DEF_SGR 	0x00000200	// Definite SGR
#define H_POSS_XRB	0x00000400	// Possible XRB
#define H_DEF_XRB 	0x00000800	// Definite XRB
#define H_GAMMA_DATA	0x00001000	// FREGATE (gamma) data in this message
#define H_WXM_DATA	0x00002000	// WXM data in this message
#define H_SXC_DATA	0x00004000	// SXC data in this message
#define H_OPT_DATA	0x00008000	// OPT (s/c ACS) data in this message
#define H_WXM_POS 	0x00010000	// WXM position in this message
#define H_SXC_POS 	0x00020000	// SXC position in this message
#define H_TRIG_spare1	0x000C0000	// spare1
#define H_USE_TRIG_SN	0x00400000	// Use H_WXM_S_N
#define H_TRIG_spare2	0x00800000	// spare2
#define H_SXC_EN_TRIG	0x01000000	// Triggerred in the 1.5-12 keV band
#define H_LOW_EN_TRIG	0x02000000	// Triggerred in the 2-30 keV band
#define H_MID_EN_TRIG	0x04000000	// Triggerred in the 6-120 keV band
#define H_HI_EN_TRIG	0x08000000	// Triggerred in the 25-400 keV band
#define H_PROB_XRB	0x10000000	// Probable XRB
#define H_PROB_SGR	0x20000000	// Probable SGR
#define H_PROB_GRB	0x40000000	// Probable GRB
#define H_TRIG_spare3	0x80000000	// spare3

// HETE POS_FLAGS bitmap definitions:
#define H_WXM_SXC_SAME	0x00000001	// Positions are consistent
#define H_WXM_SXC_DIFF	0x00000002	// Positions are inconsistent
#define H_WXM_LOW_SIG	0x00000004	// NSIG below a threshold
#define H_SXC_LOW_SIG	0x00000008	// NSIG below a threshold
#define H_GAMMA_REFINED	0x00000010	// Gamma refined since FINAL
#define H_WXM_REFINED	0x00000020	// WXM refined since FINAL
#define H_SXC_REFINED	0x00000040	// SXC refined since FINAL
#define H_POS_spare	0x00FFFFB0	// spare
#define H_SC_LONG	0xFF000000	// S/C Longitude

// HETE VALIDITY flag bit masks:
#define H_BURST_VALID	0x00000001	// Burst declared valid
#define H_BURST_INVALID	0x00000002	// Burst declared invalid
#define H_VAL0x4_spare	0x00000004	// spare
#define H_VAL0x8_spare	0x00000008	// spare
#define H_EMERGE_TRIG	0x00000010	// Emersion trigger
#define H_KNOWN_XRS	0x00000020	// Known X-ray source
#define H_NO_POSITION	0x00000040	// No WXM or SXC position
#define H_VAL0x80_spare	0x00000080	// spare
#define H_OPS_ERROR	0x00000100	// Bad burst: s/c & inst ops error
#define H_PARTICLES	0x00000200	// Bad burst: particles
#define H_BAD_FLT_LOC	0x00000400	// Bad burst: bad flight location
#define H_BAD_GND_LOC	0x00000800	// Bad burst: bad ground location
#define H_RISING_BACK	0x00001000	// Bad burst: rising background
#define H_POISSON_TRIG	0x00002000	// Bad burst: poisson fluctuation trigger
#define H_OUTSIDE_FOV	0x00004000	// Good burst: but outside WSX/SXC FOV
#define H_IPN_CROSSING	0x00008000	// Good burst: IPN crossing match
#define H_VALID_spare	0x1FFFC000	// spare
#define H_EMAIL_METHOD	0x20000000	// email import method(=1, socket=0)
#define H_INTERNAL_TEST	0x40000000	// HETE_OPS GndAna test/debug loop
#define H_NOT_A_BURST	0x80000000	// GndAna shows this trigger to be non-GRB

// INTEGRAL word and bit mask and shift definitions:
#define I_TRIGNUM_MASK	0x0000ffff	// Trigger number in low bits
#define I_TRIGNUM_SHIFT	0			// Trigger number in low bits
#define I_SEQNUM_MASK	0xffff0000	// Message number in low bits
#define I_SEQNUM_SHIFT	16			// Message number in low bits

// The Swift-based packet word definitions:
#define BURST_URL     22            // Location of the start of the URL string in the pkt

// SWIFT word and bit mask and shift definitions:
#define S_TRIGNUM_MASK	0x00ffffff	// Trigger number in low bits
#define S_TRIGNUM_SHIFT	0			// Trigger number in low bits
#define S_SEGNUM_MASK	0x000000ff	// Observation Segment number in upper bits
#define S_SEGNUM_SHIFT	24			// Observation Segment number in upper bits


// The GCN defined packet types (missing numbers are gcn-internal use only):
// Types that are commented out are no longer available (eg GRO de-orbit).
#define TYPE_UNDEF		0   // This packet type is undefined
//#define TYPE_GRB_COORDS	1   // BATSE-Original Trigger coords packet
#define TYPE_TEST_COORDS	2   // Test coords packet
#define TYPE_IM_ALIVE		3   // I'm_alive packet
#define TYPE_KILL_SOCKET	4   // Kill a socket connection packet
//#define TYPE_MAXBC		11  // MAXC1/BC packet
#define TYPE_BRAD_COORDS	21  // Special Test coords packet for BRADFORD
//#define TYPE_GRB_FINAL	22  // BATSE-Final coords packet
//#define TYPE_HUNTS_SRC	24  // Huntsville LOCBURST GRB coords packet
#define TYPE_ALEXIS_SRC		25  // ALEXIS Transient coords packet
#define TYPE_XTE_PCA_ALERT	26  // XTE-PCA ToO Scheduled packet
#define TYPE_XTE_PCA_SRC	27  // XTE-PCA GRB coords packet
#define TYPE_XTE_ASM_ALERT	28  // XTE-ASM Alert packet
#define TYPE_XTE_ASM_SRC	29  // XTE-ASM GRB coords packet
//#define TYPE_COMPTEL_SRC	30  // COMPTEL GRB coords packet
#define TYPE_IPN_RAW_SRC	31  // IPN_RAW GRB annulus coords packet
#define TYPE_IPN_SEG_SRC	32  // IPN_SEGment GRB annulus segment coords pkt
#define TYPE_SAX_WFC_ALERT	33  // SAX-WFC Alert packet
#define TYPE_SAX_WFC_SRC	34  // SAX-WFC GRB coords packet
#define TYPE_SAX_NFI_ALERT	35  // SAX-NFI Alert packet
#define TYPE_SAX_NFI_SRC	36  // SAX-NFI GRB coords packet
#define TYPE_XTE_ASM_TRANS	37  // XTE-ASM TRANSIENT coords packet
#define TYPE_spare_SRC		38  // spare
#define TYPE_IPN_POS_SRC	39  // IPN_POSition coords packet
#define TYPE_HETE_ALERT_SRC	40  // HETE S/C_Alert packet
#define TYPE_HETE_UPDATE_SRC	41  // HETE S/C_Update packet
#define TYPE_HETE_FINAL_SRC	42  // HETE S/C_Last packet
#define TYPE_HETE_GNDANA_SRC	43  // HETE Ground Analysis packet
#define TYPE_HETE_TEST		44  // HETE Test packet
#define TYPE_GRB_CNTRPART_SRC	45  // GRB Counterpart coords packet
#define TYPE_INTEGRAL_POINTDIR_SRC  51  // INTEGRAL Pointing Dir packet
#define TYPE_INTEGRAL_SPIACS_SRC    52  // INTEGRAL SPIACS packet
#define TYPE_INTEGRAL_WAKEUP_SRC    53  // INTEGRAL Wakeup packet
#define TYPE_INTEGRAL_REFINED_SRC   54  // INTEGRAL Refined packet
#define TYPE_INTEGRAL_OFFLINE_SRC   55  // INTEGRAL Offline packet
#define TYPE_SNEWS_POS_SRC          57  // SNEWS Position/Test message
#define TYPE_MILAGRO_POS_SRC        58  // MILAGRO Position message
#define TYPE_KONUS_LC_SRC           59  // KONUS Lightcurve message
#define TYPE_SWIFT_BAT_GRB_ALERT_SRC     60  // SWIFT BAT GRB ALERT message
#define TYPE_SWIFT_BAT_GRB_POS_ACK_SRC   61  // SWIFT BAT GRB Position Acknowledge message
#define TYPE_SWIFT_BAT_GRB_POS_NACK_SRC  62  // SWIFT BAT GRB Position NOT Acknowledge message
#define TYPE_SWIFT_BAT_GRB_LC_SRC        63  // SWIFT BAT GRB Lightcurve message
#define TYPE_SWIFT_SCALEDMAP_SRC         64  // SWIFT BAT Scaled Map message
#define TYPE_SWIFT_FOM_2OBSAT_SRC        65  // SWIFT BAT FOM to Observe message
#define TYPE_SWIFT_FOSC_2OBSAT_SRC       66  // SWIFT BAT S/C to Slew message
#define TYPE_SWIFT_XRT_POSITION_SRC      67  // SWIFT XRT Position message
#define TYPE_SWIFT_XRT_SPECTRUM_SRC      68  // SWIFT XRT Spectrum message
#define TYPE_SWIFT_XRT_IMAGE_SRC         69  // SWIFT XRT Image message (aka postage stamp)
#define TYPE_SWIFT_XRT_LC_SRC            70  // SWIFT XRT Lightcurve message (aka Prompt)
#define TYPE_SWIFT_XRT_CENTROID_SRC      71  // SWIFT XRT Position NOT Ack message (Centroid Error)
#define TYPE_SWIFT_UVOT_IMAGE_SRC        72  // SWIFT UVOT Image message (aka DarkBurst, aka GeNie)
#define TYPE_SWIFT_UVOT_SLIST_SRC        73  // SWIFT UVOT Source List message (aka FindChart)
#define TYPE_SWIFT_FULL_DATA_INIT_SRC    74  // SWIFT Full Data Set Initial message
#define TYPE_SWIFT_FULL_DATA_UPDATE_SRC  75  // SWIFT Full Data Set Updated message
#define TYPE_SWIFT_BAT_GRB_LC_PROC_SRC   76  // SWIFT BAT GRB Lightcurve processed message
#define TYPE_SWIFT_XRT_SPECTRUM_PROC_SRC 77  // SWIFT XRT Spectrum processed message
#define TYPE_SWIFT_XRT_IMAGE_PROC_SRC    78  // SWIFT XRT Image processed message
#define TYPE_SWIFT_UVOT_IMAGE_PROC_SRC   79  // SWIFT UVOT Image processed mesg (aka DarkBurst, aka GeNie)
#define TYPE_SWIFT_UVOT_SLIST_PROC_SRC   80  // SWIFT UVOT Source List processed message (aka FindChart)
#define TYPE_SWIFT_UVOT_POS_SRC          81  // SWIFT UVOT Position message
#define TYPE_SWIFT_BAT_GRB_POS_TEST      82  // SWIFT BAT GRB Position Test message
#define TYPE_SWIFT_POINTDIR_SRC          83  // SWIFT Pointing Direction message
#define TYPE_SWIFT_BAT_TRANS             84  // SWIFT BAT Transient Position message
#define TYPE_SWIFT_UVOT_POS_NACK_SRC     89  // SWIFT UVOT Position Nack message
//#define TYPE_SWIFT_TOO                 97  // SWIFT TOO non-public message (this will never exist)
#define TYPE_SWIFT_BAT_SUB_THRESHOLD_SRC 98  // SWIFT BAT Sub-Threshold Position message
#define TYPE_SWIFT_BAT_SLEW_POS_SRC      99  // SWIFT BAT SLEW Burst/Transient Position (the Harvard BATSS thing)
#define TYPE_AGILE_GRB_WAKEUP            100  // AGILE GRB Wake-Up Position message
#define TYPE_AGILE_GRB_PROMPT            101  // AGILE GRB Prompt Position message
#define TYPE_AGILE_GRB_REFINED           102  // AGILE GRB Refined Position message
#define TYPE_spare103_106                103  // values 103 thru 106 are not yet assigned
#define TYPE_AGILE_POINTDIR              107  // AGILE Pointing Direction
#define TYPE_AGILE_TRANS                 108  // AGILE Transient Position message
#define TYPE_AGILE_GRB_POS_TEST          109  // AGILE GRB Position Test message
#define TYPE_FERMI_GBM_ALERT             110  // FERMI GBM Trigger Alert message
#define TYPE_FERMI_GBM_FLT_POS           111  // FERMI GBM Flt-cal Position message
#define TYPE_FERMI_GBM_GND_POS           112  // FERMI GBM Gnd-cal Position message
#define TYPE_FERMI_GBM_LC                113  // FERMI GBM GRB Lightcurve message
#define TYPE_spare114_117                117  // values 114 thru 117 are not yet assigned
#define TYPE_FERMI_GBM_TRANS             118  // FERMI GBM Transient Position mesg
#define TYPE_FERMI_GBM_POS_TEST          119  // FERMI GBM GRB Position Test message
#define TYPE_FERMI_LAT_POS_INI           120  // FERMI LAT Position Initial message
#define TYPE_FERMI_LAT_POS_UPD           121  // FERMI LAT Position Update message
#define TYPE_FERMI_LAT_POS_DIAG          122  // FERMI LAT Position Diagnostic message
#define TYPE_FERMI_LAT_TRANS             123  // FERMI LAT Transient Position mesg (eg AGN, Solar Flare)
#define TYPE_FERMI_LAT_POS_TEST          124  // FERMI LAT Position Test message
#define TYPE_FERMI_OBS_REQ               125  // FERMI Observe_Request message
#define TYPE_FERMI_SC_SLEW               126  // FERMI SC_Slew message
#define TYPE_FERMI_LAT_GND_REF           127  // FERMI LAT Ground-analysis Refined Pos message
#define TYPE_FERMI_LAT_GND_TRIG          128  // FERMI LAT Ground-analysis Trigger Pos message
#define TYPE_FERMI_POINTDIR              129  // FERMI Pointing Direction

// A few global variables for the socket_demo program:
int	inetsd;			// Socket descriptor
FILE	*lg;			// Logfile stream pointer
int	errno;			// Error return code number
time_t	tloc;			// Seconds since machine epoch
long	here_sod;		// Machine GMT converted to sec-of-day
long	last_here_sod;		// When the last imalive packet arrived
double	last_imalive_sod;	// SOD of the previous imalive packet

#define FIND_SXC	0	// Used in hete_same()
#define FIND_WXM	1	// Ditto; check the corners of a WXM box

/** 
 * Now I've modified this a bit to make it nicer but all of the 
 * functionality is still from demo_socket.c
 **/

/**
 * Various housekeeping functions
 **/

// Wrap things up if the socket connection breaks
void brokenpipe();

// Set up the server end of a connection
int server(char* hostname,int port , unsigned short type);

// Specialized error/status reporting routine
int serr(int fds, char* call);

// Are the four corners the same position?
int hete_same(long* hbuf,int find_which);

// Check and report an absence of Imalive packets rcvd
void chk_imalive(int which,time_t ctloc);

// Send e-mail when no Imalive packets
void e_mail_alert(int which);   

/**
 * All the following functions just read and return the individual packets
 * for each of the instruments (to a file right now.  Need to change that).
 **/

// Print the contents of received Imalive packets
void pr_imalive(long* lbuf, FILE* s); 
// Contents of TEST/Original/Final/MAXBC/LOCBURST pkts
void pr_packet(long* lbuf, FILE* s);   
void pr_kill(long* lbuf, FILE* s);
void pr_alexis(long* lbuf, FILE* s);
void pr_xte_pca(long* lbuf, FILE* s); 
void pr_xte_asm(long* lbuf, FILE* s);    
void pr_xte_asm_trans(long* lbuf, FILE* s); 
void pr_ipn_seg(long* lbuf, FILE* s);  
void pr_ipn_seg(long* lbuf, FILE* s);
void pr_sax_wfc(long* lbuf, FILE* s);
void pr_sax_nfi(long* lbuf, FILE* s);  
void pr_ipn_pos(long* lbuf, FILE* s);
void pr_hete(long* lbuf, FILE* s);
void pr_grb_cntrpart(long* lbuf, FILE* s); 
void pr_integral_point(long* lbuf, FILE* s);
void pr_integral_spiacs(long* lbuf, FILE* s);
void pr_integral_w_r_o(long* lbuf, FILE* s);
void pr_milagro(long* lbuf, FILE* s);
void pr_swift_bat_alert(long* lbuf, FILE* s);
void pr_swift_bat_pos_ack(long* lbuf, FILE* s);
void pr_swift_bat_pos_nack(long* lbuf, FILE* s);
void pr_swift_bat_lc(long* lbuf, FILE* s);
void pr_swift_fom_2obs(long* lbuf, FILE* s);
void pr_swift_sc_2slew(long* lbuf, FILE* s);
void pr_swift_xrt_pos_ack(long* lbuf, FILE* s);
void pr_swift_xrt_pos_nack(long* lbuf, FILE* s);
void pr_swift_xrt_spec(long* lbuf, FILE* s);
void pr_swift_xrt_image(long* lbuf, FILE* s);
void pr_swift_xrt_lc(long* lbuf, FILE* s);
void pr_swift_uvot_image(long* lbuf, FILE* s);
void pr_swift_uvot_slist(long* lbuf, FILE* s);
void pr_swift_uvot_pos(long* lbuf, FILE* s);
void pr_swift_point(long* lbuf, FILE* s); 

#endif // defined GCNSocket_H
