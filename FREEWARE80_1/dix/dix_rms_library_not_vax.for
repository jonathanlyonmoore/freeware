.  	subroutine dix_rms_init_nam(fab,name,resnam) 	implicit none c  c Init nal block, on Alpha/ipf c  	include '($fabdef)' 	record /fabdef/ fab 	character*(*) name  	character*(*) resnam  c  	include '($namdef)' c  	record /namldef/ naml c  	naml.naml$b_bid = naml$c_bid  	naml.naml$b_bln = naml$c_bln  c . 	naml.naml$l_long_expand_alloc  =  len(resnam). 	naml.naml$l_long_expand        = %loc(resnam), 	naml.naml$l_long_filename_size =  len(name), 	naml.naml$l_long_filename      = %loc(name) c  	fab.fab$l_nam = %loc(naml)  c 1 c Set fns/fna to 0/-1, to force use of naml block  c  	fab.fab$l_fna = -1  	fab.fab$b_fns = 0 	return  	end) 	subroutine dix_rms_get_nam(naml,des,fid)  	implicit none c  c Copy the nam device and fidc, c since vax user na and alpha/ia64 used naml1 c  we need a diferent routyine for both platforms  c  	include '($namdef)' 	record /namldef/ naml 	integer*4 des(2)  	integer*2 fid(3)  c  	integer*4 k c *         des(1) = naml.naml$l_long_dev_size%         des(2) = naml.naml$l_long_dev  c          do k=1,3%           fid(k) = naml.naml$w_fid(k)          end do c  	return  	end  