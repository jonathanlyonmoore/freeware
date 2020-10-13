 [  IDENT ( 'LCK V1.1' ) ,  
 INHERIT ( '     'sys$library:pascal$cli_routines' , '     'sys$library:pascal$lib_routines' ,      'sys$library:starlet' ,      'int:lck_declarations' ,     'int:lck_messages' )   ]  PROGRAM lck_cli ( OUTPUT ) ;    { � Marc Van Dyck, 15-MAR-2000 }     TYPE  $     logging_modes   = ( on , off ) ;(     action	    = ( request , release ) ;   VAR        {+} *     { Local stuff - types declared locally     {-}        action_code		    : action ; ,     action_text		    : VARYING [7] OF CHAR ;     status		    : UNSIGNED ;"     logging		    : logging_modes ;       {+} J     { Passed to the LCK API - types inherited from the LCK API environment     {-}   0     resource_name	    : lck_resource_name_type ;2     resource_width	    : lck_resource_width_type ;.     request_mode	    : lck_request_mode_type ;     PROCEDURE parse_request ;   (     BEGIN { of procedure parse_request }       {+}      { Get lock name      {-}   S 	status := CLI$GET_VALUE ( 'RESNAM' , resource_name.BODY , resource_name.LENGTH ) ; c         IF status <> SS$_NORMAL THEN LIB$SIGNAL ( LCK_LCKNOTGTD , 1 , %STDESCR ' ' , status , 0 ) ;        {+}      { System or not ?      {-}   ,         status := CLI$PRESENT ( 'SYSTEM' ) ;         CASE status OFQ 	    CLI$_PRESENT , CLI$_LOCPRES :		resource_width := lck_resource_width_system ; \ 	    CLI$_ABSENT , CLI$_NEGATED , CLI$_LOCNEG :	resource_width := lck_resource_width_group ;Y 	    OTHERWISE					LIB$SIGNAL ( LCK_LCKNOTGTD , 1 , %STDESCR resource_name , status , 0 )          END ;        {+}      { Wait or not ?      {-}   *         status := CLI$PRESENT ( 'WAIT' ) ;         CASE status OF\ 	    CLI$_PRESENT , CLI$_LOCPRES	, CLI$_DEFAULTED :  request_mode := lck_request_mode_wait ;[ 	    CLI$_ABSENT , CLI$_NEGATED , CLI$_LOCNEG :	    request_mode := lck_request_mode_test ; ] 	    OTHERWISE					    LIB$SIGNAL ( LCK_LCKNOTGTD , 1 , %STDESCR resource_name , status , 0 )          END ;        {+}      { Logging      {-}   " 	status := CLI$PRESENT ( 'LOG' ) ; 	CASE status OF 4 	    CLI$_PRESENT , CLI$_LOCPRES :		logging := on  ;? 	    CLI$_ABSENT, CLI$_NEGATED , CLI$_LOCNEG :	logging := off ; U 	    OTHERWISE LIB$SIGNAL ( LCK_LCKNOTGTD , 1 , %STDESCR resource_name , status , 0 )  	END ;       {+} ;     { We know everything we need - let's request the lock }      {-}   5 	status := lck_request ( name	    := resource_name ,  ! 				width	    := resource_width,    				mode	    := request_mode ) ;       {+}      { Logging and signalling       {-}    	CASE status OF   b 	    LCK_LCKGTD :    IF logging = on THEN LIB$SIGNAL ( LCK_LCKGTD , 1 , %STDESCR resource_name ) ;  - 	    LCK_LCKINUSE :  BEGIN { of CASE branch }   ? 		    LIB$SIGNAL ( LCK_LCKINUSE, 1 , %STDESCR resource_name ) ; 0 		    $EXIT ( UOR ( SS$_CANCEL + 2 , 2 ** 28 ) )P 		    { SS$_CANCEL is Warning, we want Error => + 2 ; bit 28 suppresses output }   	    END ; { of CASE branch }   Y 	    OTHERWISE	    LIB$SIGNAL ( LCK_LCKNOTGTD , 1 , %STDESCR resource_name , status , 0 )    	END { of CASE statement }  (     END { of procedure parse_request } ;     PROCEDURE parse_release ;   (     BEGIN { of procedure parse_release }       {+}      { Get resource name      {-}   Z         status := CLI$GET_VALUE ( 'RESNAM' , resource_name.BODY , resource_name.LENGTH ) ;c         IF status <> SS$_NORMAL THEN LIB$SIGNAL ( LCK_LCKNOTREL , 1 , %STDESCR ' ' , status , 0 ) ;        {+}      { Logging      {-}   " 	status := CLI$PRESENT ( 'LOG' ) ; 	CASE status OF 3 	    CLI$_PRESENT , CLI$_LOCPRES :		logging := on ; @ 	    CLI$_ABSENT , CLI$_NEGATED , CLI$_LOCNEG :	logging := off ;U 	    OTHERWISE LIB$SIGNAL ( LCK_LCKNOTREL , 1 , %STDESCR resource_name , status , 0 )  	END ;       {+} 6     { We know everyhing we need - release the lock now     {-}   2 	status := lck_release (	name := resource_name ) ;       {+}      { Logging and signalling     {-}    	IF status <> LCK_LCKRELN 	    THEN LIB$SIGNAL ( LCK_LCKNOTREL, 1 , %STDESCR resource_name, status , 0 )\             ELSE IF logging = on THEN LIB$SIGNAL ( LCK_LCKREL , 1 , %STDESCR resource_name )  (     END { of procedure parse_release } ;     BEGIN { of main program }        {+}      { Initialize     {-}        REWRITE ( output ) ;       {+} 0     { Get the DCL syntax that has been activated     {-}   R     status := CLI$GET_VALUE ( 'ACTION' , action_text.BODY , action_text.LENGTH ) ;L     IF status <> SS$_NORMAL THEN LIB$SIGNAL ( LCK_LCKINTERR , 0 , status ) ;;     READV ( action_text, action_code, ERROR := CONTINUE ) ; ;     IF STATUSV <> 0 THEN LIB$SIGNAL ( LCK_LCKINTERR ) ;            {+} )     { Dispatch to syntax-specific section      {-}        CASE action_code OF        request : parse_request ;      release : parse_release        END { of case statement }    END { of main program } . ��                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          