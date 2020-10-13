< //  Copyright 2006 Hewlett-Packard Development Company, L.P. //& //  An example of calling sys$getquiw. //3 //  Author: Stephen Hoffman, HP OpenVMS Engineering  // #include <descrip.h> #include <jbcmsgdef.h> #include <lib$routines.h>  #include <quidef.h>  #include <ssdef.h> #include <starlet.h> #include <stdio.h> #include <stsdef.h>    struct ItemList3     {      short int ItemLength;      short int ItemCode;      void *ItemBuffer;      void *ItemRetLen;      }; #define MAXQNAM 32 #define MAXIL   5  main()   {    int QuiCtx = -1;   int RetStat, RetLen, i;    int Efn = 0;   unsigned long int QuiIosb[2];     struct ItemList3 Il3[MAXIL+1];K   struct dsc$descriptor QueueD = { 0, DSC$K_DTYPE_T, DSC$K_CLASS_D, NULL }; .   $DESCRIPTOR( PromptD, "Enter Queue Name: ");  8   RetStat = lib$get_input( &QueueD, &PromptD, &RetLen );&   if (!$VMS_STATUS_SUCCESS( RetStat ))     lib$signal( RetStat );     RetStat = lib$get_ef( &Efn ); &   if (!$VMS_STATUS_SUCCESS( RetStat ))     lib$signal( RetStat );     i = 0;,   Il3[i].ItemLength   = QueueD.dsc$w_length;)   Il3[i].ItemCode     = QUI$_SEARCH_NAME; -   Il3[i].ItemBuffer   = QueueD.dsc$a_pointer;    Il3[i++].ItemRetLen = NULL;    Il3[i].ItemLength   = 0;   Il3[i].ItemCode     = 0;   Il3[i].ItemBuffer   = NULL;    Il3[i++].ItemRetLen = NULL;   H   RetStat = sys$getquiw(Efn,QUI$_DISPLAY_QUEUE,&QuiCtx,Il3,QuiIosb,0,0);&   if (!$VMS_STATUS_SUCCESS( RetStat ))     lib$signal( RetStat );     switch ( QuiIosb[0] )      {      default:=         printf("Error 0x0%08.8x on $getquiw\n", QuiIosb[0] );          break;     case JBC$_NOSUCHQUE:3         printf("\nQueue \'%*.*s\' does not exist",  K           QueueD.dsc$w_length, QueueD.dsc$w_length, QueueD.dsc$a_pointer );          break;     case SS$_NORMAL:     case JBC$_NORMAL: +         printf("\nQueue \'%*.*s\' exists",  K           QueueD.dsc$w_length, QueueD.dsc$w_length, QueueD.dsc$a_pointer );          break;     }      return SS$_NORMAL;   }     