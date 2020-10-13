H 	function symbols_init(ptr_symbol_info,ptr_table_info,ptr_terminal_info) 	implicit none c * c We need a list of symbols defined by DCL c and also a list of logicals D c since VMS has no wildcard search for symbols or logicals we build @ c a list of symbols that contain the values for symbols/logicals c  c 1. spawn a subprocess  c 2. catch the output F c 3. Parse the lines and store the results in a linked list of symbols c  	include 'symbol.inc' 7 	integer*4 ptr_symbol_info	!:o: pointer to symbol block 5 	integer*4 ptr_table_info	!:i: info fot table routine : 	integer*4 ptr_terminal_info	!:i: adress of terminal block+ 	logical*4 do_symbols		!:o: symbols present 6 	logical*4 do_logicals           !:o: logicals present- 	integer*4 symbols_init		!:f: function result  c# 	integer*4 istat 	character*80 line 	integer*4 nk,nb1 + 	logical*4 do_sys,do_prc,do_job,do_grp,flag  c & 	integer*4 symbols__process_subprocess 	integer*4 lib$get_vm  	integer*4 lib$create_vm_zone  	logical*4 cli$present 	logical*4 cli$get_value c 8 	record /symbol_info/ symbol_info !:o: symbol info block$ 	pointer (p_symbol_info,symbol_info) c  c Create symbol block  c 6 	istat = lib$get_vm(sizeof(symbol_info),p_symbol_info) 	if(.not. istat) goto 90 c " c Create a VM zone for the symbols c 0 	istat = lib$create_vm_zone(symbol_info.vm_zone) 	if(.not. istat) goto 90 c  c  c Init the pointers  c 6 	symbol_info.ptr_top   = 0	!points to the first symbol9 	symbol_info.ptr_cur   = 0	!points top the current symbol 5 	symbol_info.ptr_last  = 0	!points to the last symbol , 	symbol_info.count     = 0	!count of symbols, 	symbol_info.last_match= 0	!count of symbols 	symbol_info.nb_alloc  = 0 c > 	call terminal_debug(ptr_terminal_info,'Initing symbol table',+      1                      0,dbg_flag_sym)  c  c Process the symbols  c  	do_symbols = .false.   	if(cli$present('symbols')) then 	  do_symbols = .true. 	  nb1 = symbol_info.nb_alloc C 	  istat = symbols__process_subprocess('show symbol *',symbol_info, :      1               ptr_table_info,'S',ptr_terminal_info) 	  if(.not. istat) goto 90 	  nb1 = symbol_info.nb_alloc 0 	  call sys$fao('Nbytes for symbol table = !UL',-      1                  nk,line,%val(nb1))	   4 	  call terminal_debug(ptr_terminal_info,line(1:nk),-      1                        0,dbg_flag_sym)  	endif c  c Process the logicals c  	do_sys = .false.  	do_grp = .false.  	do_job = .false.  	do_prc = .false.  c - 	do while(cli$get_value('logicals',line,nk))   	  if(line(1:2) .eq. 'NO') then  	    flag = .false.  	    line = line(3:) 	  else  	    flag = .true. 	  endif 	  if(line(1:1) .eq. 'A') then 	    do_sys = flag 	    do_grp = flag 	    do_job = flag 	    do_prc = flag" 	  elseif(line(1:1) .eq. 'O') then 	    do_grp = flag 	    do_job = flag 	    do_prc = flag" 	  elseif(line(1:1) .eq. 'S') then 	    do_sys = flag" 	  elseif(line(1:1) .eq. 'G') then 	    do_grp = flag" 	  elseif(line(1:1) .eq. 'J') then 	    do_job = flag" 	  elseif(line(1:1) .eq. 'P') then 	    do_prc = flag 	  endif 	enddo c  	nb1 = symbol_info.nb_alloc  	do_logicals = .false. 	if(do_prc) then@ 	  istat = symbols__process_subprocess('show logical */process',F      1               symbol_info,ptr_table_info,'L',ptr_terminal_info) 	  if(.not. istat) goto 90 	  do_logicals = .true.          endif  	if(do_job) then< 	  istat = symbols__process_subprocess('show logical */job',F      1               symbol_info,ptr_table_info,'L',ptr_terminal_info) 	  if(.not. istat) goto 90 	  do_logicals = .true.  	endif 	if(do_grp) then> 	  istat = symbols__process_subprocess('show logical */group',F      1               symbol_info,ptr_table_info,'L',ptr_terminal_info) 	  if(.not. istat) goto 90 	  do_logicals = .true.  	endif 	if(do_sys) then? 	  istat = symbols__process_subprocess('show logical */system', F      1               symbol_info,ptr_table_info,'L',ptr_terminal_info) 	  if(.not. istat) goto 90 	  do_logicals = .true.  	endif 	if(do_logicals) then  	  nb1 = symbol_info.nb_alloc 1 	  call sys$fao('Nbytes for logical table = !UL', -      1                  nk,line,%val(nb1))	   4 	  call terminal_debug(ptr_terminal_info,line(1:nk),-      1                        0,dbg_flag_sym)  	endif c  90	symbols_init = istat   	ptr_symbol_info = p_symbol_info 	return  	end c 9 	function symbols_exit(ptr_symbol_info,ptr_terminal_info)  	implicit none c > c Free the list of symbols, SInce they are in a special VMzone' c  this is easy, just deelte th vm_zone  c  	include 'symbol.inc'  	integer*4 ptr_symbol_info1 	integer*4 ptr_terminal_info		!:i: terminal block . 	integer*4 symbols_exit		 !:f: function result c# 	integer*4 istat 	integer*4 lib$delete_vm_zone  	integer*4 lib$free_vm c 8 	record /symbol_info/ symbol_info !:o: symbol info block# 	pointer (p_sybol_info,symbol_info)  c  c Delete the zone  c  	p_sybol_info = ptr_symbol_info  c A 	call terminal_debug(ptr_terminal_info,'Deleting symbol table',0, 7      1                                    dbg_flag_sym) 0 	istat = lib$delete_vm_zone(symbol_info.vm_zone) 	if(.not. istat) goto 90 c 5 	symbol_info.ptr_top  = 0	!points to the first symbol 8 	symbol_info.ptr_cur  = 0	!points top the current symbol4 	symbol_info.ptr_last = 0	!points to the last symbol+ 	symbol_info.count    = 0	!count of symbols , 	symbol_info.last_match= 0	!count of symbols c 5 	istat = lib$free_vm(sizeof(symbol_info),symbol_info)  	if(.not. istat) goto 90 	ptr_symbol_info = 0 c  90	symbols_exit = istat  	return  	end7 	function symbols__add(line,symbol_info,ptr_table_info, 5      1                        what,ptr_terminal_info)  	implicit none c 1 c Add a new token (symbol or logical) to the list G c Function called from symbols__process_subprocess with a line from DCL  c This can be a line from   c  a. show symbol   (what = "S")  c  b. show logical  (what = "L") c  	include 'symbol.inc' # 	character*(*) line		 !:i: the line 6 	record /symbol_info/ symbol_info !:io: the admin data7 	integer*4 ptr_table_info		 !:i: info for table routine  	character*1 what		 !:i: L or S 1 	integer*4 ptr_terminal_info		!:i: terminal block . 	integer*4 symbols__add		 !:f: function result c#! 	integer*4 istat,nk,ipos,jpos,nb1  	integer*4 lib$get_vm  	integer*4 table_valid_verb   	integer*4 str$find_first_in_set c  	character*4 type  c  	record /symbol/ wsymbol c  	record /symbol/ symbol  	pointer (p_symbol,symbol) c  c Either from show symb *  c        or   show log * c  c Now parse the line c syntax |  name == "value"  c 
 	istat = 1 	nk = len(line)  c  c Try to find the symbol name  c  	ipos = index(line,'=')  	if(ipos .eq. 0) goto 90 c  c Check for symbol/logical c  	if(what .eq. 'S') then  c  c Symbol : Syntax  c  name = "value" or c  name == "value" c ' 	  wsymbol.symbol_name = line(3:ipos-1) ! 	  wsymbol.nk_name     = ipos - 4  c   c See if the symbol contains a * c 7 	  wsymbol.star_pos    = index(wsymbol.symbol_name,'*') # 	  if(wsymbol.star_pos .ne. 0) then  	    jpos = wsymbol.star_pos 	    wsymbol.symbol_name =  /      1          wsymbol.symbol_name(1:jpos-1)// ;      1          wsymbol.symbol_name(jpos+1:wsymbol.nk_name) * 	    wsymbol.nk_name = wsymbol.nk_name - 1 	  endif c * c The part before the = is the symbol name c See for global symbols c 3 	  if(line(ipos+1:ipos+1) .eq. '=') ipos = ipos + 1  c  	  line = line(ipos+1:)  	  nk = nk - ipos  c  c Now try to find the value  c  	  ipos = index(line,'"')  	  if(ipos .eq. 0) goto 90 c  c We use 3 formats8 c  1. delete :="delete/confirm"   Normal verb definition1 c  2. program:="$programs"        Foreign command / c  3. proced :="@procedure"       Dcl procedure  c > 	  wsymbol.flag         = symbol_flag_verb	!assume normal verb  	  type                 = 'VERB'2           wsymbol.symbol_value = line(ipos+1:nk-1)= 	  wsymbol.nk_value     = nk - ipos - 1		!skip the trailing " : 	  if(wsymbol.nk_value .eq. 0) goto 90	!no value is useles c  c Check for two special cases ' c  symbol:=$name  	!the foreign command . c  symbol:=@procedure   !the command procedure c . 	  if(wsymbol.symbol_value(1:1) .eq. '$') then- 	    wsymbol.flag         = symbol_flag_image " 	    type                 = 'IMAG'2 	  elseif(wsymbol.symbol_value(1:1) .eq. '@') then1 	    wsymbol.flag         = symbol_flag_procedure " 	    type                 = 'PROC' 	  else  c > c Now check if value has a valid verb, get the part before the c  first / or space  c > 	    ipos = str$find_first_in_set(wsymbol.symbol_value,' /')-1, 	    if(ipos .lt. 0) ipos = wsymbol.nk_value c F c See it the first part is a valid DCL verb (Check against DCL tables) c ( 	    if(table_valid_verb(ptr_table_info,5      1            wsymbol.symbol_value(1:ipos))) then  c  c Yes it was a verb  c & 	      wsymbol.flag = symbol_flag_verb$ 	      type                 = 'VERB'	 	    else  c  c Else ohter type of symbol  c ' 	      wsymbol.flag = symbol_flag_other $ 	      type                 = 'OTHE'
 	    endif c  	  endif 	else  c  c Now for logical :syntax  c  "name" [super] = "value", c       = "test" c	  : 	  if(line(1:1) .eq. char(9)) goto 90	!2nd and foll values@ 	  if(line(1:ipos-1) .eq. ' ') goto 90	!2nd and following values c % 	  wsymbol.flag = symbol_flag_logical  	  type = 'LOGI'( 	  if(line(ipos-2:ipos-2) .eq. ']') then c ( c The [super] [exec]  case, skip until " c  	    jpos = ipos-3' 	    do while(line(jpos:jpos) .ne. '"')  	      jpos = jpos - 1 	    end do ) 	    wsymbol.symbol_name = line(4:jpos-1) # 	    wsymbol.nk_name     = jpos - 4  c  	  else ) 	    wsymbol.symbol_name = line(4:ipos-3) # 	    wsymbol.nk_name     = ipos - 6  	  endif c  c 5 c Now we must check if the symbol was not yet present G c  we can use only one versoin, otherwise ambig will always be rerurned  c ! 	  p_symbol = symbol_info.ptr_top  	  do while (p_symbol .ne. 0) 9 	    if(symbol.symbol_name .eq. wsymbol.symbol_name) then E 	      call terminal_debug(ptr_terminal_info,'Skipping '//type//' '// G      1           wsymbol.symbol_name(1:wsymbol.nk_name),0,dbg_flag_sym)  	      goto  80 
 	    endif 	    p_symbol = symbol.ptr_next 	 	  end do  c  c Not double, so insert  c  	  line = line(ipos+2:) + 	  wsymbol.symbol_value = line(ipos+2:nk-1) # 	  wsymbol.nk_value = nk-1 - ipos-2  	  wsymbol.star_pos    = 0 	endif c " c We have a new token in wsymbol, 3 c Now allocate a new symbol, and insert in the list  c D 	nb1 = %loc(wsymbol.symbol_value) - %loc(wsymbol) + wsymbol.nk_value@ 	istat = lib$get_vm(sizeof(symbol),p_symbol,symbol_info.vm_zone) 	if(.not. istat) then  	  call lib$signal(%val(istat)) 
 	  goto 90 	endif2 	symbol_info.nb_alloc = symbol_info.nb_alloc + nb1 c ! c And insert into the linked list  c " 	symbol.nk_name  = wsymbol.nk_name# 	symbol.nk_value = wsymbol.nk_value  	symbol.flag     = wsymbol.flag # 	symbol.star_pos = wsymbol.star_pos < 	symbol.symbol_name = wsymbol.symbol_name(1:wsymbol.nk_name)> 	symbol.symbol_value= wsymbol.symbol_value(1:wsymbol.nk_value) c  c Now link in 6 c The list is a linked list with forward pointers only c  	symbol.ptr_next = 0			!no next ; 	symbol_info.ptr_cur  = symbol_info.ptr_last		!save pointer 5 	symbol_info.ptr_last = p_symbol		!Return the current  c  c Check for new top  c > 	if(symbol_info.ptr_top .eq. 0) symbol_info.ptr_top = p_symbol c = c See if we had already a last one (it was saved in .ptr_cur)  c $ 	if(symbol_info.ptr_cur .ne. 0) then c ' c Yes, Let the previous one point to me  c < 	  p_symbol = symbol_info.ptr_cur	!was the previous last one) 	  symbol.ptr_next = symbol_info.ptr_last  	endif c  c One more token c * 	symbol_info.count = symbol_info.count + 1= 	call terminal_debug(ptr_terminal_info,'Adding '//type//' '// B      1      wsymbol.symbol_name(1:wsymbol.nk_name),0,dbg_flag_sym)   80	istat = 1 90	symbols__add = istat  	return  	end; 	function symbols_rewind(ptr_symbol_info,ptr_terminal_info)  	implicit none c - c Set the pointer to the first symbol/logical  c  	include 'symbol.inc' . 	integer*4 ptr_symbol_info	!:io: symbols block2 	integer*4 ptr_terminal_info			!:i: terminal block. 	integer*4 symbols_rewind		!:i: funtion result c# 	integer*4 istat5 	record /symbol_info/ symbol_info	!:io: symbols block $ 	pointer (p_symbol_info,symbol_info) c   	p_symbol_info = ptr_symbol_info c ? 	call terminal_debug(ptr_terminal_info,'Rewind symbol table',0, )      1                      dbg_flag_sym) 	 	istat =1 * 	symbol_info.ptr_cur = symbol_info.ptr_top 	symbols_rewind = istat  	return  	end> 	function symbols_match(ptr_symbol_info,what,result,nk_result,)      1                         exact,all, 6      1                         ptr_terminal_info,wild) 	implicit none c  c Match all symbols to line : c Start on symbol_info.cur_ptr and see if that one matches6 c  if not try follow the forward link and try the nextG c If we have a match, return the equivalence string and set the pointer # c  to the next one, and return true ) c If we do not have a match, return false  c  c  SO the sequence' c  istat = symbols_rewind(symbol_info)	 4 c  do while (symbols_match(symbol_info,pattern,...))	 c  end do  c ) c Will find all matching symbols/logicals  c : c The exact logical will return true if the match is exact c   so if we have a symbolA c    cop*y = copy/confirm and we ask for cop  exact will be false @ c                          if we ask for copy exact will be true2 c     in both cases symbols_match will return true c A c The parameter all defines the type of symbols/logicals to match ( c  0=match all symbols (and no logicals)B c  1=match only symbols of type verb,procedure,image and not other c  2=match logicals only c  	include 'symbol.inc' 9 	integer*4 ptr_symbol_info		!:i: poitner to symbols block . 	character*(*) what			!:i: the patern to match# 	character*(*) result			!:o: result , 	integer*4 nk_result			!:o: length of result, 	logical*4 exact				!:o: true if exact match, 	logical*4 all				!:i: 0 is list all symbolsT                                                 !    1 is list all not other symbolsK                                                 !    2 is list all logicals & 	logical*4 wild				!:i: wildcard match 	integer*4 ptr_terminal_info/ 	integer*4 symbols_match			!:f: function result  c# 	record /symbol/ wsymbol 	pointer (p_wsymbol,wsymbol) c  	integer*4 istat,nk,first_wild  	integer*4 str$find_first_in_set! 	integer*4 str$case_blind_compare  	integer*4 str$match_wild  	character*4 type  c 5 	record /symbol_info/ symbol_info	!:io: symbols block $ 	pointer (p_symbol_info,symbol_info) c   	p_symbol_info = ptr_symbol_info c  	if(wild) then0 	  first_wild = str$find_first_in_set(what,'*%') 	endif c  	istat = 0		!assume no result & 	exact = .false.		!and not exact match c  c Get the current entry  c " 10	p_wsymbol = symbol_info.ptr_cur8 	if(p_wsymbol .eq. 0) goto 90	!end of list, so not found c K c Set the pointer to the next, so the next check will be for the next entry  c ' 	symbol_info.ptr_cur = wsymbol.ptr_next  c 6 c See about the all parameter, select the wanted types c  	if    (all .eq. 0) then c & c List all symbols (skip the logicals) c 4 	  if(wsymbol.flag .eq. symbol_flag_logical) goto 10 	elseif(all .eq. 1) then c * c Skip all symbols except the "other" type c 4 	  if(wsymbol.flag .eq. symbol_flag_logical) goto 104 	  if(wsymbol.flag .eq. symbol_flag_other  ) goto 10 	else  c  c Skip all symbols c 4 	  if(wsymbol.flag .ne. symbol_flag_logical) goto 10 	endif c , c Now we have a correct type, check the name$ c the * is the end position to match$ c  symbols like cop*y = copy/confirm c ! c If we have a symbol with an *,  # c  the part before the * must match # c  the part after the * is optional @ c First check if what is at least as long as the part upto the * c  	nk = len(what)  	if(wild) then5 	  if(.not. str$match_wild(wsymbol.symbol_name(1:nk), 8      1                              what(1:nk))) goto 10 	else  	  if(str$case_blind_compare( ,      1                           what(1:nk),<      1            wsymbol.symbol_name(1:nk)) .ne. 0) goto 10 	endif c  c Now the match is complete 9 c  if the part upto the * matches, this is an exact match  c  	if(first_wild .gt. 0) then  	  exact = .false. 	else # 	  if(wsymbol.star_pos .gt. 0) then , 	    exact = len(what) .ge. wsymbol.star_pos 	  else + 	    exact = len(what) .eq. wsymbol.nk_name  	  endif 	endif c  	nk_result = wsymbol.nk_name* 	result = wsymbol.symbol_name(1:nk_result) c 9 c We have a match, return symbol, remember the last match  c # 	symbol_info.last_match = p_wsymbol 
 	istat = 1 c  c Print some debug info  c  	type = 'OTHE': 	if(wsymbol.flag .eq. symbol_flag_logical)   type = 'LOGI': 	if(wsymbol.flag .eq. symbol_flag_image)     type = 'IMAG': 	if(wsymbol.flag .eq. symbol_flag_verb)      type = 'VERB': 	if(wsymbol.flag .eq. symbol_flag_procedure) type = 'PROC' c C 	call terminal_debug(ptr_terminal_info,'symbol Match with '//type// D      1        wsymbol.symbol_name(1:wsymbol.nk_name),4,dbg_flag_sym) c  90	symbols_match = istat 	return  	end: 	function symbols__process_subprocess(command,symbol_info,<      1                ptr_table_info,what,ptr_terminal_info) 	implicit none c - c Spawn a subprocess, get the output and call  c	symbols__add for each line c  c  	include 'symbol.inc' 4 	character*(*) command			!:i: the command to execute5 	record /symbol_info/ symbol_info	!:io: control block 7 	integer*4 ptr_table_info			!:i: info fot table library   	character*1 what			!:i: S or L 2 	integer*4 ptr_terminal_info			!:i: terminal block& 	integer*4 symbols__process_subprocess c#) c Spawn a subprocess with command in line ? c and call symbols__add for every returned line from subprocess  c  	include '($dvidef)' 	include '($ssdef)'  	include '($iodef)'  	include '($efndef)' c  	structure /iosb/  	  integer*2 status  	  integer*2 nbyte 	  integer*4 rest  	end structure 	record /iosb/ iosb  c 1 	integer*4 istat,nk_dev,spawn_stat,ef_flag,pid,nk  	character*(64) devnam 	integer*2 chan_mb 	character*255 line G 	character*(symbol_name_max_size + symbol_value_max_size + 10) res_line  c  	integer*4 symbols__add  c  	integer*4 sys$crembx  	integer*4 lib$spawn 	integer*4 sys$qiow  c  c GEt and event flag c  	call lib$get_ef(ef_flag)  c  c Create mailbox c  	spawn_stat = 1 9 	istat = sys$crembx(,chan_mb,%val(sizeof(res_line)),,,,,)  	if(.not. istat) goto 90 c  	if(istat) then  c 2 c Create subprocess with output device the mailbox c and 'line' as command  c  	  nk_dev = 0 7 	  call lib$getdvi(dvi$_devnam,chan_mb,,,devnam,nk_dev) : 	  istat= lib$spawn(command,'NL:',devnam(1:nk_dev),1,,pid,.      1                     spawn_stat,ef_flag)8 	  call sys$fao('spawn !AS , status = !8XL, pid = !8XL',=      1                 nk,line,command,%val(istat),%val(pid)) 4 	  call terminal_debug(ptr_terminal_info,line(1:nk),-      1                        0,dbg_flag_sym)  	  if(.not. istat) goto 80 	end if  c 7 c Keep reading from mailbox until error (should be eof)  c  10	do while(istat)2 	  istat = sys$qiow(%val(EFN$C_ENF),%val(chan_mb),,      1            %val(io$_readvblk),iosb,,,9      1            %ref(res_line),%val(len(res_line)),,,,)   	  if(istat) istat = iosb.status 	  if(istat) then  c ; c If successfull call routine with chan and the result line  c 1 	    istat = symbols__add(res_line(1:iosb.nbyte), H      1                symbol_info,ptr_table_info,what,ptr_terminal_info)	 	  end if  	end do  c 9 	call sys$fao('Result status = !8XL',nk,line,%val(istat)) 2 	call terminal_debug(ptr_terminal_info,line(1:nk),-      1                        0,dbg_flag_sym)  c ? c kill the subprocess, (it should be gone already, but be sure)  c  	call sys$delprc(pid,) c " 	if(istat .eq. ss$_endoffile) then! 	  call sys$waitfr(%val(ef_flag))  	  istat = 1 	end if  c  c Clear up the mailbox c ! 80	call sys$dassgn(%val(chan_mb))  c  c Free the EF, and exit  c  90	call lib$free_ef(ef_flag)$ 	symbols__process_subprocess = istat 	return  	end= 	function symbols_get_value(ptr_symbol_info,result,nk_result)  	implicit none c $ c Return the value of the last matchB c  Should be called after symbols_match, this sets the .last_match c  	include 'symbol.inc' 8 	integer*4 ptr_symbol_info		!:i: pointer to symbol block- 	character*(*) result			!:o: the result value & 	integer*4 nk_result			!:o: the length8 	integer*4 symbols_get_value		!:f: function result (0/1) c  	record /symbol/ symbol  	pointer (p_symbol,symbol) c ! 	record /symbol_info/ symbol_info $ 	pointer (p_symbol_info,symbol_info) c   	p_symbol_info = ptr_symbol_info c  c See if we had a last_match c ' 	if(symbol_info.last_match .ne. 0) then  c ' c Yes we had a last_match, return value  c $ 	  p_symbol = symbol_info.last_match 	  nk_result = symbol.nk_value, 	  result = symbol.symbol_value(1:nk_result) 	  symbols_get_value = 1 	else  c  c Else return failure  c  	  symbols_get_value = 0 	endif 	return  	end