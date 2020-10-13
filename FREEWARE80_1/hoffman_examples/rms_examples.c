   /* ** COPYRIGHT (c) 1992 1999 BY 9 ** DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASSACHUSETTS.  ** ALL RIGHTS RESERVED.  **H ** THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED AND COPIEDH ** ONLY  IN  ACCORDANCE  OF  THE  TERMS  OF  SUCH  LICENSE  AND WITH THEH ** INCLUSION OF THE ABOVE COPYRIGHT NOTICE. THIS SOFTWARE OR  ANY  OTHERH ** COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANYH ** OTHER PERSON.  NO TITLE TO AND  OWNERSHIP OF THE  SOFTWARE IS  HEREBY ** TRANSFERRED.  **H ** THE INFORMATION IN THIS SOFTWARE IS  SUBJECT TO CHANGE WITHOUT NOTICEH ** AND  SHOULD  NOT  BE  CONSTRUED  AS A COMMITMENT BY DIGITAL EQUIPMENT ** CORPORATION.  **H ** DIGITAL ASSUMES NO RESPONSIBILITY FOR THE USE  OR  RELIABILITY OF ITS: ** SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DIGITAL. */   /* **++ **  Facility:  ** **	Examples  ** **  Version: V1.1  ** **  Abstract:  **+ **	Example of working with RMS calls from C  ** **  Author: " **	Steve Hoffman (XDELTA::HOFFMAN) ** **  Creation Date:  1-Jan-1990 ** **  Modification History: , **	    Hoffman	15-Oct-1994	Updates for DEC C, **	    Hoffman	05-Aug-1994	Added XABCDT demo **-- */   /* //  RMS_EXAMPLES.C //H //  Program displays some RMS calls made from the c language.  Types outA //  the calling process's SYS$LOGIN:LOGIN.COM file to SYS$OUTPUT.  //D //  Included is a main and three subroutines.  The subroutines open,B //  read a record, and close the file.  Several hooks, such as theE //  use of the NAM block to obtain the specification of the file that A //  was actually opened, are included but are not currently used.  // //  To build:  //0 //    $ CC [/DECC] [/DEBUG/NOOPTIM] RMS_EXAMPLES" //    $ LINK [/DEBUG] RMS_EXAMPLES% //    $ RUN [/[NO]DEBUG] RMS_EXAMPLES  */   #include <descrip.h> #include <lib$routines.h>  #include <rms.h> #include <starlet.h> #include <string.h>  #include <ssdef.h> #include <stdio.h> #include <stsdef.h>    /*@ // RMS_MRS is the maximum record size that can be read (and thus // displayed) by this program. */ #define RMS_MRS	255    /*< // The following is the core data structure for the program.< // The various RMS subroutines all communicate via a pointer // referencing this struct.  */ struct RmsFileContext      {      struct FAB fab;      struct RAB rab;      struct NAM nam;      struct XABDAT xabdat;      char rss[NAM$C_MAXRSS];      short max_rec_siz;     char *data_buffer;     };  ; RmsShowDate( void **CtxArg, char *Subject, void *TimeQuad )      {      int RetStat;$     struct dsc$descriptor AsciiDate;     int AsciiDateLen = 23;)     struct RmsFileContext *Ctx = *CtxArg;   *     AsciiDate.dsc$b_class = DSC$K_CLASS_D;*     AsciiDate.dsc$b_dtype = DSC$K_DTYPE_T;     AsciiDate.dsc$w_length = 0; #     AsciiDate.dsc$a_pointer = NULL; 8     RetStat = lib$sget1_dd( &AsciiDateLen, &AsciiDate );*     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;8     RetStat = sys$asctim ( 0, &AsciiDate, TimeQuad, 0 );*     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;     printf("%s\n", Subject ); +     RetStat = lib$put_output( &AsciiDate ); *     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;     printf("\n" );*     RetStat = lib$sfree1_dd( &AsciiDate );*     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;     return SS$_NORMAL;     }    RmsFileOpen( void **CtxArg, 7 char *FileName, char *DefFileName, int flags, int rss )      {      int RetStat;     struct RmsFileContext *Ctx; 1     int howbig = sizeof( struct RmsFileContext );      char *CDT = "CDT";       /*.     // acquire some space for a Context block.     */-     RetStat = lib$get_vm( &howbig, &Ctx, 0 );   *     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;       *CtxArg = (void *) Ctx;        /*7     // Fill in the various fields of the Context block. ?     // -- Builds the File Access Block (FAB), the Record Access =     // Block (RAB) and the Name (NAM) Block.  Along with some .     // other miscellaneous housekeeping stuff.     */     Ctx->fab = cc$rms_fab;     Ctx->rab = cc$rms_rab;     Ctx->nam = cc$rms_nam;      Ctx->xabdat = cc$rms_xabdat;  #     Ctx->fab.fab$l_nam = &Ctx->nam; #     Ctx->fab.fab$l_fop = FAB$M_NAM; #     Ctx->fab.fab$b_fac = FAB$M_GET;   "     Ctx->fab.fab$l_fna = FileName;,     Ctx->fab.fab$b_fns = strlen( FileName );%     Ctx->fab.fab$l_dna = DefFileName; /     Ctx->fab.fab$b_dns = strlen( DefFileName );   #     Ctx->rab.rab$l_fab = &Ctx->fab; &     Ctx->fab.fab$l_xab = &Ctx->xabdat;  &     Ctx->nam.nam$b_rss = NAM$C_MAXRSS;"     Ctx->nam.nam$l_rsa = Ctx->rss;  #     Ctx->rab.rab$b_rac = RAB$C_SEQ;        /*"     // Attempt to open the file...     */*     RetStat = sys$open( &Ctx->fab, 0, 0 );*     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;       /*!     //  Show the creation date...      */A     RetStat = RmsShowDate( CtxArg, CDT, &Ctx->xabdat.xab$q_cdt ); *     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;       /*=     // Allocate a buffer large enough for the biggest record.      */;     RetStat = lib$get_vm( &RMS_MRS, &Ctx->data_buffer, 0 ); *     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;       /*:     // Attempt to connect the record stream to the file...     */-     RetStat = sys$connect( &Ctx->rab, 0, 0 ); *     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;       return RetStat;      }   8 RmsFileRead( void **CtxArg, char **BufAdr, int *BufLen )     {      int RetStat;)     struct RmsFileContext *Ctx = *CtxArg;   *     Ctx->rab.rab$l_ubf = Ctx->data_buffer;!     Ctx->rab.rab$w_usz = RMS_MRS;   )     RetStat = sys$get( &Ctx->rab, 0, 0 );   *     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	{         *BufLen = (char) 0;  	*BufAdr = (char) 0; 	return RetStat; 	}  !     *BufAdr = Ctx->rab.rab$l_rbf; !     *BufLen = Ctx->rab.rab$w_rsz;        return RetStat;      }    RmsFileClose( void **CtxArg )      {      int RetStat;)     struct RmsFileContext *Ctx = *CtxArg;        /*#     // Free up the record buffer...      */<     RetStat = lib$free_vm( &RMS_MRS, &Ctx->data_buffer, 0 );*     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;       /*0     // Be nice and clean up the record stream...     */0     RetStat = sys$disconnect( &Ctx->rab, 0, 0 );*     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;       /*     // And close the file...     */+     RetStat = sys$close( &Ctx->fab, 0, 0 ); *     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;       /**     // And free up the allocated memory...     */I     RetStat = lib$free_vm( &sizeof( struct RmsFileContext ), CtxArg, 0 ); *     if ( !$VMS_STATUS_SUCCESS( RetStat ) ) 	return RetStat;       return RetStat;        }  main()     {      int RetStat;     void *Context;     char *BufAdr;      int BufLen;        /*H     // Open the file.  Minimal checking is performed.  Read access only.     */G     RetStat = RmsFileOpen( &Context, "LOGIN", "SYS$LOGIN:.COM", 0, 0 );        /*5     // Read the file.  Minimal checking is performed.      */     for (;;) 	{5 	RetStat = RmsFileRead( &Context, &BufAdr, &BufLen ); & 	if ( $VMS_STATUS_SUCCESS( RetStat ) )0 	    printf("%*.*s\n", BufLen, BufLen, BufAdr ); 	else  	    break;  	}       /*     // Close up shop.      */'     RetStat = RmsFileClose( &Context );        return RetStat;      }   ��                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        