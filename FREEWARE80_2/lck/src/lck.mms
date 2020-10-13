 ! " ! Build script for the LCK utility !  ! � Marc Van Dyck, 01-JUN-1999 ! F ! This script can only be used with MMS /NOACTION /OUTPUT = <filename>& ! The macro AXP or VAX must be defined ! { !==========================================================================================================================    .first .ifdef debug@ 	macro :== macro /lis=lis: /cross /diag /obj=int: /anal /deb=alls 	pascal_pen :== pascal /lis=lis: /cross /show=all /diag /env=int: /noobj /anal /check=all /deb=all /nooptim /nowarn s 	pascal_obj :== pascal /lis=lis: /cross /show=all /diag /obj=int: /noenv /anal /check=all /deb=all /nooptim /nowarn ? 	link_exe :== link /exec=exe: /map=lis: /full /debug /traceback ? 	link_shr :== link /shar=exe: /map=lis: /full /debug /traceback  .else 8 	macro :== macro /lis=lis: /cross /obj=int: /anal /nodebo 	pascal_pen :== pascal /lis=lis: /cross /show=all /nodiag /env=int: /noobj /anal /nocheck /nodeb /optim /nowarn o 	pascal_obj :== pascal /lis=lis: /cross /show=all /nodiag /obj=int: /noenv /anal /nocheck /nodeb /optim /nowarn C 	link_exe :== link /exec=exe: /map=lis: /full /nodebug /notraceback C 	link_shr :== link /shar=exe: /map=lis: /full /nodebug /notraceback  .endif  { !==========================================================================================================================   
 .ifdef axp   kits :					kits_axp   	 	continue    .endif
 .ifdef vax   kits :					kits_vax   	 	continue    .endif  { !==========================================================================================================================   ! kits_axp :				kit:lck_axp011.a, - ( 					kit:mvd-axpvms-lck-v0101--1.pcsi, - 					kit:lck.mem, -  					kit:freeware_readme.txt   	purge /log kit:*.*, lis:*.*  ! kits_vax :				kit:lck_vax011.a, -  					kit:lck.mem, -  					kit:freeware_readme.txt   	purge /log kit:*.*, lis:*.*  { !==========================================================================================================================   1 kit:mvd-axpvms-lck-v0101--1.pcsi :	executables, -  					intermediates, -  					documentation, -  					src:lck$startup.com, -  					src:lck_axp.pcf  . 	purge /log exe:*.*, int:*.*, src:*.*, doc:*.*h 	product package lck /producer = mvd /baselevel = axpvms /source = src:lck_axp.pcf /destination = kit: -G 		/material = (exe:, int:, src:, doc:)  /log /copy /format = sequential   { !--------------------------------------------------------------------------------------------------------------------------   # kit:lck_axp011.a :			executables, -  					intermediates, -  					documentation, -  					src:lck$startup.com, -  					src:kitinstal.com, -  					int:kitdata.com  . 	purge /log exe:*.*, int:*.*, src:*.*, doc:*.*p 	backup /log /verify /interchange exe:lck*.exe, int:lck_*.pen, int:lck_messages.sdl, src:lck_declarations.sdl, -b 		src:lck.hlp, src:lck.cld, doc:lck.lni, src:lck$startup.com, src:kitinstal.com, int:kitdata.com - 		kit:lck_axp011.a /save_set 					 { !--------------------------------------------------------------------------------------------------------------------------   # kit:lck_vax011.a :			executables, -  					intermediates, -  					documentation, -  					src:lck$startup.com, -  					src:kitinstal.com, -  					int:kitdata.com  . 	purge /log exe:*.*, int:*.*, src:*.*, doc:*.*p 	backup /log /verify /interchange exe:lck*.exe, int:lck_*.pen, int:lck_messages.sdl, src:lck_declarations.sdl, -b 		src:lck.hlp, src:lck.cld, doc:lck.lni, src:lck$startup.com, src:kitinstal.com, int:kitdata.com - 		kit:lck_vax011.a /save_set 					 { !--------------------------------------------------------------------------------------------------------------------------    int:kitdata.com :   ! 	open /write data int:kitdata.com a 	write data "$ lck$kit_arch == ""''f$element ( f$getsyi ( "ARCH_TYPE" ) , "|" , "|VAX|AXP|" )'"""  	close data   { !==========================================================================================================================   # executables :				exe:lck_cli.exe, -  					exe:lck_routines.exe, - 					exe:lck_messages.exe, - 					src:lck.cld  	 	continue   { !--------------------------------------------------------------------------------------------------------------------------   . intermediates :				int:lck_declarations.pen, -  					src:lck_declarations.sdl, - 					int:lck_messages.pen, - 					int:lck_messages.sdl   	 	continue   { !--------------------------------------------------------------------------------------------------------------------------   ! documentation :				lib:lck.hlb, -  					doc:lck.lni  	 	continue   { !==========================================================================================================================    lib:lck.hlb :				src:lck.hlp  Q 	if f$search ("lib:lck.hlb") .eqs. "" then library /help /create /log lib:lck.hlb 4 	library /help /replace /log lib:lck.hlb src:lck.hlp  { !==========================================================================================================================   2 kit:freeware_readme.txt :		src:freeware_readme.txt  5 	copy src:freeware_readme.txt kit:freeware_readme.txt   { !--------------------------------------------------------------------------------------------------------------------------    kit:lck.mem :				src:lck.rno, -  					int:lck.rnt  ) 	runoff /output = kit:lck.mem src:lck.rno   { !==========================================================================================================================    doc:lck.lni :				src:lck.rno, -  					int:lck.rnt  8 	runoff /device = ln03 /output = doc:lck.lni src:lck.rno  { !--------------------------------------------------------------------------------------------------------------------------    int:lck.rnt :				int:lck.brn  F 	runoff /contents /deepest_index = 2 /output = int:lck.rnt int:lck.brn  { !--------------------------------------------------------------------------------------------------------------------------    int:lck.brn :				src:lck.rno   	create /nolog int:lck.rnt9 	runoff /nooutput /intermediate = int:lck.brn src:lck.rno  	delete /nolog int:lck.rnt;0  { !==========================================================================================================================   & exe:lck_cli.exe :			int:lck_cli.obj, - 					src:lck_cli.opt, -  					int:lck_msgptrs.obj, -  					exe:lck_routines.exe   J 	link_exe int:lck_cli.obj + int:lck_msgptrs.obj + src:lck_cli.opt /options  { !--------------------------------------------------------------------------------------------------------------------------   
 .ifdef axp  0 exe:lck_routines.exe :			int:lck_routines.obj, -  					src:lck_routines_axp.opt, - 					int:lck_msgptrs.obj  B 	link_shr int:lck_routines.obj + src:lck_routines_axp.opt /options   .endif
 .ifdef vax  0 exe:lck_routines.exe :			int:lck_routines.obj, -  					src:lck_routines_vax.opt, - 					int:lck_xfr_vect.obj, - 					int:lck_msgptrs.obj  B 	link_shr int:lck_routines.obj + src:lck_routines_vax.opt /options   .endif  { !--------------------------------------------------------------------------------------------------------------------------   0 exe:lck_messages.exe :			int:lck_messages.obj, - 					src:lck_messages.opt   > 	link_shr int:lck_messages.obj + src:lck_messages.opt /options  { !==========================================================================================================================   & int:lck_cli.obj	:			src:lck_cli.pas, - 					int:lck_messages.pen, - 					int:lck_declarations.pen    	pascal_obj src:lck_cli.pas   { !--------------------------------------------------------------------------------------------------------------------------   0 int:lck_routines.obj :			src:lck_routines.pas, - 					int:lck_messages.pen, - 					int:lck_declarations.pen     	pascal_obj src:lck_routines.pas  { !--------------------------------------------------------------------------------------------------------------------------   
 .ifdef vax  - int:lck_xfr_vect.obj :			src:lck_xfr_vect.mar    	macro src:lck_xfr_vect.mar    .endif  { !==========================================================================================================================   4 int:lck_declarations.pen :		int:lck_declarations.pas  $ 	pascal_pen int:lck_declarations.pas  { !--------------------------------------------------------------------------------------------------------------------------   
 .ifdef axp  7 int:lck_declarations.pas :		src:lck_declarations.sdl, -  					src:add_ident.tpu  G 	sdl /language=pascal=int:lck_declarations.pas src:lck_declarations.sdl Y 	edit /tpu /nodisplay /command = src:add_ident.tpu /noinitialize int:lck_declarations.pas # 	delete int:lck_declarations.pas;-1    .endif
 .ifdef vax  7 int:lck_declarations.pas :		src:lck_declarations.sdl, - * 					src:change_integer_to_unsigned.tpu, - 					src:add_ident.tpu  G 	sdl /language=pascal=int:lck_declarations.pas src:lck_declarations.sdl Y 	edit /tpu /nodisplay /command = src:add_ident.tpu /noinitialize int:lck_declarations.pas # 	delete int:lck_declarations.pas;-1 j 	edit /tpu /nodisplay /command = src:change_integer_to_unsigned.tpu /noinitialize int:lck_declarations.pas# 	delete int:lck_declarations.pas;-1    .endif  { !==========================================================================================================================   - int:lck_messages.obj :			src:lck_messages.msg   6 	message /nosymbol /object = int: src:lck_messages.msg  { !--------------------------------------------------------------------------------------------------------------------------   , int:lck_msgptrs.obj :			src:lck_messages.msg  U 	message /file_name = lck_messages /object = int:lck_msgptrs.obj src:lck_messages.msg   { !--------------------------------------------------------------------------------------------------------------------------   - int:lck_messages.pen :			int:lck_messages.pas     	pascal_pen int:lck_messages.pas  { !--------------------------------------------------------------------------------------------------------------------------   0 int:lck_messages.pas :			int:lck_messages.sdl, - 					src:add_ident.tpu  > 	sdl /langage=pascal=int:lck_messages.pas int:lck_messages.sdlU 	edit /tpu /nodisplay /command = src:add_ident.tpu /noinitialize int:lck_messages.pas  	delete int:lck_messages.pas;-1   { !--------------------------------------------------------------------------------------------------------------------------   - int:lck_messages.sdl :			src:lck_messages.msg   C 	message /noobject /sdl = int:lck_messages.sdl src:lck_messages.msg   { !==========================================================================================================================    .last 2 	if f$search ("*.ana", 0) .nes. "" then sca load *; 	if f$search ("*.ana", 1) .nes. "" then delete /log *.ana;* ��                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        