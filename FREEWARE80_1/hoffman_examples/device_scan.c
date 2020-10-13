 #module	EXAMPLE "SRH X1.0-000" #pragma builtins   /* ** COPYRIGHT (c) 1992 BY9 ** DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASSACHUSETTS.  ** ALL RIGHTS RESERVED.  **H ** THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED AND COPIEDH ** ONLY  IN  ACCORDANCE  OF  THE  TERMS  OF  SUCH  LICENSE  AND WITH THEH ** INCLUSION OF THE ABOVE COPYRIGHT NOTICE. THIS SOFTWARE OR  ANY  OTHERH ** COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANYH ** OTHER PERSON.  NO TITLE TO AND  OWNERSHIP OF THE  SOFTWARE IS  HEREBY ** TRANSFERRED.  **H ** THE INFORMATION IN THIS SOFTWARE IS  SUBJECT TO CHANGE WITHOUT NOTICEH ** AND  SHOULD  NOT  BE  CONSTRUED  AS A COMMITMENT BY DIGITAL EQUIPMENT ** CORPORATION.  **H ** DIGITAL ASSUMES NO RESPONSIBILITY FOR THE USE  OR  RELIABILITY OF ITS: ** SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DIGITAL. */   /* **++ **  Facility:  ** **	Examples  ** **  Version: V1.0  ** **  Abstract:  **C **	Performs a $DEVICE_SCAN call, and other related system services. E **      (This is old VAX C code, and has not been updated to ANSI C.)  ** **  Author:  **	Steve Hoffman ** **  Creation Date:  1-Jan-1990 ** **  Modification History:  **-- */ /* **  To build this: ** **    $!# **    $ CC DEVICE_SCAN/NODEBUG/OPTI . **    $ LINK DEVICE_SCAN/NODEBUG,SYS$INPUT/OPT **    SYS$SHARE:VAXCRTL/SHARE  **    $!I **    $ If F$Search("DEVICE_SCAN.*;-1") .nes. "" Then Purge DEVICE_SCAN.* M **    $ If F$Search("DEVICE_SCAN.OBJ") .nes. "" Then Delete DEVICE_SCAN.OBJ;*  **    $! **    $ Exit **    $!" **    $ CC DEVICE_SCAN/DEBUG/NOOPT, **    $ LINK DEVICE_SCAN/DEBUG,SYS$INPUT/OPT **    SYS$SHARE:VAXCRTL/SHARE  **    $!I **    $ If F$Search("DEVICE_SCAN.*;-1") .nes. "" Then Purge DEVICE_SCAN.* M **    $ If F$Search("DEVICE_SCAN.OBJ") .nes. "" Then Delete DEVICE_SCAN.OBJ;*  **    $! */ /* **  DEVICE_SCAN.C  **G **  A test program for the VMS V5.2 $DEVICE_SCAN system service.  Scans F **  through the system looking for any *VX%0: devices.  If found, thisG **  routine checks for an owner PID and for whether or not the DECvoice  **  ACP is mounted.  */ #include    dvidef #include    ssdef  #include    stsdef #include    descrip    #ifndef SS$_NOMOREDEV  #define SS$_NOMOREDEV	2648 #endif   #define DEVNAMLEN   16 #define FAOBUFLEN   80   main()     { %     struct dsc$descriptor retdevnam = @ 	{ DEVNAMLEN, DSC$K_DTYPE_T, DSC$K_CLASS_S, malloc(DEVNAMLEN) };#     struct dsc$descriptor fao_dsc = @ 	{ FAOBUFLEN, DSC$K_DTYPE_T, DSC$K_CLASS_S, malloc(FAOBUFLEN) }; 	 *     unsigned long int cntxt[2] = { 0, 0 };%     unsigned short int retdevlen = 0; &     unsigned short int devbuflen = 16;+     unsigned long int finstat = SS$_NORMAL;      unsigned long int pid;     unsigned long int mnt;     unsigned long int retstat;
     struct 	{ 	unsigned short int buflen;  	unsigned short int itmcod;  	unsigned char *bufadr;  	unsigned char *bufrla; 
 	} dvi[] = 	    { 	    4, DVI$_PID, &pid, 0, 	    4, DVI$_MNT, &mnt, 0, 	    0, 0, 0, 0, 	    };   K     $DESCRIPTOR( mynameis, "DEVICE_SCAN demo program.  Requires VMS V5.2"); &     $DESCRIPTOR( decvoice, "*VX%0:" );L     $DESCRIPTOR( faoctrl, "DECvoice !AS, owned by PID !8XL, !AS mounted." );      $DESCRIPTOR( yesmnt, "is" );#     $DESCRIPTOR( nomnt, "is not" );   H     /* The following loop executes at least once.  It attempts to	    */K     /* locate all DECvoice devices configured on the current system.	    */ I     /* It then displays the name of each, and whether or not it is	    */ 0     /* currently allocated to a user.					    */     *     retstat = lib$put_output( &mynameis );     do 	{+         retdevnam.dsc$w_length = DEVNAMLEN; )         fao_dsc.dsc$w_length = FAOBUFLEN; G         retstat = sys$device_scan( &retdevnam, &retdevnam.dsc$w_length,  	    &decvoice, 0, cntxt );  	switch ( retstat )  	    { 	    case SS$_NORMAL: * 		retstat = sys$getdviw( 0, 0, &retdevnam, 		    dvi, 0, 0, 0, 0 );+ 		if ( !( $VMS_STATUS_SUCCESS( retstat )) )  		    return( retstat ); 		retstat = sys$fao( &faoctrl, 		    &fao_dsc.dsc$w_length,  		    &fao_dsc, &retdevnam, pid,! 		    (mnt ? &yesmnt : &nomnt )); + 		if ( !( $VMS_STATUS_SUCCESS( retstat )) )  		    return( retstat );' 		retstat = lib$put_output( &fao_dsc ); + 		if ( !( $VMS_STATUS_SUCCESS( retstat )) )  		    return( retstat ); 	        break;  	    case SS$_NOMOREDEV: 		break; 	    default:  		finstat = retstat; 		break; 	    } 	},     while ( $VMS_STATUS_SUCCESS( retstat ));     $     free( retdevnam.dsc$a_pointer );     return( finstat );          } 