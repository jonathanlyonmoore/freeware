B [environment('PQM_OBJ:GLOBALDEF'), inherit('SYS$LIBRARY:STARLET')] Module GLOBALDEF; P {*******************************************************************************  (   	GLOBALDEF		Global definitions for PQM  J   This module contains global definitions used by other modules in the PQM
   package.    ?   	Created 7-Nov-2000 by J.Begg, VSM Software Services Pty Ltd. C   	Copyright � 2000 VSM Software Development.  All rights reserved.   P *******************************************************************************}   CONST " 	%include 'PQM_VERSION.TXT/NOLIST'   	MESSAGE_SILENT		= FALSE;    	DESCRIPTION_SIZE	= 255; 	DEVICE_NAME_SIZE	= 31;   	FILE_NAME_SIZE		= NAM$C_MAXRSS; 	FORM_NAME_SIZE		= 31; 	JOB_NAME_SIZE		= 39;  	NODE_NAME_SIZE		= 6;  	QUEUE_NAME_SIZE		= 31;  	USERNAME_SIZE		= 12;    	{D 	  Change these two constants to change the rendition of headings in? 	  the Detail displays at the top of the Queue and Job screens.  	} 	DETAIL_HEADING_RENDITION	= 0; 	DETAIL_HEADING_COMPLEMENT	= 0;    	{E 	  Change these two constants to change the rendition of value fields B 	  in the Detail displays at the top of the Queue and Job screens. 	}& 	DETAIL_VALUE_RENDITION		= SMG$M_BOLD; 	DETAIL_VALUE_COMPLEMENT		= 0;     TYPEA 	VMS_datetime	= [quad] packed record lo:unsigned; hi:integer end;   ' 	IO_Status_Block	= [quad] packed record  			      sts	: [long] integer;# 			      reserved	: [long] integer; 	 			  end;    	Item_List_3	= packed record$ 			      item_len	: [word] 0..65535;% 			      item_code	: [word] 0..65535; " 			      bufaddr	: [long] integer;" 			      retaddr	: [long] integer;	 			  end;   /         Que_Status_Text	= varying [80] of char; ( 	Job_Status_Text	= varying [80] of char;   	browse_list	= record F 			      headings		: unsigned;	{ Virtual display for column headings }4 			      display		: unsigned;	{ Virtual display id }4 			      rows, cols	: integer;	{ Size of 'display' }= 			      current_row	: integer;	{ Currently highlighted row } P 			      viewport_start	: integer;	{ Row in 'display' at which viewport starts }A 			      viewport_rows	: integer;	{ No. of rows in the viewport } B 			      invalidated	: boolean;	{ Force re-placement of viewport }	 			  end;    	queue_info_ptr	= ^queue_info; 	job_info_ptr	= ^job_info;   	file_info	= record 2 			      name		: varying [FILE_NAME_SIZE] of char;$ 			      status		: FILE_STATUS$TYPE; 			      start_page	: integer; 			      finish_page	: integer; 	 			  end; ? 	file_info_arr(count:integer)		= array [1..count] of file_info;   	file_info_ptr	= ^file_info_arr;   	job_info	= record# 			      prev, next	: job_info_ptr; ! 			      queue		: queue_info_ptr; 1 			      name		: varying [JOB_NAME_SIZE] of char; 5 			      username		: varying [USERNAME_SIZE] of char; 2 			      form		: varying [FORM_NAME_SIZE] of char;  			      entry_number	: integer; 			      size		: integer;  			      blocks_done	: integer;  			      priority		: integer;  			      file_count	: integer;  			      files		: file_info_ptr;$ 			      submit_time	: VMS_datetime;' 			      completed_time	: VMS_datetime; % 			      retain_until	: VMS_datetime; ! 			      flags		: JOB_FLAGS$TYPE; # 			      status		: JOB_STATUS$TYPE; 2 			      pending_reason	: PENDING_JOB_REASON$TYPE;5 			      condition		: packed array [1..3] of integer; * 			      decoded_status	: Job_Status_Text; 			      display_row	: integer;  			      selected		: boolean; 	 			  end;    	queue_info	= record% 			      prev, next	: queue_info_ptr; 3 			      name		: varying [QUEUE_NAME_SIZE] of char; : 			      description	: varying [DESCRIPTION_SIZE] of char;2 			      form		: varying [FORM_NAME_SIZE] of char;- 			      node		: packed array [1..6] of char; 6 			      device		: varying [DEVICE_NAME_SIZE] of char;# 			      flags		: QUEUE_FLAGS$TYPE; % 			      status		: QUEUE_STATUS$TYPE;  			      display_row	: integer;   			      holding_jobs	: integer;  			      pending_jobs	: integer;! 			      retained_jobs	: integer;  			      timed_jobs	: integer;& 			      job_status	: JOB_STATUS$TYPE;/ 			      job_pending	: PENDING_JOB_REASON$TYPE; 9 			      job_first_file	: varying [NAM$C_MAXRSS] of char; 6 			      job_form		: varying [FORM_NAME_SIZE] of char;& 			      job_entry_number	: unsigned;	* 			      decoded_status	: Que_Status_Text; 			      selected		: boolean; 	 			  end;   ' 	SMG_Menu(count,width:integer)	= record   					      display   : unsigned;' 					      selection : [word] 0..65535; K 					      choices   : array [1..count] of packed array [1..width] of char;  					  end;  	SMG_Menu_ptr			= ^SMG_Menu;  P {-------------------------------------------------------------------------------  *	Global variables						       *P -------------------------------------------------------------------------------}   VAR % 	pasteboard_id	: [volatile] unsigned;  	pasteboard_rows	: integer;  	pasteboard_cols	: integer;  	keyboard_id	: unsigned; 	time_display	: unsigned; ! 	read_timeout	: integer value 15; ( 	form_selector	: SMG_Menu_Ptr value NIL;! 	exit_flag	: boolean value FALSE;   P {-------------------------------------------------------------------------------+ *	Routines declared in PQM.PAS					       * P -------------------------------------------------------------------------------}  ) Procedure UPDATE_TIME_DISPLAY;  external; # Procedure ERASE_MESSAGE;  external; a Function CONFIRM (prompt : [readonly] packed array [l0..h0:integer] of char) : boolean; external; L Procedure MESSAGE (str   : [readonly] packed array [l0..h0:integer] of char;) 		   alert : boolean := TRUE);  external; 1 Procedure SYS_MESSAGE (sts : integer);  external; * Procedure APPEND_FIELD (var dest : string;? 			src      : [readonly] packed array [l0..h0:integer] of char;  			fieldlen : integer;- 			padding  : [truncate] integer);  external; . Procedure PREPARE_MENU (var menu   : SMG_Menu; 			target_row : integer;J 			flags      : integer := SMG$M_FIXED_FORMAT+SMG$M_WRAP_MENU);  external;O Procedure DISPLAY_DCL_COMMAND (command : packed array [l0..h0:integer] of char; 6 			       paste_row, display_rows : integer);  extern;R Procedure DISPLAY_FILE (filespec : packed array [l0..h0:integer] of char); extern;    P {-------------------------------------------------------------------------------. *	Routines declared in QUEUES.PAS					       *P -------------------------------------------------------------------------------}  ; Procedure BUILD_FORM_SELECTOR (var forms    : SMG_Menu_Ptr; ! 			       target_row   : integer; U 			       initial_form : [truncate] packed array [l0..h0:integer] of char); external; B Procedure BUILD_QUEUE_SELECTOR (var queue_selector : SMG_Menu_Ptr;! 				target_row         : integer; U 				exclude_queue      : [truncate] packed array [l0..h0:integer] of char); external; 1 Procedure MANAGE_QUEUES (initial_queue		: string;  			 start_with_jobs	: boolean;. 			 ignore_server_queues	: boolean); external;G Function UPDATE_ONE_QUEUE (var queue : queue_info) : integer; external;   P {-------------------------------------------------------------------------------, *	Routines declared in JOBS.PAS					       *P -------------------------------------------------------------------------------}  : Procedure MANAGE_JOBS (queue : queue_info_ptr);  external;E Procedure DECODE_PENDING_JOB_REASON (var str : varying [len] of char; 7 				     p       : PENDING_JOB_REASON$TYPE);  external;   P {-------------------------------------------------------------------------------. *	Routines declared in BROWSER.PAS				       *P -------------------------------------------------------------------------------}  $ Function BROWSE_CREATE (r : integer; 			v : integer; - 			h : packed array [l0..h0:integer] of char;  			var b : browse_list 			) : integer;  external;7 Procedure BROWSE_DOWN (var b : browse_list);  external; W Procedure BROWSE_ERASE (var b : browse_list; new_rows : [truncate] integer);  external; U Procedure BROWSE_SELECT_ROW (var b : browse_list; selected_row : integer);  external; 5 Procedure BROWSE_UP (var b : browse_list);  external;    END.