J 1       !%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%3         ! Program: INT_DEMO_B_IO_BASELINE_BASIC.BAS /         ! System : INTOUCH DEMONSTRATION SYSTEM %         ! Author : Daniel James Swain          ! Date   : 11-JAN-1991L         ! Purpose: Establish performance baseline for opening/closing a fileJ         !%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  0         map (vendor_map) string whole_rec$ = 2007         map (vendor_map) string name$ = 30, sex$ = 1, & A                 bdate$ = 6, street$ = 30, city$ = 12, zip$ = 9, & G                 ethnic$ = 1, parent$ = 24, phone$ = 10, bphone$ = 10, & *                 ephone$ = 10, balance$ = 8    A         open 'sys$scratch:tti_vendor.tmp' for input as file #1, & K                 access read, allow modify, map vendor_map, sequential fixed            close #1           end 