 $ ! Procedure:	BUILD_PATCH.COM
 $ set noon $ say = "write sys$output"$ $ _arch_type = f$getsyi("ARCH_TYPE")@ $!$ _arch_name = f$getsyi("ARCH_NAME") ! "OTHER,VAX,Alpha,IA-64"D $ _arch_name = f$element(_arch_type,",","OTHER,VAX,ALPHA,IPF") - "," $ _vax = (_arch_type .eq. 1) $ _axp = (_arch_type .eq. 2) $ _ipf = (_arch_type .eq. 3)- $ _other = (.not. (_vax .or. _axp .or. _ipf)) ! $ exe_type = ".''_arch_name'_EXE" ! $ obj_type = ".''_arch_name'_OBJ"  $ link = "link/notrace" @ $ if (_axp) then link = "link/notrace/nonative/sysexe=selective"@ $ if (_ipf) then link = "link/notrace/nonative/sysexe=selective" $  $ say "%Building PATCH"  $ say """ $ say "  ... compiling PATCH_DISK"% $ macro/object=PATCH_DISK'obj_type' - , 	SYS$LIBRARY:ARCH_DEFS+SYS$DISK:[]PATCH_DISK  $ say "  ... linking PATCH_DISK"1 $ link/executable='exe_type' PATCH_DISK'obj_type'  $ say """ $ say "  ... compiling PATCH_FILE"% $ macro/object=PATCH_FILE'obj_type' - , 	SYS$LIBRARY:ARCH_DEFS+SYS$DISK:[]PATCH_FILE  $ say "  ... linking PATCH_FILE"1 $ link/executable='exe_type' PATCH_FILE'obj_type'  $ say "" $ say "%Done"  $ 
 $bail_out:	 $ exitt 1 