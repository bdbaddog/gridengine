/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "sgermon.h"
#include "sge_gdi_intern.h"
#include "sge_answerL.h"
#include "sge_confL.h"
#include "rw_configuration.h"
#include "sge_log.h"
#include "sge_exit.h"
#include "parse_range.h"
#include "sge_rangeL.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "sge_spoolmsg.h"
#include "sge_feature.h" 

/*-----------------------------------------------------------------------*
 * write_configuration
 *  write configuration in human readable (and editable) form to file or 
 *  fileptr. If fname == NULL use fpout for writing.
 * if conf_list = NULL use config for writing
 *-----------------------------------------------------------------------*/
int write_configuration(
int spool,
lList **alpp,
char *fname,
lListElem *epc,
FILE *fpout,
u_long32 flags 
) {
   FILE *fp;
   lListElem *ep = NULL;
   lList *cfl;
   
   DENTER(TOP_LAYER, "write_configuration");

   if (fname) {
      SGE_FOPEN(fp, fname, "w");
      if (fp == NULL) {
         ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, fname, 
                strerror(errno)));
         if (!alpp) {
            SGE_EXIT(1);
         } else {
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return -1;  
         }
      }
   }
   else
      fp = fpout;

   if (spool && sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION)) < 0) {
      goto FPRINTF_ERROR;
   } 

   if (flags & FLG_CONF_SPOOL) {
      FPRINTF((fp, "%-25s " u32"\n", "conf_version", 
            lGetUlong(epc, CONF_version)));
      DPRINTF(("writing conf %s version " u32 "\n", fname, 
               lGetUlong(epc, CONF_version)));
   }

   cfl = lGetList(epc, CONF_entries);

   for_each(ep, cfl) {
      FPRINTF((fp, "%-25s %s\n", lGetString(ep, CF_name), 
               lGetString(ep, CF_value)));
   }

   if (fname) {
      FCLOSE(fp);
   }

   DEXIT;
   return 0;

FPRINTF_ERROR:
   if(errno != 0)
      CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITING_SS, fname, strerror(errno)));
   if(fname != NULL) {
      FCLEANUP(fp, fname);
   }   
   DEXIT;
   return -1;
}


/****** rw_configuration/read_adminuser_from_configuration() *******************
*  NAME
*     read_adminuser_from_configuration() 
*
*  SYNOPSIS
*     const char* read_adminuser_from_configuration(const char *fname, const 
*     char *conf_name, u_long32 flags) 
*
*  BUGS
*     Memory leak when used multiple times - shadowd only
*
*  SEE ALSO
*     should better use get_confval() instead
*******************************************************************************/
const char *read_adminuser_from_configuration(
const lListElem *el,
const char *fname, 
const char *conf_name,
u_long32 flags 
) {
   lListElem *conf = NULL;
   const char *ret = NULL;

   DENTER(TOP_LAYER, "read_adminuser_from_configuration");

   if (el == NULL) {
      el = read_configuration(fname, conf_name, flags);
   }

   for_each(conf, lGetList(el, CONF_entries)) {
      if (!strcmp(lGetString(conf, CF_name), "admin_user")) {
         ret = lGetString(conf, CF_value); 
         break;
      } 
   }
   DPRINTF(("admin_user: "SFN"\n", ret));

   DEXIT;
   return ret;
}

/*---------------------------------------------------------
 * read_configuration()
 * read configuration from file
 *
 * returns a CONF_Type list.
 *
 * fname: File which is read
 * conf_name: Internal name of configuration
 *---------------------------------------------------------*/
lListElem *read_configuration(
const char *fname,
const char *conf_name,
u_long32 flags 
) {
   FILE *fp;
   lListElem *ep=NULL, *epc;
   char buf[512], *value, *name;
   lList *lp;
   int first_line = 1;
   u_long32 conf_version = 0L;
   lList *alp = NULL;
   lList *rlp = NULL;

   DENTER(TOP_LAYER, "read_configuration");

   if (!(fp = fopen(fname, "r"))) {
      WARNING((SGE_EVENT, MSG_CONFIG_CONF_ERROROPENINGSPOOLFILE_SS, fname, strerror(errno)));
      DEXIT;
      return NULL;
   }
   
   lp = NULL;
   while (fgets(buf, sizeof(buf), fp)) {
      /* set chrptr to the first non blank character
       * If line is empty continue with next line   
       * sge_strtok() led to a error here because of recursive use
       * in parse_qconf.c
       */
      if(!(name = strtok(buf, " \t\n")))
         continue;

      /* allow commentaries */
      if (name[0] == '#')
         continue;

      if ((flags & FLG_CONF_SPOOL) && first_line) {
         first_line = 0;
         if (!strcmp(name, "conf_version")) {
            if (!(value = strtok(NULL, " \t\n"))) {
               WARNING((SGE_EVENT, MSG_CONFIG_CONF_NOVALUEFORCONFIGATTRIB_S, name));
               fclose(fp);
               DEXIT;
               return NULL;
            }
            sscanf(value, u32, &conf_version);
            DPRINTF(("read conf %s version " u32"\n", conf_name, conf_version));
            continue;
         } else {
            WARNING((SGE_EVENT, MSG_CONFIG_CONF_VERSIONNOTFOUNDONREADINGSPOOLFILE));
         }   
      }
      
      ep = lAddElemStr(&lp, CF_name, name, CF_Type);
      if (!ep) {
         WARNING((SGE_EVENT, MSG_CONFIG_CONF_ERRORSTORINGCONFIGVALUE_S, name));
         lFreeList(lp);
         fclose(fp);
         DEXIT;
         return NULL;
      }

      /* validate input */
      if (!strcmp(name, "gid_range")) {
         if ((value= strtok(NULL, " \t\n"))) {
            if (!strcmp(value, "none") ||
                !strcmp(value, "NONE")) {
               lSetString(ep, CF_value, value);
            } else if (!(rlp = parse_ranges(value, 0, 0, &alp, NULL,
                INF_NOT_ALLOWED))) {
               WARNING((SGE_EVENT, MSG_CONFIG_CONF_INCORRECTVALUEFORCONFIGATTRIB_SS, 
                        value, name));
               lFreeList(alp);
               lFreeList(lp);
               fclose(fp);
               DEXIT;
               return (NULL);
            } else {
               lListElem *rep;

               for_each (rep, rlp) {
                  long min;

                  min = lGetUlong(rep, RN_min);
                  if (min < GID_RANGE_NOT_ALLOWED_ID) {
                     WARNING((SGE_EVENT, MSG_CONFIG_CONF_GIDRANGELESSTHANNOTALLOWED_I, GID_RANGE_NOT_ALLOWED_ID));
                     lFreeList(alp);
                     lFreeList(lp);
                     fclose(fp);
                     DEXIT;
                     return (NULL);
                  }                  
               }
               lFreeList(alp);
               lFreeList(rlp);
               lSetString(ep, CF_value, value);
            }
         }
      } 
      else if (!strcmp(name, "admin_user")) {
         value = strtok(NULL, " \t\n");
         while (value[0] && isspace((int) value[0]))
            value++;
         if (value) {
            lSetString(ep, CF_value, value);
         } else {
            WARNING((SGE_EVENT, MSG_CONFIG_CONF_NOVALUEFORCONFIGATTRIB_S, name));
            lFreeList(lp);
            fclose(fp);
            DEXIT;
            return NULL;
         }
      } 
      else if (!strcmp(name, "user_lists") || 
         !strcmp(name, "xuser_lists") || 
         !strcmp(name, "projects") || 
         !strcmp(name, "xprojects") || 
         !strcmp(name, "prolog") || 
         !strcmp(name, "epilog") || 
         !strcmp(name, "starter_method") || 
         !strcmp(name, "suspend_method") || 
         !strcmp(name, "resume_method") || 
         !strcmp(name, "terminate_method") || 
         !strcmp(name, "qmaster_params") || 
         !strcmp(name, "execd_params") || 
         !strcmp(name, "schedd_params") ||
         !strcmp(name, "qlogin_command") ||
         !strcmp(name, "rlogin_command") ||
         !strcmp(name, "rsh_command") ||
         !strcmp(name, "qlogin_daemon") ||
         !strcmp(name, "rlogin_daemon") ||
         !strcmp(name, "rsh_daemon")) {
         if (!(value = strtok(NULL, "\t\n"))) {
            /* return line if value is empty */
            WARNING((SGE_EVENT, MSG_CONFIG_CONF_NOVALUEFORCONFIGATTRIB_S, name));
            lFreeList(lp);
            fclose(fp);
            DEXIT;
            return NULL;
         }
         /* skip leading delimitors */
         while (value[0] && isspace((int) value[0]))
            value++;

         lSetString(ep, CF_value, value);
      } else {
         if (!(value = strtok(NULL, " \t\n"))) {
            WARNING((SGE_EVENT, MSG_CONFIG_CONF_NOVALUEFORCONFIGATTRIB_S, name));
            lFreeList(lp);
            fclose(fp);
            DEXIT;
            return NULL;
         }
           
         lSetString(ep, CF_value, value);

         if (strtok(NULL, " \t\n")) {
            /* Allow only one value per line */
            WARNING((SGE_EVENT, MSG_CONFIG_CONF_ONLYSINGLEVALUEFORCONFIGATTRIB_S, name));
            fclose(fp);
            DEXIT;
            return NULL;
         }
      }
   }

   fclose(fp);

   epc = lCreateElem(CONF_Type);
   lSetHost(epc, CONF_hname, conf_name);
   lSetUlong(epc, CONF_version, conf_version);
   lSetList(epc, CONF_entries, lp);

   DEXIT;
   return epc;
}