 $!% $! Compile and LINK LIST_LISTEN_TABLE  $!% $ IF P1 .EQS. "LINK" THEN GOTO DOLINK  $ IF P1 .EQS. "DEBUG"  $ THEN $     cc_flags = "/NOOPT/DEBUG"  $     lnk_flags = "/DEBUG" $ ELSE $     cc_flags = ""  $     lnk_flags = "" $ ENDIF ' $ CC'cc_flags' LISTEN_TABLE.C	/PREF=ALL " $ CC'cc_flags' LIST_LISTEN_TABLE.C $ CC'cc_flags' LISTEN_SUBS.C $ CC'cc_flags' FORMAT_SYSID.C  $dolink:$ $ arch_type = F$GETSYI("ARCH_TYPE") / $ IF (arch_type .EQ. 2) .OR. (arch_type .EQ. 3)  $ THEN] $     LINK'lnk_flags' LIST_LISTEN_TABLE.OBJ,FORMAT_SYSID.OBJ,LISTEN_TABLE.OBJ,LISTEN_SUBS.OBJ  $ ENDIF  $ IF arch_type .EQ. 1  $ THEN[ $     LINK'lnk_flags' LIST_LISTEN_TABLE.OBJ,FORMAT_SYSID.OBJ,LISTEN_TABLE,LISTEN_SUBS.OBJ,-  	SYS$INPUT/OPT SYS$SHARE:VAXCRTL.EXE/SHARE  $ ENDIF  $ EXIT