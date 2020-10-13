 #module RDB_TO_RTF "V2.00"   /*D    Extract table and domain information from an Rdb Database.  Write?    the information in RTF format for input to a word processor.   6    B. Z. Lederman	16-Jun-1992	Adapted from the command 					procedure */   #include STDIO #include DESCRIP #include RMS #include ERRNO #include "DTYPE.H"  = /* #define DEBUG TRUE		/* get printouts while program runs	*/    int field_type, field_length; 9 char relation_name[64], field_name[64], field_source[64];  short int flags;  % unsigned long int status, ret_status;    int num_tab	= 0; int wanted      = FALSE; int source_mode = FALSE; int comment     = FALSE; int index       = FALSE;" char table_name[] = "Dummy Table";  & struct dsc$descriptor_s command_line =)     {0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};   ( $DESCRIPTOR (prompt, "Database Name: ");: $DESCRIPTOR (sqlcmd1, "SQL$ @rdb_fetch_domains_temp.sql");   struct FAB in_fab; struct NAM in_nam;   #define SQL_SUCCESS 0    /* Include the SQLCA							*/        EXEC SQL INCLUDE SQLCA;   & /* Declare the database schema						*/  4     EXEC SQL DECLARE SCHEMA FILENAME 'SQL$DATABASE';  *     EXEC SQL DECLARE TRANSACTION READ ONLY/ 	RESERVING RDB$RELATION_FIELDS FOR SHARED READ, " 		  RDB$RELATIONS FOR SHARED READ, 		  RDB$FIELDS FOR SHARED READ;   #     EXEC SQL DECLARE R_C CURSOR FOR B 	SELECT R.RDB$RELATION_NAME, R.RDB$FIELD_NAME, R.RDB$FIELD_SOURCE,& 		F.RDB$FIELD_TYPE, F.RDB$FIELD_LENGTH) 	FROM RDB$RELATION_FIELDS R, RDB$FIELDS F 0 	WHERE F.RDB$FIELD_NAME = R.RDB$FIELD_SOURCE AND 		F.RDB$SYSTEM_FLAG = 0 E         ORDER BY RDB$FIELD_SOURCE, RDB$RELATION_NAME, RDB$FIELD_NAME;   '     EXEC SQL DECLARE TABLE_C CURSOR FOR ( 	SELECT T.RDB$RELATION_NAME, T.RDB$FLAGS 	FROM RDB$RELATIONS T  	WHERE T.RDB$SYSTEM_FLAG = 0.         ORDER BY RDB$FLAGS, RDB$RELATION_NAME;  (     EXEC SQL DECLARE DOMAIN_C CURSOR FOR 	SELECT D.RDB$FIELD_NAME 	FROM RDB$FIELDS D 	WHERE D.RDB$SYSTEM_FLAG = 0          ORDER BY RDB$FIELD_NAME;  2     EXEC SQL WHENEVER SQLERROR GOTO ERROR_HANDLER;   rdb_to_rtf ()  MAIN_PROGRAM { ;     int i, o, secondary_status, out_file_ptr, rtf_file_ptr; '     char out_buf[256], length_text[32];      short int length; -     char cross_file[] = "RDB_CROSS_TEMP.RTF"; .     char table_file[] = "RDB_TABLES_TEMP.RTF";7     char relation_file[] = "RDB_FETCH_TABLES_TEMP.SQL"; 6     char domain_file[] = "RDB_FETCH_DOMAINS_TEMP.SQL";,     char report_file[] = "RDB_DATABASE.RTF";  -     int status = 1, num_tab = 0, context = 0;   (     struct dsc$descriptor_s found_desc =& 	{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};  &     struct dsc$descriptor_s out_desc =' 	{ 0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};   D     $DESCRIPTOR (faoctl1, "{\\b Database !AS\\par !11<!%D!>}\\par");  .     $DESCRIPTOR (search_desc, "SQL$DATABASE");       in_fab = cc$rms_fab;     in_nam = cc$rms_nam; /*E    $ IF F$MODE () .EQS. "BATCH" THEN SET PROCESS /NAME = "Rdb to RTF"  */   /*?    $ IF F$TYPE (SQL$) .EQS. "" THEN $ SQL$ :== $SYS$SYSTEM:SQL$  */  
 GET_INPUT:  @     ret_status = LIB$GET_FOREIGN (&command_line, &prompt, 0, 0);       if (ret_status == RMS$_EOF)      {      	exit (SS$_NORMAL);      };       if ((ret_status & 1) != 1)     {      	LIB$SIGNAL (ret_status);      };  D /* Change the logical name SQL$DATABASE into a file specification	*/  A     status = LIB$FIND_FILE (&search_desc, &found_desc, &context,    				0, 0, &secondary_status, 0);  G     if (status == RMS$_NMF) exit (status);	/* if no more files, quit */   A     if ((status & 1) != 1) LIB$SIGNAL (status, secondary_status);   J     status = LIB$FIND_FILE_END (&context);  /* finish up the connection	*/ /*C    Parse the filespec to remove the type and replace it with ".RDB"  */     in_fab.fab$l_nam = &in_nam; 2     in_fab.fab$l_fna = command_line.dsc$a_pointer;1     in_fab.fab$b_fns = command_line.dsc$w_length;   D     status = SYS$PARSE (&in_fab);	/* fill in the NAM block fields	*/       if ((status & 1) != 1)     {  	LIB$SIGNAL (status);      };  1 /* if extension is blank, change it to .RDB				*/    /** $ rdb_file = file_dir + file_name + ".RDB" */  :     status = SYS$SEARCH (&in_fab);	/* See if it exists		*/       if ((status & 1) != 1)     { 2 	LIB$SIGNAL (status);		/* should exit with FNF		*/     }; /*( $ DEFINE /NOLOG SQL$DATABASE 'this_file' */( /* Delete old files if they exist					*/ /*2 $ IF F$SEARCH ("rdb_database.rtf") .NES. "" THEN -"   DELETE /NOLOG rdb_database.rtf;*2 $ IF F$SEARCH ("rdb_tables.temp2") .NES. "" THEN -"   DELETE /NOLOG rdb_tables.temp2;*2 $ IF F$SEARCH ("rdb_tables.temp4") .NES. "" THEN -"   DELETE /NOLOG rdb_tables.temp4;*5 $ IF F$SEARCH ("rdb_tables_temp.rtf") .NES. "" THEN - %   DELETE /NOLOG rdb_tables_temp.rtf;* 4 $ IF F$SEARCH ("rdb_cross_temp.rtf") .NES. "" THEN -$   DELETE /NOLOG rdb_cross_temp.rtf;*; $ IF F$SEARCH ("rdb_fetch_tables_temp.sql") .NES. "" THEN - +   DELETE /NOLOG rdb_fetch_tables_temp.sql;* < $ IF F$SEARCH ("rdb_fetch_domains_temp.sql") .NES. "" THEN -,   DELETE /NOLOG rdb_fetch_domains_temp.sql;* */ /* $ IF F$MODE () .EQS. "BATCH" $ THEN' $    SET PROCESS /NAME = "Rdb to RTF 1"  $ ELSEI $    WRITE SYS$OUTPUT "Get list of tables, domains, and cross reference."  $ ENDIF  */  9 /* start the older cross_reference program code here			*/    /*B    Create a report file, and write the header information into it.B    Done here because this way we get a normal 'CR' file instead ofG    DCL's weird VFC file.  It should also be faster, and perhaps someday 5    all the processing will be in this program anyway.  */J     out_file_ptr = creat (report_file, 0, "rfm=var", "rat=cr", "mrs=255");       if (out_file_ptr < 0)      { 8 	printf (" error %d opening %s \n", errno, report_file); 	exit (errno);     };  f     strcpy (out_buf, "{\\rtf1\\mac\\deff2 {\\fonttbl{\\f0\\fswiss Chicago;}{\\f2\\froman New York;}");5     write (out_file_ptr, &out_buf, strlen (out_buf)); ]     strcpy (out_buf, "{\\f3\\fswiss Geneva;}{\\f4\\fmodern Monaco;}{\\f16\\fnil Palatino;}"); 5     write (out_file_ptr, &out_buf, strlen (out_buf)); b     strcpy (out_buf, "{\\f20\\froman Times;}{\\f21\\fswiss Helvetica;}{\\f22\\fmodern Courier;}");5     write (out_file_ptr, &out_buf, strlen (out_buf)); i     strcpy (out_buf, "{\\f23\\ftech Symbol;}}{\\colortbl\\red0\\green0\\blue0;\\red0\\green0\\blue255;"); 5     write (out_file_ptr, &out_buf, strlen (out_buf)); e     strcpy (out_buf, "\\red0\\green255\\blue255;\\red0\\green255\\blue0;\\red255\\green0\\blue255;"); 5     write (out_file_ptr, &out_buf, strlen (out_buf)); h     strcpy (out_buf, "\\red255\\green0\\blue0;\\red255\\green255\\blue0;\\red255\\green255\\blue255;}");5     write (out_file_ptr, &out_buf, strlen (out_buf)); G     strcpy (out_buf, "{\\stylesheet {\\sbasedon222\\snext0 Normal;}}"); 5     write (out_file_ptr, &out_buf, strlen (out_buf)); k     strcpy (out_buf, "\\margl1440\\margr1440\\widowctrl\\ftnbj\\sectd\\linemod0\\linex0\\cols1\\endnhere"); 5     write (out_file_ptr, &out_buf, strlen (out_buf)); 3     strcpy (out_buf, "\\ftnbj\\pard\\plain\\fs20"); 5     write (out_file_ptr, &out_buf, strlen (out_buf)); 5     strcpy (out_buf, "{\\header\\pard\\plain\\fs24"); 5     write (out_file_ptr, &out_buf, strlen (out_buf));   F /* Create the header which contains the database name and date/time	*/  B     status = LIB$SYS_FAO (&faoctl1, 0, &out_desc, &found_desc, 0);  /     if ((status & 1) != 1) LIB$SIGNAL (status);   H     write (out_file_ptr, out_desc.dsc$a_pointer, out_desc.dsc$w_length);  5     strcpy (out_buf, "\\par{\\b\\ul Domains}}\\par"); 5     write (out_file_ptr, &out_buf, strlen (out_buf));   [     strcpy (out_buf, "{\\footer\\pard\\plain\\tqc\\tx4320\\tqr\\tx8640\\tab\\chpgn\\par}"); 5     write (out_file_ptr, &out_buf, strlen (out_buf));   <     strcpy (out_buf, "\\pard\\plain\\tx2800\\tx4600\\fs20");5     write (out_file_ptr, &out_buf, strlen (out_buf));        close (out_file_ptr);   2 /* Get the list of relations to be processed				*/  L     out_file_ptr = creat (relation_file, 0, "rfm=var", "rat=cr", "mrs=255");       if (out_file_ptr < 0)      { : 	printf (" error %d opening %s \n", errno, relation_file); 	exit (errno);     };  I     rtf_file_ptr = creat (table_file, 0, "rfm=var", "rat=cr", "mrs=255");        if (rtf_file_ptr < 0)      { 7 	printf (" error %d opening %s \n", errno, table_file);  	exit (errno);     };  4     strcpy (out_buf, "SET OUTPUT rdb_tables.temp2");5     write (out_file_ptr, &out_buf, strlen (out_buf));   Y     strcpy (out_buf, "\\page\\sectd\\pard\\plain\\fs20\\s244\\tqc\\tx4320\\tqr\\tx8640"); 5     write (rtf_file_ptr, &out_buf, strlen (out_buf));   5     strcpy (out_buf, "{\\header\\pard\\plain\\fs24"); 5     write (rtf_file_ptr, &out_buf, strlen (out_buf));   H     write (rtf_file_ptr, out_desc.dsc$a_pointer, out_desc.dsc$w_length);  4     strcpy (out_buf, "\\par{\\b\\ul Tables}}\\par");5     write (rtf_file_ptr, &out_buf, strlen (out_buf));   d     strcpy (out_buf, "\\trowd \\trgaph80\\trleft-80 \\cellx4600\\cellx9280\\pard \\intbl\\tx2500 ");5     write (rtf_file_ptr, &out_buf, strlen (out_buf));        EXEC SQL OPEN TABLE_C;  (     while (SQLCA.SQLCODE == SQL_SUCCESS)     { 4 	EXEC SQL FETCH TABLE_C INTO :relation_name, :flags;  C 	if (SQLCA.SQLCODE != SQL_SUCCESS) break;    /* no more records  */   / 	for (i = 0;  i < sizeof (relation_name);  i++)  	{! 	    if (relation_name[i] == ' ')  	    { 		relation_name[i] = '\0'; 		break; 	    };  	};   ! 	strcpy (out_buf, "SHOW TABLE "); ! 	strcat (out_buf, relation_name); 2 	write (out_file_ptr, &out_buf, strlen (out_buf));  ! 	strcpy (out_buf, relation_name);  /*/    Append "\tab A view." to the names of views.  */ 	if ((flags & 1) == 1) 	{' 	    strcat (out_buf, "\\tab A view.");  	};    	strcat (out_buf, "\\par"); 2 	write (rtf_file_ptr, &out_buf, strlen (out_buf)); /*E    Count the number of tables, and insert a column break when needed.  */ 	num_tab = num_tab + 1;  	if (num_tab == 45)  	{7 	    strcpy (out_buf, "\\cell \\pard \\intbl\\tx2500"); 6 	    write (rtf_file_ptr, &out_buf, strlen (out_buf)); 	};      };       EXEC SQL CLOSE TABLE_C;        EXEC SQL ROLLBACK;       strcpy (out_buf, "EXIT"); 5     write (out_file_ptr, &out_buf, strlen (out_buf));        close (out_file_ptr);  /*G    If we didn't write anything into the right-hand column we must do so F    now, or else the remainder of the document goes into the right-hand    column and MS Word hangs. */     if (num_tab < 45)      { C 	strcpy (out_buf, "\\cell \\pard \\intbl\\tx2500 \\par { } \\par"); 2 	write (rtf_file_ptr, &out_buf, strlen (out_buf));     };  2     strcpy (out_buf, "\\cell \\pard \\row \\par");5     write (rtf_file_ptr, &out_buf, strlen (out_buf));   A     close (rtf_file_ptr);	/* probably want to hold off on this	*/   0 /* Get the list of domains to be processed				*/  J     out_file_ptr = creat (domain_file, 0, "rfm=var", "rat=cr", "mrs=255");       if (out_file_ptr < 0)      { 8 	printf (" error %d opening %s \n", errno, domain_file); 	exit (errno);     };  4     strcpy (out_buf, "SET OUTPUT rdb_tables.temp4");5     write (out_file_ptr, &out_buf, strlen (out_buf));        EXEC SQL OPEN DOMAIN_C;   (     while (SQLCA.SQLCODE == SQL_SUCCESS)     { * 	EXEC SQL FETCH DOMAIN_C INTO :field_name;  C 	if (SQLCA.SQLCODE != SQL_SUCCESS) break;    /* no more records  */   , 	for (i = 0;  i < sizeof (field_name);  i++) 	{ 	    if (field_name[i] == ' ') 	    { 		field_name[i] = '\0';  		break; 	    };  	};   " 	strcpy (out_buf, "SHOW DOMAIN "); 	strcat (out_buf, field_name);2 	write (out_file_ptr, &out_buf, strlen (out_buf));     };       EXEC SQL CLOSE DOMAIN_C;       EXEC SQL ROLLBACK;       strcpy (out_buf, "EXIT"); 5     write (out_file_ptr, &out_buf, strlen (out_buf));        close (out_file_ptr);   7 /* Output cross reference of domains in RTF format			*/   I     out_file_ptr = creat (cross_file, 0, "rfm=var", "rat=cr", "mrs=255");        if (out_file_ptr < 0)      { 7 	printf (" error %d opening %s \n", errno, cross_file);  	exit (errno);     };  <     strcpy (out_buf, "\\page\\sectd\\pard\\tx3000\\tx7300");5     write (out_file_ptr, &out_buf, strlen (out_buf));   5     strcpy (out_buf, "{\\header\\pard\\plain\\fs24"); 5     write (out_file_ptr, &out_buf, strlen (out_buf));   H     write (out_file_ptr, out_desc.dsc$a_pointer, out_desc.dsc$w_length);H     strcpy (out_buf, "\\par{\\b\\ul Cross Reference of Domains}}\\par");5     write (out_file_ptr, &out_buf, strlen (out_buf));        EXEC SQL OPEN R_C;  (     while (SQLCA.SQLCODE == SQL_SUCCESS)     { 5 	EXEC SQL FETCH R_C INTO :relation_name, :field_name, * 		:field_source, field_type, field_length;  C 	if (SQLCA.SQLCODE != SQL_SUCCESS) break;    /* no more records  */    	out_buf[0] = '\0';   . 	for (i = 0;  i < sizeof (field_source);  i++) 	{  	    if (field_source[i] == ' ') 	    { 		field_source[i] = '\0';  		break; 	    };  	};   9 	for (i = 0;  i < sizeof (field_name);  i++)	/* 'trim'	*/  	{						/* trailing	*/2 	    if (field_name[i] == ' ')			/* blanks from	*/ 	    {						/* the end of	*/) 		field_name[i] = '\0';			/* the field	*/  		break; 	    };  	};   / 	for (i = 0;  i < sizeof (relation_name);  i++)  	{! 	    if (relation_name[i] == ' ')  	    { 		relation_name[i] = '\0'; 		break; 	    };  	};   4 	strcpy (out_buf, "\\par ");		/* build up the RTF	*/6 	strcat (out_buf, field_source);		/* formatted line	*/ 	strcat (out_buf, "\\tab ");! 	strcat (out_buf, relation_name);  	strcat (out_buf, ".");  	strcat (out_buf, field_name); 	strcat (out_buf, "\\tab ");% 	strcat (out_buf, dtype[field_type]); + 	if ((field_type == 37) ||		/* VARCHAR(		*/ ' 	    (field_type == 14))			/* CHAR(		*/  	{/ 	    sprintf (length_text, "%d", field_length); # 	    strcat (out_buf, length_text);  	    strcat (out_buf, ")");  	};   2 	write (out_file_ptr, &out_buf, strlen (out_buf));     };       EXEC SQL CLOSE R_C;        EXEC SQL ROLLBACK;       strcpy (out_buf, "\\par}"); 5     write (out_file_ptr, &out_buf, strlen (out_buf));        close (out_file_ptr);    */ /* $ IF F$MODE () .EQS. "BATCH" $ THEN' $    SET PROCESS /NAME = "Rdb to RTF 2"  $ ELSE9 $    WRITE SYS$OUTPUT "Get domain descriptions from SQL."  $ ENDIF  */  /* spawn the SQL command						*/  0     status = LIB$SPAWN (&sqlcmd1, 0, 0, 0, 0, 0, 			&ret_status, 0, 0, 0, 0, 0);  /* $!A $! Check exit status to see if there was a database access error?  $! */ /* $ IF F$MODE () .EQS. "BATCH" $ THEN' $    SET PROCESS /NAME = "Rdb to RTF 3"  $ ELSE3 $    WRITE SYS$OUTPUT "Append domain descriptions."  $ ENDIF  */  7 $ OPEN /APPEND /ERROR = nofile outfile rdb_database.rtf 7 $ OPEN /READ   /ERROR = nofile tmpfile rdb_tables.temp4  $! $ readloop3: $!' $ READ /END = endfile3 tmpfile instring  $!% $ col1   = F$EXTRACT (0, 1, instring)  $!/ $ IF col1 .EQS. "	"			! if line starts with tab " $ THEN						! should be multi-line $!						! comment, compress it6 $    WRITE outfile F$EDIT (instring, "COMPRESS, TRIM") $    GOTO readloop3  $ ENDIF  $!9 $ tmpstring = F$EDIT (instring, "COMPRESS, TRIM, UPCASE")  $!( $ first  = F$ELEMENT (0, " ", tmpstring)( $ second = F$ELEMENT (1, " ", tmpstring) $!I $ IF first .EQS. "SHOW" .AND. second .EQS. "DOMAIN" THEN $ GOTO readloop3  $!, $ IF first .EQS. "EXIT" THEN $ GOTO endfile3 $!B $! The line with the field name must be formatted differently thanD $! the comment and missing value lines. None of the field name lines@ $! I've seen have a colon (":") in them, so that's what I'll use! $! to tell one line from another.  $!C $ IF F$LOCATE (":", tmpstring) .EQ. F$LENGTH (tmpstring)	! no colon  $ THEN							! must be field* $    field = F$ELEMENT (0, " ", tmpstring) $    rest = tmpstring - field  $    WRITE outfile "\par\par" ; $    WRITE outfile "{\b " + field + "}\tab" + rest + "\par"  $!!    WRITE SYS$OUTPUT field  $    GOTO readloop3  $ ENDIF  $!I $! Should probably parse here for tabs and replace with spaces or "\tab", > $! though hard tab characters are acceptable in RTF.  May have/ $! to account for multi-line comments, however.  $!# $!! WRITE outfile instring + "\par" 9 $ WRITE outfile instring		! try no \par at end of comment  $! $ GOTO readloop3 $! $ endfile3:  $! $ CLOSE tmpfile  $ CLOSE outfile  $! $ IF F$MODE () .EQS. "BATCH" $ THEN' $    SET PROCESS /NAME = "Rdb to RTF 4"l $ ELSE. $    WRITE SYS$OUTPUT "Append list of tables." $ ENDIFo $!4 $ APPEND /NOLOG rdb_tables_temp.rtf rdb_database.rtf $! $ IF F$MODE () .EQS. "BATCH" $ THEN' $    SET PROCESS /NAME = "Rdb to RTF 5"m $ ELSE8 $    WRITE SYS$OUTPUT "Get table descriptions from SQL." $ ENDIFu $! $ DEFINE SYS$OUTPUT NL:u $!! $ SQL$ @rdb_fetch_tables_temp.sql/ $! $ DEASSIGN SYS$OUTPUTa $! $ IF F$MODE () .EQS. "BATCH" $ THEN' $    SET PROCESS /NAME = "Rdb to RTF 6"] $ ELSE3 $    WRITE SYS$OUTPUT "Process table descriptions."t $ ENDIF_ $!7 $ OPEN /APPEND /ERROR = nofile outfile rdb_database.rtfo7 $ OPEN /READ   /ERROR = nofile tmpfile rdb_tables.temp2  $! $ readloop4: $!' $ READ /END = endfile4 tmpfile instringd $!G $ IF F$LENGTH (instring) .LE. 2 THEN GOTO readloop4	! skip if too shortS $!/ $ IF instring .EQS. "EXIT" THEN $ GOTO endfile4l $!& $ select = F$EXTRACT (0, 10, instring)& $ col1   = F$EXTRACT ( 0, 1, instring) $! $ IF select .EQS. "Table cons" $ THEN $    wanted      = FALSE $    source_mode = FALSE $    GOTO readloop4s $ ENDIF	 $!: $ IF select .EQS. "Comment on"		! start capturing comments $ THEN $    comment = TRUEC $    GOTO readloop4R $ ENDIFR $! $ IF comment $ THEN; $    IF select .EQS. "Columns fo"	! stop capturing commetnsR	 $    THEND $	comment = FALSED $	WRITE outfile "\par" $	GOTO readloop4
 $    ENDIF% $    WRITE outfile "\par " + instring	 $    GOTO readloop4D $ ENDIFE $!C $ IF select .EQS. "Indexes on"		! start capturing index information  $ THEN $    index = TRUE	0 $    WRITE outfile "\par\par{\b\ul Indexes}\par" $    GOTO readloop4_ $ ENDIF$ $!
 $ IF index $ THEN@ $    IF select .EQS. "Storage Ma" .OR. select .EQS. "No indexes"	 $    THENF4 $	index = FALSE			! stop capturing index information $	GOTO readloop4
 $    ENDIF $!4 $    tmpstring = F$EDIT (instring, "COMPRESS, TRIM") $!. $    IF col1 .NES. "	"			! first line in index	 $    THENS( $	domain = F$ELEMENT (0, " ", tmpstring)  $	tmpstring = tmpstring - domain3 $	tmpstring = "{\b " + domain + "}\tab" + tmpstringR$ $    ELSE					! other lines in group! $	tmpstring = "\tab " + tmpstring 
 $    ENDIF $!% $    WRITE outfile "\par" + tmpstringg $    GOTO readloop4f $ ENDIFR $! $ IF select .EQS. "SHOW TABLE" $ THEN. $    table_name = F$ELEMENT (2, " ", instring) $    wanted      = FALSE $    source_mode = FALSE< $!!    WRITE SYS$OUTPUT table_name		! so we can see progress2 $    WRITE outfile "\page\par\pard \tx2800\tx4600" $!< $! May consider putting the database file name back in here. $!< $    WRITE outfile "{\b Columns for " + table_name + "}\par" $    GOTO readloop4  $ ENDIFP $! $ IF select .EQS. "----------" $ THEN $    wanted = TRUE_ $!    WRITE outfile "\par{\ul Column Name}{\ul \tab }{\ul Data Type}{\ul \tab}{\ul Domain}\par"=G $    WRITE outfile "\par{\ul Column Name\tab Data Type\tab Domain}\par"H $    GOTO readloop4A $ ENDIF  $!C $ IF .NOT. wanted THEN $ GOTO readloop4		! not ready for output yetY $!B $! Have to extract field names from everything else.  Fields startD $! in column 1 and have a blank space (or should have) in columns 31
 $! and 32. $!% $ col31 = F$EXTRACT (30, 1, instring)  $!) $ IF col1 .NES. " " .AND. col31 .EQS. " ") $ THEN> $    tmpstring = F$EDIT (F$ELEMENT (0, "	", instring), "TRIM")E $    domain = F$EDIT (F$ELEMENT (1, "	", instring), "COLLAPSE, TRIM")o] $    IF domain .EQS. "" THEN domain = F$EDIT (F$ELEMENT (2, "	", instring), "COLLAPSE, TRIM")n+ $    column = F$ELEMENT (0, " ", tmpstring))' $    length = F$LENGTH (tmpstring) - 32u/ $    data_t = F$EXTRACT (32, length, tmpstring); $    WRITE outfile "\par"oI $    WRITE outfile "{\b " + column + "}\tab " + data_t + "\tab " + domainB $    GOTO readloop4b $ ENDIF& $!5 $! Still have to look for the start of a source line.; $!# $ select = F$EXTRACT (0, 8, select)$ $ IF select .EQS. " Source:" $ THEN( $    WRITE outfile "\par\par { Source:}" $!% $! Strip the <CR><LF> out of the lineL $!M $    WRITE outfile "\par" + F$EXTRACT (10, F$LENGTH(instring) - 10, instring)  $    source_mode = TRUEi $    GOTO readloop4" $ ENDIF  $!C $! Have to look for Missing Values, which are not part of comments.uA $! Start them on their own line. Also look for View Source lines.F $!J $ IF select .EQS. " Missing" .OR. select .EQS. " Comment" .OR. source_mode $ THEN$ $    WRITE outfile "\par" + instring $    GOTO readloop4E $ ENDIFN $!1 $! Parse tabs out of comments and similar fields." $!0 $ instring = F$EDIT (instring, "COMPRESS, TRIM") $ WRITE outfile instring $ GOTO readloop4 $! $ endfile4:  $! $ CLOSE tmpfileO $ CLOSE outfile4 $! $ IF F$MODE () .EQS. "BATCH" $ THEN' $    SET PROCESS /NAME = "Rdb to RTF 7"l $ ELSEE $    WRITE SYS$OUTPUT "Append cross-reference of domains to columns."/ $ ENDIF_ $!3 $ APPEND /NOLOG rdb_cross_temp.rtf rdb_database.rtfq $!
 $ exitout: $!
 $ SET NOON $!4 $ IF P2 .NES. ""			! if an output file was specified $ THEN( $    RENAME /NOLOG rdb_database.rtf 'P2' $ ENDIF_ $!" $ DELETE /NOLOG rdb_tables.temp2;*" $ DELETE /NOLOG rdb_tables.temp4;*% $ DELETE /NOLOG rdb_tables_temp.rtf;* $ $ DELETE /NOLOG rdb_cross_temp.rtf;*+ $ DELETE /NOLOG rdb_fetch_tables_temp.sql;* , $ DELETE /NOLOG rdb_fetch_domains_temp.sql;* $!	 $ nofile:  $! $ EXIT $STATUS       exit (1);f   ERROR_HANDLER:  7     status = SYS$PUTMSG (&RDB$MESSAGE_VECTOR, 0, 0, 0);        SQL$SIGNAL (); } 