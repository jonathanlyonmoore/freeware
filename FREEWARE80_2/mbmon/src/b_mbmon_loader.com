 $! $! MBMON_LOADER build procedure  $!H $! This software is COPYRIGHT (c) 2005, Ian Miller. ALL RIGHTS RESERVED. $!; $! Released under licence described in freeware+readme.txt   $!# $ arch_type = F$GETSYI("ARCH_TYPE")  $ IF arch_type .NE. 2  $ THENM $     WRITE SYS$OUTPUT "Unsupported system architecture - no build performed" 
 $     EXIT $ ENDIF  $ IF P1 .EQS. "DEBUG"  $ THEN $     CC_FLAGS = "/DEBUG/NOOPT"  $     LL_FLAGS = "/DEBUG"  $ ELSE $     CC_FLAGS = ""  $     LL_FLAGS = ""  $ ENDIF A $ CC'CC_FLAGS' MBMON_LOADER.C + SYS$LIBRARY:SYS$LIB_C.TLB/LIBRARY > $ LINK/SYSEXE'LL_FLAGS' MBMON_LOADER.OBJ, MBMON_LOADER.OPT/OPT $ EXIT