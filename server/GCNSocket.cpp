#include "GCNSocket.h"

/*---------------------------------------------------------------------------*/
void brokenpipe(){
    if(shutdown(inetsd,2) == -1)
	serr(0,"ERR: brokenpipe(): The shutdown did NOT work.\n");
    else
	{
	    printf(    "brokenpipe(): The shutdown worked OK.\n");
	    fprintf(lg,"brokenpipe(): The shutdown worked OK.\n");
	}
    if(close(inetsd))
	{
	    perror("brokenpipe(), close(): ");
	    printf(    "ERR: close() problem in brokenpipe(), errno=%d\n",errno);
	    fprintf(lg,"ERR: close() problem in brokenpipe(), errno=%d\n",errno);
	}
    inetsd = -1;
}

/*---------------------------------------------------------------------------*/
int server(char* hostname, int port, unsigned short type){

    int                 sock;      // The connected sock descriptor
    int                 sd=-1;     // The offerred sock descriptor
    socklen_t           temp;      // Dummy variable
    int                 saddrlen;  // Socket address length + 2
    char                on=1;      // Flag for nonblocking I/O
    struct sockaddr     saddr;     // Socket structure for UNIX
    struct sockaddr     *psaddr;   // Pointer to sin
    struct sockaddr_in  sin;       // Socket structure for inet


    temp = 0;
    // If the socket is for inet connection, then set up the sin structure.
    if(type == AF_INET)
	{
	    bzero((char *) &sin, sizeof(sin));
	    sin.sin_family = AF_INET;
	    sin.sin_addr.s_addr = htonl(INADDR_ANY);
#	ifdef LINUXPC
	    sin.sin_port = htons(port);
#	else
	    sin.sin_port = (unsigned short)port;
#	endif
	    psaddr = (struct sockaddr *) &sin;
	    saddrlen = sizeof(sin);
	}
    else
	{
	    // If the socket is for UNIX connection, then set up the saddr structure.
	    saddr.sa_family = AF_UNIX;
	    strcpy(saddr.sa_data, hostname);
	    psaddr = &saddr;
	    saddrlen = strlen(saddr.sa_data) + 2;
	}
 
    printf("server(): type=%d  (int)type=%d\n",type,(int)type);  // Debug
    
    // Initiate the socket connection.
    if((sd = socket((int)type, SOCK_STREAM, IPPROTO_TCP)) < 0)
	return(serr(sd,"server(): socket."));
    
    // Bind the name to the socket.
    if(bind(sd, psaddr, saddrlen) == -1)
	{
	    printf("bind() errno=%d\n",errno);
	    return(serr(sd,"server(): bind."));
	}
    
    // Listen for the socket connection from the GCN machine.
    if(listen(sd, 5))
	return(serr(sd,"server(): listen."));
    
    // Accept the socket connection from the GCN machine (the client).
    if((sock = accept(sd, &saddr, &temp)) < 0)
	return(serr(sd,"server(): accept."));
    
    close(sd);
    
    // Make the connection nonblocking I/O.
    if(ioctl(sock, FIONBIO, &on) < 0)
	return(serr(sock,"server(): ioctl."));
    
    serr(0,"server(): the server is up.");
    return(sock);
}

/*---------------------------------------------------------------------------*/
/* The error routine that prints the status of your socket connection. */
int serr(int fds, char* call){
    if(fds > 0)
	if(close(fds))
	    {
		perror("serr(), close(): ");
		printf("close() problem in serr(), errno=%d\n",errno);
	    }
    printf(    "%s\n",call);
    fprintf(lg,"%s\n",call);
    return(-1);
}

/*---------------------------------------------------------------------------*/
/* print the contents of the imalive packet */
void pr_imalive(long* lbuf,FILE* s){               

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
 fprintf(s,"   Type= %ld     SN  = %ld\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   TM_present: %s   Triggerable: %s   PKT_SOD= %.2f    delta=%5.2f\n",
									(lbuf[MISC] & TM_IND_MASK) ? "Yes":"No ",
									(lbuf[MISC] & NO_TRIG_MASK) ? "No ":"Yes",
									lbuf[PKT_SOD]/100.0,
									here_sod - lbuf[PKT_SOD]/100.0);
if(((lbuf[PKT_SOD]/100.0) - last_imalive_sod) > 62.0)
	fprintf(s,"ERR: Imalive packets generated at greater than 60sec intervals(=%.1f).\n",
									(lbuf[PKT_SOD]/100.0) - last_imalive_sod);
else if(((lbuf[PKT_SOD]/100.0) - last_imalive_sod) < 58.0)
	fprintf(s,"ERR: Imalive packets generated at less than 60sec intervals(=%.1f).\n",
									(lbuf[PKT_SOD]/100.0) - last_imalive_sod);
if((here_sod - last_here_sod) > 62.0)
	fprintf(s,"ERR: Imalive packets arrived at greater than 60sec intervals(=%ld).\n",
		(here_sod - last_here_sod));
else if((here_sod - last_here_sod) < 58.0)
	fprintf(s,"ERR: Imalive packets arrived at less than 60sec intervals(=%ld).\n",
		(here_sod - last_here_sod));
fflush(s);	/* This flush is optional -- it's useful for debugging */
}

/*---------------------------------------------------------------------------*/
/* print the contents of the BATSE-based or test packet*/
void pr_packet(long* lbuf, FILE* s){
int			i;		/* Loop var */
static char	*note_type[4] = {"Original","n/a","n/a","Final"};

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %ld     SN  = %ld\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %ld\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %ld\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %ld\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.2f [deg]\n", lbuf[BURST_RA]/100.0);
fprintf(s,"   Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/100.0);
fprintf(s,"   Inten=  %ld     Peak=%ld\n", lbuf[BURST_INTEN],lbuf[BURST_PEAK]);
fprintf(s,"   Error= %6.1f [deg]\n", lbuf[BURST_ERROR]/100.0);
fprintf(s,"   SC_Az= %7.2f [deg]\n", lbuf[SC_AZ]/100.0);
fprintf(s,"   SC_El= %7.2f [deg]\n", lbuf[SC_EL]/100.0);
fprintf(s,"   SC_X_RA=  %7.2f [deg]\n", lbuf[SC_X_RA]/100.0);
fprintf(s,"   SC_X_Dec= %7.2f [deg]\n", lbuf[SC_X_DEC]/100.0);
fprintf(s,"   SC_Z_RA=  %7.2f [deg]\n", lbuf[SC_Z_RA]/100.0);
fprintf(s,"   SC_Z_Dec= %7.2f [deg]\n", lbuf[SC_Z_DEC]/100.0);
 fprintf(s,"   Trig_id= %08x\n", (unsigned int)lbuf[TRIGGER_ID]);
 fprintf(s,"   Misc= %08x\n", (unsigned int)lbuf[MISC]);
fprintf(s,"   E_SC_Az=  %7.2f [deg]\n", lbuf[E_SC_AZ]/100.0);
fprintf(s,"   E_SC_El=  %7.2f [deg]\n", lbuf[E_SC_EL]/100.0);
fprintf(s,"   SC_Radius=%ld [km]\n", lbuf[SC_RADIUS]);
fprintf(s,"   Spares:");
for(i=PKT_SPARE23; i<=PKT_SPARE38; i++)
    fprintf(s," %x",(unsigned int)lbuf[i]);
fprintf(s,"\n");
fprintf(s,"   Pkt_term: %02x %02x %02x %02x\n",
										*((char *)(&lbuf[PKT_TERM]) + 0),
										*((char *)(&lbuf[PKT_TERM]) + 1),
										*((char *)(&lbuf[PKT_TERM]) + 2),
										*((char *)(&lbuf[PKT_TERM]) + 3));

fprintf(s,"   TM_present: %s\n", (lbuf[MISC] & TM_IND_MASK) ? "Yes" : "No ");
fprintf(s,"   NoticeType: %s\n",note_type[(lbuf[MISC] & NOTICE_TYPE_MASK) >>8]);
fprintf(s,"   Prog_level: %ld\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
fprintf(s,"   Prog_version: %ld.%02ld\n",
									((lbuf[MISC] & VER_MAJOR_MASK) >> 28)&0xF,
									(lbuf[MISC] & VER_MINOR_MASK) >> 16);
fflush(s);	/* This flush is optional -- it's useful for debugging */
}

/*---------------------------------------------------------------------------*/
/* print the contents of the KILL packet */
void pr_kill(long* lbuf, FILE* s)
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %ld     SN  = %ld\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %ld\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]\n", lbuf[PKT_SOD]/100.0);
}

/*---------------------------------------------------------------------------*/
/* print the contents of the ALEXIS packet */
void pr_alexis(long* lbuf, FILE* s)
{
int			i;	/* Loop var */
static char	tele[7][16]={"n/a", "1A, 93eV","1B, 70eV","2A, 93eV","2B, 66eV","3A, 70eV","3B, 66eV"};

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Ssn=    %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.2f [deg]\n", lbuf[BURST_RA]/100.0);
fprintf(s,"   Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/100.0);
fprintf(s,"   Error= %6.1f [deg]\n", lbuf[BURST_ERROR]/100.0);
fprintf(s,"   Alpha= %.2f\n", lbuf[SC_RADIUS]/100.0);
fprintf(s,"   Trig_id= %08x\n", lbuf[TRIGGER_ID]);
fprintf(s,"   Misc= %08x\n", lbuf[MISC]);
fprintf(s,"   Map_duration= %d [hrs]\n",lbuf[BURST_T_PEAK]);
fprintf(s,"   Telescope(=%d): %s\n",lbuf[24],tele[lbuf[24]]);
fprintf(s,"   Spares:");
for(i=PKT_SPARE23; i<=PKT_SPARE38; i++)
	fprintf(s," %x",lbuf[i]);
fprintf(s,"\n");
fprintf(s,"   Pkt_term: %02x %02x %02x %02x\n",
										*((char *)(&lbuf[PKT_TERM]) + 0),
										*((char *)(&lbuf[PKT_TERM]) + 1),
										*((char *)(&lbuf[PKT_TERM]) + 2),
										*((char *)(&lbuf[PKT_TERM]) + 3));

fprintf(s,"   TM_present: %s\n", (lbuf[MISC] & TM_IND_MASK) ? "Yes" : "No ");
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
fprintf(s,"   Prog_version: %d.%02d\n",((lbuf[MISC] & VER_MAJOR_MASK)>>28)&0xF,
									    (lbuf[MISC] & VER_MINOR_MASK) >> 16);
}

/*---------------------------------------------------------------------------*/
/* print the contents of the XTE-PCA packet */
void pr_xte_pca(long* lbuf, FILE* s)               
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
if(lbuf[MISC] & OBS_STATUS)
	{
	fprintf(s,"   RA=    %7.3f [deg]\n", lbuf[BURST_RA]/10000.0);
	fprintf(s,"   Dec=   %7.3f [deg]\n", lbuf[BURST_DEC]/10000.0);
	fprintf(s,"   Inten=  %d [mCrab]\n", lbuf[BURST_INTEN]);
	fprintf(s,"   Error= %6.2f [deg]\n", lbuf[BURST_ERROR]/10000.0);
	fprintf(s,"   Trig_id= %08x\n", lbuf[TRIGGER_ID]);
	fprintf(s,"   OBS_START_DATE= %d TJD\n", lbuf[24]);
	fprintf(s,"   OBS_START_SOD=  %d UT\n", lbuf[25]);
	}
else{
	fprintf(s,"RXTE-PCA could not localize this GRB.\n");
	}
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
}

/*---------------------------------------------------------------------------*/
/* print the contents of the XTE-ASM packet */
void pr_xte_asm(long* lbuf, FILE* s)
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
if(lbuf[MISC] & OBS_STATUS)
	{
	fprintf(s,"   RA=    %7.3f [deg]\n", lbuf[BURST_RA]/10000.0);
	fprintf(s,"   Dec=   %7.3f [deg]\n", lbuf[BURST_DEC]/10000.0);
	fprintf(s,"   Inten=  %d [mCrab]\n", lbuf[BURST_INTEN]);
	fprintf(s,"   Error= %6.2f [deg]\n", lbuf[BURST_ERROR]/10000.0);
	fprintf(s,"   Trig_id= %08x\n", lbuf[TRIGGER_ID]);
	fprintf(s,"   OBS_START_DATE= %d TJD\n", lbuf[24]);
	fprintf(s,"   OBS_START_SOD=  %d UT\n", lbuf[25]);
	}
else{
	fprintf(s,"RXTE-ASM could not localize this GRB.\n");
	}
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
}

/*---------------------------------------------------------------------------*/
/* print the contents of the XTE-ASM_TRANS packet */
void pr_xte_asm_trans(long* lbuf, FILE* s)
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   RefNum= %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.3f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=   %7.3f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Inten=  %.1f [mCrab]\n", lbuf[BURST_INTEN]/100.0);
fprintf(s,"   Error= %6.2f [deg]\n", lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   T_Since=  %d [sec]\n", lbuf[13]);
fprintf(s,"   ChiSq1=   %.2f\n", lbuf[14]/100.0);
fprintf(s,"   ChiSq2=   %.2f\n", lbuf[15]/100.0);
fprintf(s,"   SigNoise1=   %.2f\n", lbuf[16]/100.0);
fprintf(s,"   SigNoise2=   %.2f\n", lbuf[17]/100.0);
/*fprintf(s,"   Trig_ID=      %8x\n", lbuf[TRIGGER_ID]);*/
fprintf(s,"   RA_Crnr1=    %7.3f [deg]\n", lbuf[24]/10000.0);
fprintf(s,"   Dec_Crnr1=   %7.3f [deg]\n", lbuf[25]/10000.0);
fprintf(s,"   RA_Crnr2=    %7.3f [deg]\n", lbuf[26]/10000.0);
fprintf(s,"   Dec_Crnr2=   %7.3f [deg]\n", lbuf[27]/10000.0);
fprintf(s,"   RA_Crnr3=    %7.3f [deg]\n", lbuf[28]/10000.0);
fprintf(s,"   Dec_Crnr3=   %7.3f [deg]\n", lbuf[29]/10000.0);
fprintf(s,"   RA_Crnr4=    %7.3f [deg]\n", lbuf[30]/10000.0);
fprintf(s,"   Dec_Crnr4=   %7.3f [deg]\n", lbuf[31]/10000.0);
if(lbuf[MISC] | OBS_STATUS)
	fprintf(s,"   ASM_TRANSIENT Notices are all Cross_box type.\n");
else
	{
	fprintf(s,"   LineLength=  %7.3f [deg]\n", lbuf[32]/10000.0);
	fprintf(s,"   LineWidth=   %7.3f [deg]\n", lbuf[33]/10000.0);
	fprintf(s,"   PosAngle=    %7.3f [deg]\n", lbuf[34]/10000.0);
	}
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
}

/*---------------------------------------------------------------------------*/
/* print the contents of the IPN SEGMENT packet */
void pr_ipn_seg(long* lbuf, FILE* s)
{
fprintf(s,"ERR: IPN_SEG exists, but the printout routine doesn't exist yet.\n");
}

#ifdef SAX_NO_LONGER_AVAILABLE
/*---------------------------------------------------------------------------*/
/* print the contents of a SAX-WFC packet */
void pr_sax_wfc(long* lbuf, FILE* s)
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.3f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=   %7.3f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Inten=  %d [mCrab]\n", lbuf[BURST_INTEN]);
fprintf(s,"   Error= %6.2f [deg]\n", lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Trig_id= %08x\n", lbuf[TRIGGER_ID]);
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
}

/*---------------------------------------------------------------------------*/
/* print the contents of a SAX-NFI packet */
void pr_sax_nfi(long* lbuf, FILE* s)
{
fprintf(s,"SAX-NFI exists, but the printout routine doesn't exist yet.\n");
fprintf(s,"The NFI packet is very similar to the WFC packet.\n");
}
#endif

/*---------------------------------------------------------------------------*/
/* print the contents of the IPN POSITION packet */
void pr_ipn_pos(long* lbuf, FILE* s)   
{
fprintf(s,"IPN_POS exists, but the printout routine doesn't exist yet.\n");
}

/*---------------------------------------------------------------------------*/
/* Please note that not all of the many flag bits are extracted and printed
 * in this example routine.  The important ones are shown (plus all the
 * numerical fields).  Please refer to the sock_pkt_def_doc.html on the
 * GCN web site for the full details of the contents of these flag bits.     */
/* print the contents of the HETE-based packet */
void pr_hete(long* lbuf, FILE* s)             
{
unsigned short	max_arcsec, overall_goodness;
int				wxm_x_lc_sn, wxm_y_lc_sn, wxm_x_img_sn, wxm_y_img_sn;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d,  ", lbuf[PKT_TYPE]);
switch(lbuf[PKT_TYPE])
	{
	case TYPE_HETE_ALERT_SRC:	printf("S/C_Alert");	break;
	case TYPE_HETE_UPDATE_SRC:	printf("S/C_Update");	break;
	case TYPE_HETE_FINAL_SRC:	printf("S/C_Last");	break;
	case TYPE_HETE_GNDANA_SRC:	printf("GndAna");	break;
	case TYPE_HETE_TEST:		printf("Test");		break;
	default:					printf("Illegal");	break;
	}
fprintf(s,"   SN= %d\n", lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=     %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=     %d\n",(lbuf[BURST_TRIG] & H_TRIGNUM_MASK) >>H_TRIGNUM_SHIFT);
fprintf(s,"   Seq#=      %d\n",(lbuf[BURST_TRIG] & H_SEQNUM_MASK)  >>H_SEQNUM_SHIFT);
fprintf(s,"   BURST_TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   BURST_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);

fprintf(s,"   TRIGGER_SOURCE: ");
if(lbuf[H_TRIG_FLAGS] & H_SXC_EN_TRIG) 
	fprintf(s,"Trigger on the 1.5-12 keV band.\n");
else if(lbuf[H_TRIG_FLAGS] & H_LOW_EN_TRIG) 
	fprintf(s,"Trigger on the 2-30 keV band.\n");
else if(lbuf[H_TRIG_FLAGS] & H_MID_EN_TRIG) 
	fprintf(s,"Trigger on the 6-120 keV band.\n");
else if(lbuf[H_TRIG_FLAGS] & H_HI_EN_TRIG) 
	fprintf(s,"Trigger on the 25-400 keV band.\n");
else
	fprintf(s,"Error: No trigger source!\n");

fprintf(s,"   GAMMA_CTS_S:   %d [cnts/s] ", lbuf[H_GAMMA_CTS_S]);
fprintf(s,"on a %.3f sec timescale\n", (float)lbuf[H_GAMMA_TSCALE]/1000.0);
fprintf(s,"   WXM_SIG/NOISE:  %d [sig/noise]  ",lbuf[H_WXM_S_N]);
fprintf(s,"on a %.3f sec timescale\n", (float)lbuf[H_WXM_TSCALE]/1000.0);
if(lbuf[H_TRIG_FLAGS] & H_WXM_DATA)		/* Check for XG data and parse */
	fprintf(s,",  Data present.\n");
else
	fprintf(s,",  Data NOT present.\n");
fprintf(s,"   SXC_CTS_S:     %d\n", lbuf[H_SXC_CTS_S]);


if(lbuf[H_TRIG_FLAGS] & H_OPT_DATA)
	fprintf(s,"   S/C orientation is available.\n");
else
	fprintf(s,"   S/C orientation is NOT available.\n");
fprintf(s,"   SC_-Z_RA:    %4d [deg]\n",(unsigned short)(lbuf[H_POINTING] >> 16));
fprintf(s,"   SC_-Z_DEC:   %4d [deg]\n",(short)(lbuf[H_POINTING] & 0xffff));
fprintf(s,"   SC_LONG:     %3d [deg East]\n",
							2*(((lbuf[H_POS_FLAGS] & H_SC_LONG) >> 24) &0xFF)); 

fprintf(s,"   BURST_RA:    %7.3fd  (current)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   BURST_DEC:   %+7.3fd  (current)\n", lbuf[BURST_DEC]/10000.0);

if(lbuf[H_TRIG_FLAGS] & H_WXM_POS)	/* Flag says that WXM pos is available */
	fprintf(s,"   WXM position is available.\n");
else
	fprintf(s,"   WXM position is NOT available.\n");
fprintf(s,"   WXM_CORNER1:  %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_WXM_CNR1_RA]/10000.0,
										(float)lbuf[H_WXM_CNR1_DEC]/10000.0);
fprintf(s,"   WXM_CORNER2:  %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_WXM_CNR2_RA]/10000.0,
										(float)lbuf[H_WXM_CNR2_DEC]/10000.0);
fprintf(s,"   WXM_CORNER3:  %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_WXM_CNR3_RA]/10000.0,
										(float)lbuf[H_WXM_CNR3_DEC]/10000.0);
fprintf(s,"   WXM_CORNER4:  %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_WXM_CNR4_RA]/10000.0,
										(float)lbuf[H_WXM_CNR4_DEC]/10000.0);

max_arcsec = lbuf[H_WXM_DIM_NSIG] >> 16;
overall_goodness = lbuf[H_WXM_DIM_NSIG] & 0xffff;
fprintf(s,"   WXM_MAX_SIZE:  %.2f [arcmin]\n", (float)max_arcsec/60.0);
fprintf(s,"   WXM_LOC_SN:    %d\n", overall_goodness);
wxm_x_lc_sn  = ((lbuf[38] & H_X_LC_SN) >> 24) & 0xFF;
wxm_y_lc_sn  = ((lbuf[38] & H_Y_LC_SN) >> 16) & 0xFF;
wxm_x_img_sn = ((lbuf[38] & H_X_IMAGE_SN) >> 8) & 0xFF;
wxm_y_img_sn =  (lbuf[38] & H_Y_IMAGE_SN);
fprintf(s,"   WXM_IMAGE_SN:    X= %.1f Y= %.1f [sig/noise]\n",
										wxm_x_img_sn/10.0, wxm_y_img_sn/10.0);
fprintf(s,"   WXM_LC_SN:       X= %.1f Y= %.1f [sig/noise]\n",
										wxm_x_lc_sn/10.0, wxm_y_lc_sn/10.0);

if(lbuf[H_TRIG_FLAGS] & H_SXC_POS)	/* Flag says that SXC pos is available */
	fprintf(s,"   SXC position is available.\n");
else
	fprintf(s,"   SXC position is NOT available.\n");
fprintf(s,"   SXC_CORNER1:   %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_SXC_CNR1_RA]/10000.0,
										(float)lbuf[H_SXC_CNR1_DEC]/10000.0);
fprintf(s,"   SXC_CORNER2:   %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_SXC_CNR2_RA]/10000.0,
										(float)lbuf[H_SXC_CNR2_DEC]/10000.0);
fprintf(s,"   SXC_CORNER3:   %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_SXC_CNR3_RA]/10000.0,
										(float)lbuf[H_SXC_CNR3_DEC]/10000.0);
fprintf(s,"   SXC_CORNER4:   %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_SXC_CNR4_RA]/10000.0,
										(float)lbuf[H_SXC_CNR4_DEC]/10000.0);

	max_arcsec = lbuf[H_SXC_DIM_NSIG] >> 16;
	overall_goodness = lbuf[H_SXC_DIM_NSIG] & 0xffff;
	fprintf(s,"   SXC_MAX_SIZE:   %.2f [arcmin]\n", (float)max_arcsec/60.0);
	fprintf(s,"   SXC_LOC_SN:   %d\n", overall_goodness);

if(lbuf[H_TRIG_FLAGS] & H_ART_TRIG) 
	fprintf(s,"   COMMENT:   ARTIFICIAL BURST TRIGGER!\n");
if(!(lbuf[H_TRIG_FLAGS] & H_OPT_DATA))
	fprintf(s,"   COMMENT:   No s/c ACS pointing info available yet.\n");
if(lbuf[H_TRIG_FLAGS] & H_DEF_GRB)
	fprintf(s,"   COMMENTS:  Definite GRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_PROB_GRB)
	fprintf(s,"   COMMENTS:  Probable GRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_POSS_GRB)
	fprintf(s,"   COMMENTS:  Possible GRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_DEF_NOT_GRB)
	fprintf(s,"   COMMENTS:  Definitely not a GRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_DEF_SGR)
	fprintf(s,"   COMMENTS:  Definite SGR.\n");
if(lbuf[H_TRIG_FLAGS] & H_PROB_SGR)
	fprintf(s,"   COMMENTS:  Probable SGR.\n");
if(lbuf[H_TRIG_FLAGS] & H_POSS_SGR)
	fprintf(s,"   COMMENTS:  Possible SGR.\n");
if(lbuf[H_TRIG_FLAGS] & H_DEF_XRB)
	fprintf(s,"   COMMENTS:  Definite XRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_PROB_XRB)
	fprintf(s,"   COMMENTS:  Probable XRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_POSS_XRB)
	fprintf(s,"   COMMENTS:  Possible XRB.\n");

if(!(lbuf[H_VALIDITY] & H_EMAIL_METHOD))
 if(!(lbuf[H_TRIG_FLAGS] & H_DEF_GRB)    &&
   !(lbuf[H_TRIG_FLAGS] & H_PROB_GRB)    &&
   !(lbuf[H_TRIG_FLAGS] & H_POSS_GRB)    &&
   !(lbuf[H_TRIG_FLAGS] & H_DEF_NOT_GRB) &&
   !(lbuf[H_TRIG_FLAGS] & H_DEF_SGR)     &&
   !(lbuf[H_TRIG_FLAGS] & H_PROB_SGR)    &&
   !(lbuf[H_TRIG_FLAGS] & H_POSS_SGR)    &&
   !(lbuf[H_TRIG_FLAGS] & H_DEF_XRB)     &&
   !(lbuf[H_TRIG_FLAGS] & H_PROB_XRB)    &&
   !(lbuf[H_TRIG_FLAGS] & H_POSS_XRB)       )
	fprintf(s,"COMMENTS:       Possible GRB.\n");

 if(lbuf[H_TRIG_FLAGS] & H_NEAR_SAA) 
     fprintf(s,"   COMMENT:   S/c is in or near the SAA.\n");
 
 if(lbuf[H_TRIG_FLAGS] & H_WXM_POS)/* Flag says that SXC position is available */
     if(hete_same(lbuf,FIND_WXM))
	 fprintf(s,"   COMMENT:   WXM error box is circular; not rectangular.\n");
 if(lbuf[H_TRIG_FLAGS] & H_SXC_POS)/* Flag says that SXC position is available */
     if(hete_same(lbuf,FIND_SXC))
	 fprintf(s,"   COMMENT:   SXC error box is circular; not rectangular.\n");

 if(lbuf[H_POS_FLAGS] & H_WXM_SXC_SAME) 
     fprintf(s,"   COMMENT:   The WXM & SXC positions are consistant; overlapping error boxes.\n");
 if(lbuf[H_POS_FLAGS] & H_WXM_SXC_DIFF) 
     fprintf(s,"   COMMENT:   The WXM & SXC positions are NOT consistant; non-overlapping error boxes.\n");
 if(lbuf[H_POS_FLAGS] & H_WXM_LOW_SIG) 
     fprintf(s,"   COMMENT:   WXM S/N is less than a reasonable value.\n");
 if(lbuf[H_POS_FLAGS] & H_SXC_LOW_SIG) 
     fprintf(s,"   COMMENT:   SXC S/N is less than a reasonable value.\n");


if((lbuf[PKT_TYPE] == TYPE_HETE_UPDATE_SRC) ||
   (lbuf[PKT_TYPE] == TYPE_HETE_FINAL_SRC)  ||
   (lbuf[PKT_TYPE] == TYPE_HETE_GNDANA_SRC)   )
{
if(lbuf[H_VALIDITY] & H_EMERGE_TRIG) 
	fprintf(s,"   COMMENT:   This trigger is the result of Earth Limb emersion.\n");
if(lbuf[H_VALIDITY] & H_KNOWN_XRS) 
	fprintf(s,"   COMMENT:   This position matches a known X-ray source.\n");
if( (((lbuf[H_TRIG_FLAGS] & H_WXM_POS) == 0) &&
	 ((lbuf[H_TRIG_FLAGS] & H_SXC_POS) == 0)   ) ||
    (lbuf[H_VALIDITY] & H_NO_POSITION)             )
	fprintf(s,"   COMMENT:   There is no position known for this trigger at this time.\n");
}

if((lbuf[PKT_TYPE] == TYPE_HETE_FINAL_SRC) ||
   (lbuf[PKT_TYPE] == TYPE_HETE_GNDANA_SRC)  )
{
if(lbuf[H_VALIDITY] & H_BURST_VALID) 
	fprintf(s,"   COMMENT:       Burst_Validity flag is true.\n");
if(lbuf[H_VALIDITY] & H_BURST_INVALID) 
	fprintf(s,"   COMMENT:       Burst_Invalidity flag is true.\n");
}
if(lbuf[PKT_TYPE] == TYPE_HETE_GNDANA_SRC)
{
if(lbuf[H_POS_FLAGS] & H_GAMMA_REFINED) 
	fprintf(s,"   COMMENT:   FREGATE data refined since S/C_Last Notice.\n");
if(lbuf[H_POS_FLAGS] & H_WXM_REFINED) 
	fprintf(s,"   COMMENT:   WXM data refined since S/C_Last Notice.\n");
if(lbuf[H_POS_FLAGS] & H_SXC_REFINED) 
	fprintf(s,"   COMMENT:   SXC data refined since S/C_Last Notice.\n");


if(lbuf[H_VALIDITY] & H_OPS_ERROR) 
	fprintf(s,"   COMMENT:   Invalid trigger due to operational error.\n");
if(lbuf[H_VALIDITY] & H_PARTICLES) 
	fprintf(s,"   COMMENT:   Particle event.\n");
if(lbuf[H_VALIDITY] & H_BAD_FLT_LOC) 
	fprintf(s,"   COMMENT:   Invalid localization by flight code.\n");
if(lbuf[H_VALIDITY] & H_BAD_GND_LOC) 
	fprintf(s,"   COMMENT:   Invalid localization by ground analysis code.\n");
if(lbuf[H_VALIDITY] & H_RISING_BACK) 
	fprintf(s,"   COMMENT:   Trigger caused by rising background levels.\n");
if(lbuf[H_VALIDITY] & H_POISSON_TRIG) 
	fprintf(s,"   COMMENT:   Background fluctuation exceeded trigger level.\n");
if(lbuf[H_VALIDITY] & H_OUTSIDE_FOV) 
	fprintf(s,"   COMMENT:   Burst was outside WXM and SXC fields-of-view.\n");
if(lbuf[H_VALIDITY] & H_IPN_CROSSING) 
	fprintf(s,"   COMMENT:   Burst coordinates consistent with IPN annulus.\n");
if(lbuf[H_VALIDITY] & H_NOT_A_BURST) 
	fprintf(s,"   COMMENT:   Ground & human analysis have determined this to be a non-GRB.\n");
}

if(lbuf[PKT_TYPE] == TYPE_HETE_TEST)
	{
	fprintf(s,"   COMMENT:\n");
	fprintf(s,"   COMMENT:   This is a TEST Notice.\n");
	fprintf(s,"   COMMENT:   And for this TEST Notice, most of the flags\n");
	fprintf(s,"   COMMENT:   have been turned on to show most of the fields.\n");
	}
}

/*---------------------------------------------------------------------------*/
/* print the contents of the GRB CouNTeRPART packet */
void pr_grb_cntrpart(long* lbuf, FILE* s)  
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN= %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Circ#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   GRB_TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   GRB_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   CntrPart_RA=    %7.2f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   CntrPart_Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   CntrPart_Inten=  %d\n", lbuf[BURST_INTEN]);
fprintf(s,"   CntrPart_Error= %6.1f [deg]\n", lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Filter= %d\n", lbuf[12]);
fprintf(s,"   Seeing= %.2f [arcsec]\n", lbuf[13]/100.0);
fprintf(s,"   Obs_TJD=    %d\n", lbuf[14]);
fprintf(s,"   Obs_SOD=  %.2f [sec]\n", lbuf[15]/100.0);
fprintf(s,"   Obs_Dur=  %.2f [sec]\n", lbuf[16]/100.0);
/* I need to finish this part: */
/*fprintf(s,"   Flags= %08x\n", lbuf[18]);
fprintf(s,"   Misc=  %08x\n", lbuf[19]);
fprintf(s,"   Tele=  %d\n", lbuf[??]);
fprintf(s,"   Obs_Filter=  %d\n", lbuf[??]);
fprintf(s,"   Red_Shift=  %.3f\n", lbuf[??]/10000.0); */
}

/*---------------------------------------------------------------------------*/
/* print the contents of a INTEGRAL POINTDIR packet */
void pr_integral_point(long* lbuf, FILE* s)
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,  Pointing Direction\n",
					lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   Next_Point_RA:  %8.4fd (J2000)\n", lbuf[14]/10000.0);
fprintf(s,"   Next_Point_Dec: %+8.4fd (J2000)\n",lbuf[15]/10000.0);
fprintf(s,"   Slew_Time:      %.2f SOD UT\n", lbuf[6]/100.0);
fprintf(s,"   Slew_Date:      %5d TJD\n", lbuf[5]);
fprintf(s,"   Test/MPos= 0x%08x\n", lbuf[12]);
fprintf(s,"   S/C_att=   0x%08x\n", lbuf[19]);
}

/*---------------------------------------------------------------------------*/
/* print the contents of a INTEGRAL SPIACS packet */
void pr_integral_spiacs(long* lbuf, FILE* s) 
{
int	id, seqnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d, SPI ACS\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] & I_TRIGNUM_MASK) >> I_TRIGNUM_SHIFT;
seqnum = (lbuf[BURST_TRIG] & I_SEQNUM_MASK)  >> I_SEQNUM_SHIFT;
fprintf(s,"   Trig#=  %d,  Subnum= %d\n",id,seqnum);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   Intensity= %.2f [sigma]\n", lbuf[BURST_INTEN]/100.0);
fprintf(s,"   TimeScale= %.2f [sigma]\n", lbuf[13]/10000.0);
fprintf(s,"   TimeError= %.2f [sigma]\n", lbuf[16]/10000.0);
fprintf(s,"   Det_Flags= 0x%08x\n", lbuf[9]);
fprintf(s,"   Test/MPos= 0x%08x\n", lbuf[12]);
fprintf(s,"   S/C_att=   0x%08x\n", lbuf[19]);
}

/*---------------------------------------------------------------------------*/
/* print the contents of a INTEGRAL_WAKEUP/REFINED/OFFLINE packet */
void pr_integral_w_r_o(long* lbuf, FILE* s)
{
int	id, seqnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,   ", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
switch(lbuf[PKT_TYPE])
	{
	case TYPE_INTEGRAL_WAKEUP_SRC:		fprintf(s,"Wakeup\n");			break;
	case TYPE_INTEGRAL_REFINED_SRC:		fprintf(s,"Refined\n");			break;
	case TYPE_INTEGRAL_OFFLINE_SRC:		fprintf(s,"Offline\n");			break;
	default:							fprintf(s,"Illegal\n");			break;
	}
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] & I_TRIGNUM_MASK) >> I_TRIGNUM_SHIFT;
seqnum = (lbuf[BURST_TRIG] & I_SEQNUM_MASK)  >> I_SEQNUM_SHIFT;
fprintf(s,"   Trig#=  %d,  Subnum= %d\n",id,seqnum);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.2f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error= %.2f [arcmin, radius, statistical only]\n",
											(float)lbuf[BURST_ERROR]/60.0);
fprintf(s,"   Inten= %.2f [sigma]\n", lbuf[BURST_INTEN]/100.0);
fprintf(s,"   SC_RA= %6.2f [deg] (J2000)\n", lbuf[14]/10000.0);
fprintf(s,"   SC_DEC=%6.2f [deg] (J2000)\n", lbuf[15]/10000.0);
fprintf(s,"   TimeScale= %.2f [sigma]\n", lbuf[13]/10000.0);
fprintf(s,"   TimeError= %.2f [sigma]\n", lbuf[16]/10000.0);
/* I'll give you more on these flags when the IBAS people define them: */
fprintf(s,"   Det_Flags= 0x%08x\n", lbuf[9]);
fprintf(s,"   Test/MPos= 0x%08x\n", lbuf[12]);
fprintf(s,"   S/C_att=   0x%08x\n", lbuf[19]);
}

/*---------------------------------------------------------------------------*/
/* print the contents of a MILAGRO Position packet */
void pr_milagro(long* lbuf, FILE* s)
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,   ", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
switch(lbuf[MISC] & 0x0F)
	{
	case 1:		fprintf(s,"Short\n");			break;
	case 2:		fprintf(s,"Medium(TBD)\n");		break;
	case 3:		fprintf(s,"Long(TBD)\n");		break;
	default:	fprintf(s,"Illegal\n");			break;
	}
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d,  Subnum= %d\n",lbuf[BURST_TRIG],(lbuf[MISC] >> 24) & 0xFF);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.2f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error= %.2f [deg, radius, statistical only]\n",
											(float)lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Inten= %d [events]\n", lbuf[BURST_INTEN]);
fprintf(s,"   Bkg=   %.2f [events]\n", lbuf[10]/10000.0);
fprintf(s,"   Duration=%.2f [sec]\n", lbuf[13]/100.0);
fprintf(s,"   Signif=%6.2e\n",pow((double)10.0,(double)-1.0*lbuf[12]/100.0));
fprintf(s,"   Annual_rate= %.2f [estimated occurances/year]\n", lbuf[14]/100.0);
fprintf(s,"   Zenith= %.2f [deg]\n", lbuf[15]/100.0);
if(lbuf[TRIGGER_ID] & 0x00000001)
	fprintf(s,"   Possible GRB.\n");
if(lbuf[TRIGGER_ID] & 0x00000002)
	fprintf(s,"   Definite GRB.\n");
if(lbuf[TRIGGER_ID] & (0x01 << 15))
	fprintf(s,"   Definately not a GRB.\n");
}

/*---------------------------------------------------------------------------*/
// The SWIFT_BAT_ALERT is currently not being sent to the TDRSS transmitter,
// so there is never any to receive and distribute through GCN.
// This is because the original motivation for it no longer exists.
// The ALERT type was created to come out at T_zero to "warm up" the transmitter
// and get the end-to-end bit-n-frame-sync's locked by the time the BAT_POS type
// became available ~20sec later (range: T+7-40sec, ave=20sec).
// But Swift is operating the TDRSS transmitter in a 24/7 mode, so it is always "warm".
/* print the contents of a Swift-BAT Alert packet */
void pr_swift_bat_alert(long* lbuf, FILE* s) 
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Alert\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=   %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=   %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=     %d,  Segnum= %d\n", id, segnum);
fprintf(s,"   TJD=       %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=       %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   Trig_Index=%d\n", lbuf[17]);
fprintf(s,"   Signif=    %.2f [sigma]\n", lbuf[21]/100.0);
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This packet was ground-reprocessed from flight-data.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-BAT Pos_Ack packet */
void pr_swift_bat_pos_ack(long* lbuf, FILE* s)
{
int i;
int id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
if(lbuf[PKT_TYPE] == TYPE_SWIFT_BAT_GRB_POS_ACK_SRC)
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Pos_Ack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_SWIFT_BAT_TRANS)
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Transient Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_SWIFT_BAT_GRB_POS_TEST)
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Test Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else
fprintf(s,"   Type= %d   SN= %d    Illegal type!\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=   %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=     %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=     %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=      %7.2f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=     %7.2f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error=   %.2f [arcmin, radius, statistical only]\n",
											(float)60.0*lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Inten=   %d [cnts]\n", lbuf[BURST_INTEN]);
fprintf(s,"   Peak=    %d [cnts]\n", lbuf[BURST_PEAK]);
fprintf(s,"   Int_Time=%.3f [sec]\n", lbuf[14]/250.0); // convert 4msec_ticks to sec
fprintf(s,"   Phi=     %6.2f [deg]\n", lbuf[12]/100.0);
fprintf(s,"   Theta=   %6.2f [deg]\n", lbuf[13]/100.0);
fprintf(s,"   Trig_Index=   %d\n", lbuf[17]);
fprintf(s,"   Soln_Status=  0x%x\n", lbuf[18]);
fprintf(s,"   Rate_Signif=  %.2f [sigma]\n", lbuf[21]/100.0); // signif of peak in FFT image
fprintf(s,"   Image_Signif= %.2f [sigma]\n", lbuf[20]/100.0); // signif of the rate trigger
fprintf(s,"   Bkg_Inten=    %d [cnts]\n", lbuf[22]);
fprintf(s,"   Bkg_Time=     %.2f [sec]   delta=%.2f [sec]\n",
				(double)lbuf[23]/100.0, lbuf[BURST_SOD]/100.0 - lbuf[23]/100.0);
fprintf(s,"   Bkg_Dur=      %.2f [sec]\n",lbuf[24]/100.0);  // Still whole seconds
fprintf(s,"   Merit_Vals= ");
for(i=0;i<10;i++) fprintf(s," %d ",*(((char *)(lbuf+36))+i));
fprintf(s,"\n");
if(lbuf[MISC] & 0x10000000)
	{
	fprintf(s,"\n");
	fprintf(s,"   Since all the BAT_Pos_Ack & BAT_LC & BAT_ScaledMap messages were missing in the real-time TDRSS stream,\n");
	fprintf(s,"   this Notice was reconstructed from the (delayed) Malindi-pass ground-generated BAT_Pos SERS message.\n");
	fprintf(s,"   As such, many of the fields above are undefined -- they are not available in the limited SERS messages.\n");
	fprintf(s,    "However, the GRB_RA/DEC, GRB_DATE/TIME, IMAGE_SIGNIF, and a few other fields are defined and valid.\n");
	fprintf(s,"\n");
	}
if(lbuf[MISC] & 0x20000000)
	{
	fprintf(s,"\n");
	fprintf(s,"   Since the real-time TDRSS version of the BAT_Pos_Ack message was missing in the TDRSS stream,\n");
	fprintf(s,"   this Notice was reconstructed from pieces of a LightCurve or ScaledMap message.\n");
	fprintf(s,"   As such, some of the fields above are undefined -- they are not available in the LC or SM messages.\n");
	fprintf(s,"\n");
	}
if(lbuf[MISC] & 0x01000000)
	{
	fprintf(s,"   This Notice was delayed by more than 60 sec past the end of the trigger integration interval;\n");
	fprintf(s,"   probably due to it occurring during a Malindi downlink session.\n");
	}
if(id < 99999)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   As a result of this TOO, only the GRB_RA/_DEC, SUN, MOON, and GAL_/ECL_COORDS fields will be valid!\n");
	}
if(lbuf[MISC] & 0x00800000)
	{
	fprintf(s,"   This Notice was originally generated with the GRB_DATE & GRB_TIME fields set to zero,\n");
	fprintf(s,"   so those fields are now filled using the spacecraft system clock (not the normal burst trigger_time).\n");
	fprintf(s,"   This is usually the result of an uploaded GRB_TOO -- this is not a normal BAT-detected burst.\n");
	}
if(lbuf[TRIGGER_ID] & 0x00000010)
	{
	fprintf(s,"   This is an image trigger.    (The RATE_SIGNIF & BKG_{INTEN, TIME, DUR} are undefined.)\n");
	}
else
	fprintf(s,"   This is a rate trigger.\n");
if(lbuf[TRIGGER_ID] & 0x00000001)
	fprintf(s,"   A point_source was found.\n");
else
	fprintf(s,"   A point_source was not found.\n");
if(lbuf[TRIGGER_ID] & 0x00000008)
	fprintf(s,"   This matches a source in the on-board catalog.\n");
else
	fprintf(s,"   This does not match any source in the on-board catalog.\n");
if(lbuf[TRIGGER_ID] & 0x00000100)
	fprintf(s,"   This matches a source in the ground catalog.\n");
else
	fprintf(s,"   This does not match any source in the ground catalog.\n");
if((lbuf[TRIGGER_ID] & 0x00000002)  && ((lbuf[TRIGGER_ID] & 0x00000020) == 0) && !(lbuf[TRIGGER_ID] & 0x00000100))
	fprintf(s,"   This is a GRB.\n");
else if(id < 99999)
	fprintf(s,"   This is a TOO.\n");
else
	fprintf(s,"   This is not a GRB.\n");
if(lbuf[TRIGGER_ID] & 0x00000004)
	fprintf(s,"   This is an interesting source.\n");
if(lbuf[TRIGGER_ID] & 0x00000040)
	fprintf(s,"   This trigger occurred during a high background rate interval.\n");
if(lbuf[TRIGGER_ID] & 0x00000080)
	fprintf(s,"   Since the IMAGE_SIGNIF is less than 7 sigma, this is a questionable detection.\n");
if(lbuf[TRIGGER_ID] & 0x00000200)
	fprintf(s,"   Since the Bkg_rate is greater than the foregnd_rate (exiting SAA), this is a questionable detection.\n");
if(lbuf[TRIGGER_ID] & 0x00000400)
    fprintf(s,"   This trigger occured while the StarTracker had lost lock, so it is probably bogus.\n");
if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This Notice was ground-reprocessed from flight-data.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-BAT Pos_Nack packet */
void pr_swift_bat_pos_nack(long* lbuf, FILE* s)
{
int i;
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Pos_Nack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=   %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=     %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=     %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   Trig_Index=   %d\n", lbuf[17]);
fprintf(s,"   Soln_Status=  0x%x\n", lbuf[18]);
//fprintf(s,"   Rate_Signif=  %.2f [sigma]\n", lbuf[21]/100.0);  // Always zero's
//fprintf(s,"   Image_Signif= %.2f [sigma]\n", lbuf[20]/100.0);
//fprintf(s,"   Bkg_Inten=    %d [cnts]\n", lbuf[22]);
//fprintf(s,"   Bkg_Time=     %.2f [sec]\n",(double)lbuf[23]/100.0);
//fprintf(s,"   Bkg_Dur=      %.2f [sec]\n",lbuf[24]/100);
fprintf(s,"   Merit_Vals= ");
for(i=0;i<10;i++) fprintf(s," %d ",*(((char *)(lbuf+36))+i));
fprintf(s,"\n");
if(lbuf[TRIGGER_ID] & 0x00000001)
	{
	fprintf(s,"   A point_source was found.\n");
	fprintf(s,"   ERR: a BAT_POS_NACK with the Pt-Src flag bit set!\n");
	}
else
	fprintf(s,"   A point_source was not found.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-BAT LightCurve packet */
void pr_swift_bat_lc(long* lbuf, FILE* s)
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-BAT LightCurve\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=        %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=        %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=         %7.2f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=        %7.2f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Trig_Index= %d\n", lbuf[17]);
fprintf(s,"   Phi=        %6.2f [deg]\n", lbuf[12]/100.0);
fprintf(s,"   Theta=      %6.2f [deg]\n", lbuf[13]/100.0);
fprintf(s,"   Delta_T=    %.2f [sec]\n", lbuf[14]/100.0);  // of the 3rd/final packet
fprintf(s,"   URL=        %s\n", (char *)(&lbuf[BURST_URL]));
if(lbuf[MISC] & 0x10000000)
	{
	fprintf(s,"   This lightcurve contains more than 50 illegal values -- probably an empty lightcurve.\n");
	fprintf(s,"   So the FITS file will have empty/missing rows -- possibly all rows will be missing.\n");
	}
if(lbuf[MISC] & 0x20000000)
	fprintf(s,"   This notice was forced out after watchdog timer expiring -- most likely due to missing packet(s).\n");
if(lbuf[MISC] & 0x08000000)
	fprintf(s,"   The 1st packet (of 3) was missing in the lightcurve data stream.\n");
if(lbuf[MISC] & 0x04000000)
	fprintf(s,"   The 2nd packet (of 3) was missing in the lightcurve data stream.\n");
if(lbuf[MISC] & 0x02000000)
	fprintf(s,"   The 3rd packet (of 3) was missing in the lightcurve data stream.\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This Notice was ground-reprocessed from flight-data.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & (0x1<<24))  // Only if this got a copy of BAT_POS_ACK soln_status
	{                       // will the following be valid/defined.
	fprintf(s,"\n");
	fprintf(s,"   The next comments were copied from the BAT_POS Notice:\n");
	if(lbuf[TRIGGER_ID] & 0x00000010)
		fprintf(s,"   This is an image trigger.\n");
	else
		fprintf(s,"   This is a rate trigger.\n");
	if(lbuf[TRIGGER_ID] & 0x00000001)
		fprintf(s,"   A point_source was found.\n");
	else
		fprintf(s,"   A point_source was not found.\n");
	if(lbuf[TRIGGER_ID] & 0x00000008)
		fprintf(s,"   This matches a source in the on-board catalog.\n");
	else
		fprintf(s,"   This does not match any source in the on-board catalog.\n");
	if(lbuf[TRIGGER_ID] & 0x00000100)
		fprintf(s,"   This matches a source in the ground catalog.\n");
	else
		fprintf(s,"   This does not match any source in the ground catalog.\n");
	if((lbuf[TRIGGER_ID] & 0x00000002) && ((lbuf[TRIGGER_ID] & 0x00000020) == 0) && ((lbuf[TRIGGER_ID] & 0x00000100) == 0))
		fprintf(s,"   This is a GRB.\n");   // grb  &&  !def_not_grb                &&     !gnd_catalog
	else
		fprintf(s,"   This is not a GRB.\n");
	if(lbuf[TRIGGER_ID] & 0x00000040)
		fprintf(s,"   This trigger occurred during a high background rate interval.\n");
	if(lbuf[TRIGGER_ID] & 0x00000080)
		fprintf(s,"   Since the IMAGE_SIGNIF is less than 7 sigma, this is a questionable detection.\n");
	if(lbuf[TRIGGER_ID] & 0x00000200)
		fprintf(s,"   Since the foreground interval count rate is less than the bkg interval (SAA exit), this is a questionable detection.\n");
	if(lbuf[TRIGGER_ID] & 0x00000400)
		fprintf(s,"   This trigger occured while the StarTracker had lost lock, so it is probably bogus.\n");
	if(lbuf[TRIGGER_ID] & 0x00000004)
		fprintf(s,"   This is an interesting source.\n");
	}
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-FOM 2Observe packet */
void pr_swift_fom_2obs(long* lbuf, FILE* s)
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift FOM2Observe\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=     %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=     %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=       %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=         %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=         %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=         %7.2f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=        %7.2f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Trig_Index=  %d\n", lbuf[17]);
fprintf(s,"   Rate_Signif= %.2f [sigma]\n", lbuf[21]/100.0);
fprintf(s,"   Flags=       0x%x\n", lbuf[18]);
fprintf(s,"   Merit=       %.2f\n", lbuf[38]/100.0);
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   As a result of this TOO, the TRIG# and Rate_Signif fields will not be valid!\n");
	fprintf(s,"   And sometimes the GRB_DATE/TIME fields are not valid!\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   As a result of this TOO, the TRIG# and Rate_Signif fields will not be valid!\n");
	fprintf(s,"   And sometimes the GRB_DATE/TIME fields are not valid!\n");
	}
else if(lbuf[MISC] & 0x20000000)
	fprintf(s,"   Because no BAT_Position Notice was received, this FOM Notice was forced out after watchdog expiration.\n");
if(lbuf[18] & 0x1)
	{
	fprintf(s,"   This was of sufficient merit to become the new Automated Target;\n");
	fprintf(s,"   but if the Current PPT has a greater merit, then we will not request a slew.\n");
	}
else
	{
	fprintf(s,"   This was NOT of sufficient merit to become the new Automated Target.\n");
	fprintf(s,"   FOM will NOT request the s/c to slew.\n");
	}
if(lbuf[18] & 0x2)
	{
	fprintf(s,"   FOM will request the s/c to slew.\n");
	fprintf(s,"   The S/C has yet to make its decision about safe to slew or not.\n");
	}
else
	fprintf(s,"   FOM will NOT request to observe.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
if(lbuf[16] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & (0x1<<24))  // Only if this got a copy of BAT_POS_ACK soln_status
	{                       // will the following be valid/defined.
	fprintf(s,"\n");
	fprintf(s,"   The next comments were copied from the BAT_POS Notice:\n");
	if(lbuf[16] & 0x00000010)
		fprintf(s,"   This is an image trigger.\n");
	else
		fprintf(s,"   This is a rate trigger.\n");
	if(lbuf[16] & 0x00000001)
		fprintf(s,"   A point_source was found.\n");
	else
		fprintf(s,"   A point_source was not found.\n");
	if(lbuf[16] & 0x00000008)
		fprintf(s,"   This matches a source in the on-board catalog.\n");
	else
		fprintf(s,"   This does not match any source in the on-board catalog.\n");
	if(lbuf[16] & 0x00000100)
		fprintf(s,"   This matches a source in the ground catalog.\n");
	else
		fprintf(s,"   This does not match any source in the ground catalog.\n");
	if((lbuf[16] & 0x00000002) && ((lbuf[16] & 0x00000020) == 0) && ((lbuf[16] & 0x00000100) == 0))
		fprintf(s,"   This is a GRB.\n");   // grb  &&  !def_not_grb                &&     !gnd_catalog
	else
		fprintf(s,"   This is not a GRB.\n");
	if(lbuf[16] & 0x00000040)
		fprintf(s,"   This trigger occurred during a high background rate interval.\n");
	if(lbuf[16] & 0x00000080)
		fprintf(s,"   Since the IMAGE_SIGNIF is less than 7 sigma, this is a questionable detection.\n");
	if(lbuf[16] & 0x00000800)
		fprintf(s,"   The IMAGE_SIGNIF is VERY LOW, this is almost certainly not real.\n");
	if(lbuf[16] & 0x00000200)
		fprintf(s,"   Since the foreground interval count rate is less than the bkg interval (SAA exit), this is a questionable detection.\n");
	if(lbuf[16] & 0x00000400)
		fprintf(s,"   This trigger occured while the StarTracker had lost lock, so it is probably bogus.\n");
	if(lbuf[16] & 0x00000004)
		fprintf(s,"   This is an interesting source.\n");
	}
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-FOM 2Slew packet */
void pr_swift_sc_2slew(long* lbuf, FILE* s)
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift SC2Slew\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=        %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=        %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=       %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=      %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Roll=     %9.4f [deg] (J2000)\n", lbuf[9]/10000.0);
fprintf(s,"   Trig_Index= %d\n", lbuf[17]);
fprintf(s,"   Rate_Signif=%.2f [sigma]\n", lbuf[21]/100.0);
fprintf(s,"   Slew_Query= %d\n", lbuf[18]);
fprintf(s,"   Wait_Time=  %.2f [sec]\n", lbuf[13]/100.0);
fprintf(s,"   Obs_Time=   %.2f [sec]\n", lbuf[14]/100.0);
fprintf(s,"   Inst_Modes: BAT: %d   XRT: %d   UVOT: %d\n",lbuf[22],lbuf[23],lbuf[24]);
fprintf(s,"   Merit=      %.2f\n", lbuf[38]/100.0);
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   As a result of this TOO, the TRIG# and Rate_Signif fields will not be valid!\n");
	fprintf(s,"   And sometimes the GRB_DATE/TIME fields are not valid!\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   As a result of this TOO, the TRIG# and Rate_Signif fields will not be valid!\n");
	fprintf(s,"   And sometimes the GRB_DATE/TIME fields are not valid!\n");
	}
if(lbuf[18] == 0)                                          // The SC reply
	{
	if(lbuf[13] == 0)
	fprintf(s,"   SC will immediately slew to this target.\n");
	else
	fprintf(s,"   SC will not immediately slew to this target; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 1)
	{
	fprintf(s,"   SC will NOT slew to this target; Sun constraint.\n");
	fprintf(s,"   There maybe a delayed slew or no slew at all; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 2)
	{
	fprintf(s,"   SC will NOT slew to this target; Earth_limb constraint.\n");
	fprintf(s,"   There maybe a delayed slew or no slew at all; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 3)
	{
	fprintf(s,"   SC will NOT slew to this target; Moon constraint.\n");
	fprintf(s,"   There maybe a delayed slew or no slew at all; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 4)
	{
	fprintf(s,"   SC will NOT slew to this target; Ram_vector constraint.\n");
	fprintf(s,"   There maybe a delayed slew or no slew at all; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 5)
	fprintf(s,"   SC will NOT slew to this target; Invalid slew_request parameter value.\n");
else
	fprintf(s,"   Unknown reply_code.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
if(lbuf[16] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & (0x1<<24))  // Only if this got a copy of BAT_POS_ACK soln_status
	{                       // will the following be valid/defined.
	fprintf(s,"\n");
	fprintf(s,"   The next comments were copied from the BAT_POS Notice:\n");
	if(lbuf[16] & 0x00000010)
		fprintf(s,"   This is an image trigger.\n");
	else
		fprintf(s,"   This is a rate trigger.\n");
	if(lbuf[16] & 0x00000001)
		fprintf(s,"   A point_source was found.\n");
	else
		fprintf(s,"   A point_source was not found.\n");
	if(lbuf[16] & 0x00000008)
		fprintf(s,"   This matches a source in the on-board catalog.\n");
	else
		fprintf(s,"   This does not match any source in the on-board catalog.\n");
	if(lbuf[16] & 0x00000100)
		fprintf(s,"   This matches a source in the ground catalog.\n");
	else
		fprintf(s,"   This does not match any source in the ground catalog.\n");
	if((lbuf[16] & 0x00000002) && ((lbuf[16] & 0x00000020) == 0) && ((lbuf[16] & 0x00000100) == 0))
		fprintf(s,"   This is a GRB.\n");   // grb  &&  !def_not_grb                &&     !gnd_catalog
	else
		fprintf(s,"   This is not a GRB.\n");
	if(lbuf[16] & 0x00000040)
		fprintf(s,"   This trigger occurred during a high background rate interval.\n");
	if(lbuf[16] & 0x00000080)
		fprintf(s,"   Since the IMAGE_SIGNIF is less than 7 sigma, this is a questionable detection.\n");
	if(lbuf[16] & 0x00000200)
		fprintf(s,"   Since the foreground interval count rate is less than the bkg interval (SAA exit), this is a questionable detection.\n");
	if(lbuf[16] & 0x00000400)
		fprintf(s,"   This trigger occured while the StarTracker had lost lock, so it is probably bogus.\n");
	if(lbuf[16] & 0x00000004)
		fprintf(s,"   This is an interesting source.\n");
	}
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-XRT Pos_Ack packet */
void pr_swift_xrt_pos_ack(long* lbuf, FILE* s)
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT Pos_Ack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=  %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=    %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=%d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=%.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=       %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=      %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error=    %.1f [arcsec, radius, statistical only]\n",
											(float)3600.0*lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Flux=     %.2f [arb]\n", lbuf[BURST_INTEN]/100.0);  //this are arbitrary units
fprintf(s,"   Signif=   %.2f [sigma]\n", lbuf[21]/100.0); // sqrt(num_pixels_w_photons)
fprintf(s,"   TAM[0]=   %7.2f\n", lbuf[12]/100.0);
fprintf(s,"   TAM[1]=   %7.2f\n", lbuf[13]/100.0);
fprintf(s,"   TAM[2]=   %7.2f\n", lbuf[14]/100.0);
fprintf(s,"   TAM[3]=   %7.2f\n", lbuf[15]/100.0);
fprintf(s,"   Amplifier=%d\n", (lbuf[17]>>8) & 0xFF);
fprintf(s,"   Waveform= %d\n",  lbuf[17] & 0xFF);
if(lbuf[MISC] & 0x10000000)
	{
	fprintf(s,"   This Notice was reconstructed from the (delayed) Malindi-pass ground-generated XRT_Pos SERS message.\n");
	fprintf(s,"   As such, many of the fields above are undefined -- they are not available in the limited SERS messages.\n");
	fprintf(s,"   However, the GRB_RA/DEC, IMG_START_DATE/TIME, GRB_INTEN, and a few other fields are defined and valid.\n");
	}
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the IMG_START_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the IMG_START_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[TRIGGER_ID] & 0x1)
	{
	fprintf(s,"   The object found at this position is either a very bright burst or a cosmic ray hit.\n");
	fprintf(s,"   Examine the XRT Image to differentiate (CRs are much more compact; see examples at:\n");
	fprintf(s,"   http://www.swift.psu.edu/xrt/XRT_Postage_Stamp_Image_Photo_Gallery.htm ).\n");
	}
if(lbuf[TRIGGER_ID] & 0x1)
	fprintf(s,"   The 'object' found for this position is very likely a cosmic ray hit in the CCD; not a burst afterglow.\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-XRT Pos_Nack packet */
void pr_swift_xrt_pos_nack(long* lbuf, FILE* s)
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT Pos_Nack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=  %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
								lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=    %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=%d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=%.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   Bore_RA=  %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Bore_Dec= %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Counts=   %d    Min_needed= %d\n",lbuf[BURST_INTEN],lbuf[10]);
fprintf(s,"   StdD=     %.2f   Max_StdDev_for_Good=%.2f  [arcsec]\n",
								3600.0*lbuf[11]/10000.0,3600.0*lbuf[12]/10000.0);
fprintf(s,"   Ph2_iter= %d    Max_iter_allowed= %d\n",lbuf[16],lbuf[17]);
fprintf(s,"   Err_Code= %d\n", lbuf[18]);
fprintf(s,"           = 1 = No source found in the image.\n");
fprintf(s,"           = 2 = Algorithm did not converge; too many interations.\n");
fprintf(s,"           = 3 = Standard deviation too large.\n");
fprintf(s,"           = 0xFFFF = General error.\n");
if(lbuf[18] == 1)
	fprintf(s,"   No source found in the image.\n");
else if(lbuf[18] == 2)
	fprintf(s,"   Algorithm did not converge; too many interations.\n");
else if(lbuf[18] == 3)
	fprintf(s,"   Standard deviation too large.\n");
else
	fprintf(s,"   UNRECOGNIZED ERROR CODE.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-XRT Spectrum packet */
void pr_swift_xrt_spec(long* lbuf, FILE* s)
{
int	  id, segnum;
float delta;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT Spectrum\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Bore_RA=    %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Bore_Dec=   %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Start_TJD=  %d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=  %.2f [sec]\n", lbuf[BURST_SOD]/100.0);
fprintf(s,"   Stop_TJD=   %d\n", lbuf[10]);
fprintf(s,"   Stop_SOD=   %.2f [sec]\n", lbuf[11]/100.0);
delta = lbuf[BURST_SOD]/100.0 - lbuf[11]/100.0;
if(delta < 0.0) delta += 86400.0;  // Handle UT_midnight rollover
fprintf(s,"   Stop_Start= %.2f [sec]\n", delta);
fprintf(s,"   LiveTime=   %.2f [sec]\n", lbuf[9]/100.0);
fprintf(s,"   Mode=       %d\n", lbuf[12]);
fprintf(s,"   Waveform=   %d\n", lbuf[13]);
fprintf(s,"   Bias=       %d\n", lbuf[14]);
fprintf(s,"   Term_cond=  %d\n", lbuf[21] & 0x07);
fprintf(s,"   URL=        %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the SPEC_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the SPEC_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((lbuf[21] & 0x07) == 1)
	fprintf(s,"   This spectrum accumulation was terminated by the 'Time' condition.\n");
else if((lbuf[21] & 0x07) == 2)
	fprintf(s,"   This spectrum accumulation was terminated by the 'Snapshot End' condition.\n");
else if((lbuf[21] & 0x07) == 3)
	fprintf(s,"   This spectrum accumulation was terminated by the 'SAA Entry' condition.\n");
else if((lbuf[21] & 0x07) == 4)
	fprintf(s,"   This spectrum accumulation was terminated by the 'LRPD-to-WT Transition' condition.\n");
else if((lbuf[21] & 0x07) == 5)
	fprintf(s,"   This spectrum accumulation was terminated by the 'WT-to-LRorPC Transition' condition.\n");
else
	fprintf(s,"   WARNING: This spectrum accumulation has an unrecognized termination condition.\n");
if(lbuf[MISC] & 0x08000000)
	fprintf(s,"   The 1st packet (of 3) was missing in the spectrum data stream.\n");
if(lbuf[MISC] & 0x04000000)
	fprintf(s,"   The 2nd packet (of 3) was missing in the spectrum data stream.\n");
if(lbuf[MISC] & 0x02000000)
	fprintf(s,"   The 3rd packet (of 3) was missing in the spectrum data stream.\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x20000000)
	fprintf(s,"   This notice was forced out after watchdog timer expiring -- most likely due to missing packet(s).\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-XRT Image packet */
void pr_swift_xrt_image(long* lbuf, FILE* s){
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT Image\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=  %d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=         %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=        %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error=      %.1f [arcsec]\n",3600.0*lbuf[11]/10000.0);
fprintf(s,"   Counts=     %d\n",lbuf[BURST_INTEN]);
fprintf(s,"   Centroid_X= %.2f\n",lbuf[12]/100.0);
fprintf(s,"   Centroid_Y= %.2f\n",lbuf[13]/100.0);
fprintf(s,"   Raw_X=      %d\n",lbuf[14]);
fprintf(s,"   Raw_Y=      %d\n",lbuf[15]);
fprintf(s,"   Roll=       %.4f [deg]\n",lbuf[16]/10000.0);
fprintf(s,"   ExpoTime=   %.2f [sec]\n",lbuf[18]/100.0);
fprintf(s,"   Gain=       %d\n",(lbuf[17] >> 16) & 0xFF);
fprintf(s,"   Mode=       %d\n",(lbuf[17] >>  8) & 0xFF);
fprintf(s,"   Waveform=   %d\n",(lbuf[17]      ) & 0xFF);
fprintf(s,"   GRB_Pos_in_XRT_Y= %.4f\n",lbuf[20]/100.0);
fprintf(s,"   GRB_Pos_in_XRT_Z= %.4f\n",lbuf[21]/100.0);
fprintf(s,"   URL=        %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[18] & 0x1)
	fprintf(s,"   The 'object' found for this position is very likely a cosmic ray hit in the CCD; not a burst afterglow.\n");
if(lbuf[MISC] & 0x08000000)
	fprintf(s,"   The 1st packet (of 3) was missing in the image data stream.\n");
if(lbuf[MISC] & 0x04000000)
	fprintf(s,"   The 2nd packet (of 3) was missing in the image data stream.\n");
if(lbuf[MISC] & 0x02000000)
	fprintf(s,"   The 3rd packet (of 3) was missing in the image data stream.\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x20000000)
	fprintf(s,"   This notice was forced out after watchdog timer expiring -- most likely due to missing packet(s).\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-XRT LightCurve packet */
void pr_swift_xrt_lc(long* lbuf, FILE* s)
{
int   id, segnum;
float delta;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT LightCurve\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Bore_RA=    %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Bore_Dec=   %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Start_TJD=  %d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=  %.2f [sec]\n",lbuf[BURST_SOD]/100.0);
fprintf(s,"   Stop_TJD=   %d\n", lbuf[10]);
fprintf(s,"   Stop_SOD=   %.2f [sec]\n", lbuf[11]/100.0);
delta = lbuf[BURST_SOD]/100.0 - lbuf[11]/100.0;
if(delta < 0.0) delta += 86400.0;  // Handle UT_midnight rollover
fprintf(s,"   Stop_Start= %.2f [sec]\n", delta);
fprintf(s,"   N_Bins=     %d\n", lbuf[20]);
fprintf(s,"   Term_cond=  %d\n", lbuf[21]);
fprintf(s,"            =  0 = Normal (the full 100 bins).\n");
fprintf(s,"            =  1 = Terminated by time (1-99 bins).\n");
fprintf(s,"            =  2 = Terminated by Snapshot (1-99 bins).\n");
fprintf(s,"            =  3 = Terminated by entering the SAA (1-99 bins).\n");
fprintf(s,"   URL=        %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the LC_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the LC_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[21] == 0)
	fprintf(s,"   This Lightcurve was terminated normally.\n");
else if(lbuf[21] == 1)
	fprintf(s,"   This Lightcurve was terminated by the 'Time' condition.\n");
else if(lbuf[21] == 2)
	fprintf(s,"   This Lightcurve was terminated by the 'Snapshot End' condition.\n");
else if(lbuf[21] == 3)
	fprintf(s,"   This Lightcurve was terminated by the 'SAA Entry' condition.\n");
else if(lbuf[21] == 4)
	fprintf(s,"   This Lightcurve accumulation was terminated by the 'LRPD-to-WT Transition' condition.\n");
else if(lbuf[21] == 5)
	fprintf(s,"   This Lightcurve accumulation was terminated by the 'WT-to-LRorPC Transition' condition.\n");
else
	fprintf(s,"   WARNING: This Lightcurve has an unrecognized termination condition.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-UVOT Image packet */
void pr_swift_uvot_image(long* lbuf, FILE* s)
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-UVOT Image\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=  %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=    %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=%d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=%.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=       %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=      %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Roll=     %.4f [deg]\n", lbuf[9]/10000.0);
fprintf(s,"   filter=   %d\n", lbuf[10]);
fprintf(s,"   expo_id=  %d\n", lbuf[11]);
fprintf(s,"   x_offset= %d\n", lbuf[12]);
fprintf(s,"   y_offset= %d\n", lbuf[13]);
fprintf(s,"   width=    %d\n", lbuf[14]);
fprintf(s,"   height=   %d\n", lbuf[15]);
fprintf(s,"   x_grb_pos=%d\n", lbuf[16] & 0xFFFF);
fprintf(s,"   y_grb_pos=%d\n", (lbuf[16] >> 16) & 0xFFFF);
fprintf(s,"   n_frames= %d\n", lbuf[17]);
fprintf(s,"   biining=  %d [index]\n", lbuf[MISC] & 0xF);
fprintf(s,"   swl=      %d\n", lbuf[18] & 0x000F);
fprintf(s,"   lwl=      %d\n", (lbuf[18] >> 16) & 0x000F);
fprintf(s,"   URL=      %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[MISC] & 0x10000000)
	fprintf(s,"   The source of the GRB Position came from the XRT Position command.\n");
else
	fprintf(s,"   The source of the GRB Position came from the Window Position in the Mode command.\n");
if((lbuf[MISC] & 0x0000000F) == 0)
	fprintf(s,"   The image has 1x1 binning (ie no compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 1)
	fprintf(s,"   The image has 2x2 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 2)
	fprintf(s,"   The image has 4x4 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 3)
	fprintf(s,"   The image has 8x8 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 4)
	fprintf(s,"   The image has 16x16 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 5)
	fprintf(s,"   The image has 32x32 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 6)
	fprintf(s,"   The image has 64x64 binning (compression).\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-UVOT SrcList packet */
void pr_swift_uvot_slist(long* lbuf, FILE* s)
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-UVOT SrcList\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=  %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=    %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=%d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=%.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=       %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=      %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Roll=     %7.4f [deg]\n", lbuf[9]/10000.0);
fprintf(s,"   Filter=   %d\n", lbuf[10]);
fprintf(s,"   bkg_mean= %d\n", lbuf[11]);
fprintf(s,"   x_max=    %d\n", lbuf[12]);
fprintf(s,"   y_max=    %d\n", lbuf[13]);
fprintf(s,"   n_star=   %d\n", lbuf[14]);
fprintf(s,"   x_offset= %d\n", lbuf[15]);
fprintf(s,"   y_offset= %d\n", lbuf[16]);
fprintf(s,"   det_thresh=   %d\n", lbuf[17]);
fprintf(s,"   photo_thresh= %d\n", lbuf[18]);
fprintf(s,"   URL=      %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a Swift-UVOT Pos packet */
void pr_swift_uvot_pos(long* lbuf, FILE* s) 
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-UVOT Pos_Ack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=   %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=     %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=     %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=      %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=     %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error=   %.1f [arcsec, radius, statistical only]\n",
											(float)3600.0*lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Mag=     %.2f\n", lbuf[BURST_INTEN]/100.0);  //confirm this!!!!!!!!!!!!!!!
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
else
	fprintf(s,"   ERR: Bit not set -- this UVOT_Pos Notice can only ever be generated on the ground.\n");
if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a burst afterglow -- it is NOT a GRB.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
/* print the contents of a SWIFT POINTDIR packet */
void pr_swift_point(long* lbuf, FILE* s)
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,  Pointing Direction\n",
					lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Next_Point_RA:   %8.4fd (J2000)\n", lbuf[7]/10000.0);
fprintf(s,"   Next_Point_Dec:  %+8.4fd (J2000)\n",lbuf[8]/10000.0);
fprintf(s,"   Next_Point_Roll: %+8.4fd (J2000)\n",lbuf[9]/10000.0);
fprintf(s,"   Slew_Time:       %.2f SOD UT\n", lbuf[6]/100.0);
fprintf(s,"   Slew_Date:       %5d TJD\n", lbuf[5]);
fprintf(s,"   TgtName:         %s\n", (char *)(&lbuf[BURST_URL]));
fprintf(s,"   ObsTime:         %.2f [sec]\n", lbuf[14]/100.0);
fprintf(s,"   Merit:           %.2f\n", lbuf[15]/100.0);
}

/*---------------------------------------------------------------------------*/
/* This technique of trying to "notice" that imalives have not arrived
 * for a long period of time works only some of the time.  It depends on what
 * caused the interruption.  Some forms of broken socket connections hang
 * in the "elseif" shutdown() system call, and so the program never actually
 * gets to the point of calling this routine to check the time and
 * send a message.
 */
void chk_imalive(int which,time_t ctloc){
    static time_t	last_ctloc;		// Timestamp of last imalive received
    static int		imalive_state = 0;	// State of checking engine
    char	        buf[32];		// Tmp place for date string modifications
    time_t		nowtime;		// The Sun machine time this instant
    //char		*ctime();		// Convert the sec-since-epoch to ASCII
    
    switch(imalive_state)
	{
	case 0:
	    // Do something ONLY when we get that first Imalive timestamp.
		if(which)					/* Ctloc is time of most recent imalive */
			{
			last_ctloc = ctloc;
			imalive_state = 1;		/* Go to testing deltas */
			}
		break;
	case 1:
		/* Now that we have an Imalive timestamp, check it against the
		 * current time, and update the timestamp when new ones come in. */
		if(which)					/* Ctloc is time of most recent imalive */
			last_ctloc = ctloc;
		else if((ctloc - last_ctloc) > 600)
			{
			time(&nowtime);			/* Get system time for mesg timestamp */
			strcpy(buf,ctime((time_t *)&nowtime));
			buf[24] = '\0';			/* Overwrite the newline with a null */
			printf(    "ERR: %s ESDT: No Imalive packets for >600sec.\n",buf);
			fprintf(lg,"ERR: %s ESDT: No Imalive packets for >600sec.\n",buf);
			e_mail_alert(1);
			imalive_state = 2;
			}
		break;
	case 2:
		/* Waiting for a resumption of imalives so we can go back to testing
		 * and e-mailing if they should stop for a 2nd (n-th) time.         */
		if(which)					/* Ctloc is time of the resumed imalive */
			{
			last_ctloc = ctloc;
			time(&nowtime);			/* Get system time for mesg timestamp */
			strcpy(buf,ctime((time_t *)&nowtime));
			buf[24] = '\0';			/* Overwrite the newline with a null */
			printf(    "OK: %s ESDT: Imalive packets have resumed.\n",buf);
			fprintf(lg,"OK: %s ESDT: Imalive packets have resumed.\n",buf);
			e_mail_alert(0);
			imalive_state = 1;		/* Go back to testing */
			}
		break;
	default:
		time(&nowtime);				/* Get system time for mesg timestamp */
		strcpy(buf,ctime((time_t *)&nowtime));
		buf[24] = '\0';				/* Overwrite the newline with a null */
		printf(    "ERR: %s ESDT: Bad imalive_state(=%d)\n",imalive_state,buf);
		fprintf(lg,"ERR: %s ESDT: Bad imalive_state(=%d)\n",imalive_state,buf);
		imalive_state = 0;			/* Reset to the initial state */
		break;
	}
}

/*---------------------------------------------------------------------------*/
/* send an E_MAIL ALERT if no pkts in 600sec */
void e_mail_alert(int which)
{
int		rtn;				/* Return value from the system() call */
FILE	*etmp;				/* The temp email scratch file stream pointer */
time_t	nowtime;			/* The Sun machine time this instant */
//char	*ctime();			/* Convert the sec-since-epoch to ASCII */
char	buf[32];			/* Tmp place for date string modifications */
static char	cmd_str[256];	/* Place to build the system() cmd string */

time(&nowtime);				/* Get the system time for the notice timestamp */
strcpy(buf,ctime((time_t *)&nowtime));
buf[24] = '\0';				/* Overwrite the newline with a null */

if((etmp=fopen("e_me_tmp", "w")) == NULL)
	{
	printf(    "ERR: %s ESDT: Can't open scratch e_mail_alert file.\n",buf);
	fprintf(lg,"ERR: %s ESDT: Can't open scratch e_mail_alert file.\n",buf);
	return;
	}

if(which)
	fprintf(etmp,"TITLE:        socket_demo GCN NO PACKETS ALERT\n");
else
	fprintf(etmp,"TITLE:        socket_demo GCN PACKETS RESUMED\n");
fprintf(etmp,"NOTICE_DATE:  %s ESDT\n", buf);

if(fclose(etmp))			/* Close the email file so the filesys is updated */
	{
	printf(    "ERR: %s ESDT: Problem closing scratch email_me file.\n",buf);
	fprintf(lg,"ERR: %s ESDT: Problem closing scratch email_me file.\n",buf);
	}

cmd_str[0] = 0;				/* Clear string buffer before building new string */
if(which)
	strcat(cmd_str,"mail -s BACO_NO_PKT_ALERT ");
else
	strcat(cmd_str,"mail -s BACO_PKT_RESUMED ");
// NOTICE TO GCN SOCKET SITES:  Please don't forget to change this
// e-mail address to your address(es).
strcat(cmd_str,"scott@lheamail");
strcat(cmd_str," < ");
strcat(cmd_str,"e_me_tmp");
strcat(cmd_str," &");
printf(    "%s\n",cmd_str);
fprintf(lg,"%s\n",cmd_str);
rtn = system(cmd_str);
printf(    "%sSystem() rtn=%d.\n",rtn?"ERR: ":"",rtn);
fprintf(lg,"%sSystem() rtn=%d.\n",rtn?"ERR: ":"",rtn);
}

/*---------------------------------------------------------------------------*/
int hete_same(long* hbuf,int find_which)
{
double	cra[4], cdec[4];	/* Generic 4 corners locations */
int		same = 0;			/* Are the 4 corners the same? */

switch(find_which)
{
case FIND_SXC:
	cra[0]  = (double)hbuf[H_SXC_CNR1_RA]/10000.0;	/* All 4 in J2000 */
	cra[1]  = (double)hbuf[H_SXC_CNR2_RA]/10000.0;
	cra[2]  = (double)hbuf[H_SXC_CNR3_RA]/10000.0;
	cra[3]  = (double)hbuf[H_SXC_CNR4_RA]/10000.0;
	cdec[0] = (double)hbuf[H_SXC_CNR1_DEC]/10000.0;	/* All 4 in J2000 */
	cdec[1] = (double)hbuf[H_SXC_CNR2_DEC]/10000.0;
	cdec[2] = (double)hbuf[H_SXC_CNR3_DEC]/10000.0;
	cdec[3] = (double)hbuf[H_SXC_CNR4_DEC]/10000.0;
	break;

case FIND_WXM:
	cra[0]  = (double)hbuf[H_WXM_CNR1_RA]/10000.0;	/* All 4 in J2000 */
	cra[1]  = (double)hbuf[H_WXM_CNR2_RA]/10000.0;
	cra[2]  = (double)hbuf[H_WXM_CNR3_RA]/10000.0;
	cra[3]  = (double)hbuf[H_WXM_CNR4_RA]/10000.0;
	cdec[0] = (double)hbuf[H_WXM_CNR1_DEC]/10000.0;	/* All 4 in J2000 */
	cdec[1] = (double)hbuf[H_WXM_CNR2_DEC]/10000.0;
	cdec[2] = (double)hbuf[H_WXM_CNR3_DEC]/10000.0;
	cdec[3] = (double)hbuf[H_WXM_CNR4_DEC]/10000.0;
	break;

default:
	printf(    "ERR: hete_same(): Shouldn't be able to get here; find_which=%d\n",
																find_which);
	fprintf(lg,"ERR: hete_same(): Shouldn't be able to get here; find_which=%d\n",
																find_which);
	return(0);
	break;
}
if( (cra[0] == cra[1]) && (cra[0] == cra[2]) && (cra[0] == cra[3]) &&
    (cdec[0] == cdec[1]) && (cdec[0] == cdec[2]) && (cdec[0] == cdec[3]))
	return(1);
else
	return(0);
}
