 $ ! Procedure:	LINK_NULSYM.COM
 $ set noon $ say = "write sys$output"$ $ _arch_type = f$getsyi("ARCH_TYPE")@ $!$ _arch_name = f$getsyi("ARCH_NAME") ! "OTHER,VAX,Alpha,IA-64"D $ _arch_name = f$element(_arch_type,",","OTHER,VAX,ALPHA,IPF") - "," $ _vax = (_arch_type .eq. 1) $ _axp = (_arch_type .eq. 2) $ _ipf = (_arch_type .eq. 3)- $ _other = (.not. (_vax .or. _axp .or. _ipf)) ! $ exe_type = ".''_arch_name'_EXE" ! $ obj_type = ".''_arch_name'_OBJ"  $ link = "link/notrace" @ $ if (_axp) then link = "link/notrace/nonative/sysexe=selective"> $ if (_ipf) then link = "link/notrace/native/sysexe=selective" $  $ say "%Linking NULLSYM". $ link/executable='exe_type' NULLSYM'obj_type' $ say "%Done"  $ 	 $ exitt 1 