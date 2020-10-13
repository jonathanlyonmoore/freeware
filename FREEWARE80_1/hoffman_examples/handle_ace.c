! #module	HANDLE_ACE "SRH X1.0-000"  #pragma builtins   /* ** COPYRIGHT (c) 1992 BY9 ** DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASSACHUSETTS.  ** ALL RIGHTS RESERVED.  **H ** THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED AND COPIEDH ** ONLY  IN  ACCORDANCE  OF  THE  TERMS  OF  SUCH  LICENSE  AND WITH THEH ** INCLUSION OF THE ABOVE COPYRIGHT NOTICE. THIS SOFTWARE OR  ANY  OTHERH ** COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANYH ** OTHER PERSON.  NO TITLE TO AND  OWNERSHIP OF THE  SOFTWARE IS  HEREBY ** TRANSFERRED.  **H ** THE INFORMATION IN THIS SOFTWARE IS  SUBJECT TO CHANGE WITHOUT NOTICEH ** AND  SHOULD  NOT  BE  CONSTRUED  AS A COMMITMENT BY DIGITAL EQUIPMENT ** CORPORATION.  **H ** DIGITAL ASSUMES NO RESPONSIBILITY FOR THE USE  OR  RELIABILITY OF ITS: ** SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DIGITAL. */   /* **++ **  Facility:  ** **	Examples  ** **  Version: V1.0  ** **  Abstract:  **= **	Example of messing around with Access Control List Entries + **	via SYS$CHANGE_ACL system service calls.  ** **  Author:  **	Steve Hoffman ** **  Creation Date:  1-Jan-1990 ** **  Modification History:  **-- */ #module HANDLE_ACE "V1.0"    #include acedef  #include acldef  #include descrip #include lib$routines  #include ssdef #include starlet #include stdio  8 unsigned long int handle_ace(filename_dsc,operation_dsc)$ struct dsc$descriptor *filename_dsc;% struct dsc$descriptor *operation_dsc;  {    unsigned long int status;    unsigned long int status1;    unsigned long int context = 0;
   struct {#            unsigned char  int size; #            unsigned char  int type; $            unsigned short int flags;)            unsigned long  int info_flags;             char facility[8];            char status[8];          } information_ace; 
   struct {&            unsigned short int buf_len;(            unsigned short int item_code;'            unsigned long  int buf_addr; +            unsigned long  int ret_len_addr; )            unsigned long  int terminator; #          } item_list = {0,0,0,0,0};   :   memset( &information_ace, 0, sizeof( information_ace) );  1   information_ace.size = sizeof(information_ace); %   information_ace.type =  ACE$C_INFO; &   information_ace.flags = ACE$C_CUST |9     (ACE$M_HIDDEN | ACE$M_PROTECTED | ACE$M_NOPROPAGATE); !   information_ace.info_flags = 0; -   strcpy( information_ace.facility, "TEST" ); -   strcpy( information_ace.status, "STATUS" );      /* Lock ACL for writing */     item_list.buf_len   = 0;(   item_list.item_code = ACL$C_WLOCK_ACL;   item_list.buf_addr  = 0;  6   status1 = sys$change_acl(0,&ACL$C_FILE,filename_dsc,!     &item_list,0,0,&context,0,0); &   if (!(status1 & 1)) return(status1);     /* Initialize item list */  0   item_list.buf_len  = sizeof (information_ace);(   item_list.buf_addr = &information_ace;  ,   switch (*(operation_dsc -> dsc$a_pointer))   {      case 'a':      case 'A': 8       item_list.item_code = ACL$C_ADDACLENT;	  /* ADD */       break;     case 'd':      case 'D': ;       item_list.item_code = ACL$C_DELACLENT;	  /* DELETE */        break;     case 'c':      case 'C': :       item_list.item_code = ACL$C_FNDACLENT;	  /* CHECK */       break;     default:  8       return (SS$_BADPARAM);			  /* Unknown operation */   }   "   /* Perform selected operation */  5   status = sys$change_acl(0,&ACL$C_FILE,filename_dsc,    &item_list,0,0,&context,0,0);      /* Unlock ACL */     item_list.buf_len   = 0;)   item_list.item_code = ACL$C_UNLOCK_ACL;    item_list.buf_addr  = 0;  6   status1 = sys$change_acl(0,&ACL$C_FILE,filename_dsc,   &item_list,0,0,&context,0,0);   &   if (!(status1 & 1)) return(status1);%   if (!(status  & 1)) return(status);      return(status);  }   ! unsigned long int main(argc,argv)  unsigned long int argc;  char *argv[];  {    unsigned long int status; $   $DESCRIPTOR(filename_dsc,argv[1]);%   $DESCRIPTOR(operation_dsc,argv[2]);      if (argc < 3)    {      printf("\n"); R     printf("This program sets, clears or checks an information ACE on a file.\n");     printf("\n");      printf("Syntax:\n");     printf("\n"); 6     printf("  $ mcr %s filename operation\n",argv[0]);     printf("\n");      printf("\n");      printf("Parameters:\n");     printf("\n"); R     printf("  filename      The name of the file to perform the operation on.\n");     printf("\n"); N     printf("  operation     The name of the operation to perform. One of:\n");7     printf("                ADD        Add the ACE\n"); C     printf("                CHECK      Check if the ACE exists\n"); :     printf("                DELETE     Delete the ACE\n");     printf("\n");      return(SS$_NORMAL);    }   /   filename_dsc.dsc$w_length  = strlen(argv[1]); /   operation_dsc.dsc$w_length = strlen(argv[2]);   4   status = handle_ace(&filename_dsc,&operation_dsc);   lib$signal(status);      return(status);  } ��                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            