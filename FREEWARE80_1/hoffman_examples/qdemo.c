 #pragma  module	qdemo	"V2.0" #pragma builtins   /*- ** Copyright 2001 Compaq Computer Corporation  ** */   /* **++ **  FACILITY:  Examples  ** **  MODULE DESCRIPTION:  **J **      This routine contains a demonstration of the OpenVMS self-relativeD **	interlocked RTL queue routines lib$remqhi() and lib$insqti(), andH **      the equivilent Compaq C compiler builtin functions, and providesK **      a demonstration of the OpenVMS Compaq C memory management routines.  ** **  AUTHORS: ** **      Stephen Hoffman  ** **  CREATION DATE:  21-Jan-1990  ** **  DESIGN ISSUES: **
 **      NA ** **  MODIFICATION HISTORY:  ** **      9-Aug-2001  Hoffman : **                  Compaq C updates, added builtin calls. ** **-- */   /*$ **  $! queue demo build procedure... **  $ cc/decc/debug/noopt qdemo  **  $ link qdemo/debug **  $! */   /* ** **  INCLUDE FILES  ** */   #include <builtins.h>  #include <lib$routines.h>  #include <libdef.h>  #include <ssdef.h> #include <stdio.h> #include <stdlib.h>  #include <stsdef.h>    main()     {      unsigned long int retstat;     unsigned long int i;     struct queueblock  	{ 	unsigned long int *flink; 	unsigned long int *blink; 	unsigned long int dd; 	} *qb;      /*.     **	Allocate the (zeroed) queue header now.     **B     **	The interlocked queue forward and backward links located inE     **	the queue header (of self-relative queues) must be initialized F     **	to zero prior to usage.  calloc() performs this for us.  BlocksE     **	allocated and inserted in the queue subsequently need not have      **  their links zeroed.      **D     **	NB: On VMS, the calloc() and malloc() routines acquire memoryD     **	that is quadword (or better) aligned.  The VAX hardware queueC     **	instructions (and thus the queue routines) require a minimum      **	of quadword alignment.      */G     struct queueblock *header = calloc(1, sizeof( struct queueblock ));       struct queueblock *qtmp = 0;  1     printf( "qdemo.c -- queue demomstration\n" ); #     printf( "\nRTL calls...\n\n" );        /*E     **  dynamically allocate the memory for each block, place a value E     **  in the block and insert the block onto the tail of the queue.      */     for ( i = 0; i < 10; i++ ) 	{. 	qtmp = calloc(1,sizeof( struct queueblock )); 	qtmp->dd = i;, 	printf( "inserting item: %d\n", qtmp->dd );& 	retstat = lib$insqti( qtmp, header );   	};        /*4     **	Remove queue entries until there are no more.     */     retstat = SS$_NORMAL; ,     while ( $VMS_STATUS_SUCCESS( retstat ) ) 	{' 	retstat = lib$remqhi( header, &qtmp ); & 	if ( $VMS_STATUS_SUCCESS( retstat ) ) 	    {/ 	    printf( "removing item: %d\n", qtmp->dd );  	    free( qtmp ); 	    } 	}  $     if ( retstat != LIB$_QUEWASEMP )6 	printf( "unexpected status %x received\n", retstat );     else3 	printf( "expected completion status received\n" );   '     printf( "\nbuiltin calls...\n\n" );        /*E     **  dynamically allocate the memory for each block, place a value E     **  in the block and insert the block onto the tail of the queue.      */     for ( i = 0; i < 10; i++ ) 	{. 	qtmp = calloc(1,sizeof( struct queueblock )); 	qtmp->dd = i;, 	printf( "inserting item: %d\n", qtmp->dd );# 	retstat = _INSQTI( qtmp, header );  	};          /*4     **	Remove queue entries until there are no more.     */"     retstat = _remqi_removed_more;0     while (( retstat == _remqi_removed_more ) ||( 	   ( retstat == _remqi_removed_empty )) 	{$ 	retstat = _REMQHI( header, &qtmp );* 	if (( retstat == _remqi_removed_more ) ||( 	   ( retstat == _remqi_removed_empty )) 	    {/ 	    printf( "removing item: %d\n", qtmp->dd );  	    free( qtmp ); 	    } 	}       switch ( retstat )       {         case _remqi_removed_empty:? 	printf( "unexpected status _remqi_removed_empty received\n" );          break;       case _remqi_removed_more: > 	printf( "unexpected status _remqi_removed_more received\n" );         break;       case _remqi_not_removed:= 	printf( "unexpected status _remqi_not_removed received\n" );          break;       case _remqi_empty:5 	printf( "expected status _remqi_empty received\n" );          break;       }        printf( "\nDone...\n" );     return SS$_NORMAL;       } ��                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        