 #include <descrip.h> #include <lib$routines.h>  #include <lmfdef.h>  #include <ssdef.h> #include <starlet.h> #include <stddef.h>  #include <stdio.h> #include <stsdef.h>    #include descrip #define LMF$_PROD_VERSION 3  #define LMF$_PROD_DATE 4 #define LMF$_PROD_TOKEN 1    typedef struct _quad_word {  long int upper_long; long int lower_long; } quad_word;   typedef struct _long_word {  short int upper_word;  short int lower_word;  } long_word;   main() {  long status; quad_word product_date;  long_word product_vers = {6,4};    struct { short itm$w_buflen ; short itm$w_code ; char *itm$a_addr ; short *itm$a_lenaddr ; } grant_license_items[] = { > sizeof (quad_word), LMF$_PROD_DATE, (char *)&product_date, 0, ' sizeof (long_word), LMF$_PROD_VERSION,  ( (char *)&product_vers, 0, 0, 0, 0, 0 };   0 static $DESCRIPTOR (product_name, "MYPROGRAM"); / static $DESCRIPTOR (product_owner, "FREEWARE");     6 /* Call the SYS$GRANT_LICENSE system service and store    the return status. */, status = SYS$GRANT_LICENSE( &product_name,  . 			&product_owner, 0,  &grant_license_items); ' printf("Lookup status is %x",status); } 