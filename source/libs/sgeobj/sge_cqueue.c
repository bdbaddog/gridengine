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

#include <string.h>
#include <fnmatch.h>

#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "cull_list.h"
#include "symbols.h"
#include "sge.h"

#include "sge_gdi.h"

#include "parse.h"
#include "sge_dstring.h"
#include "sge_object.h"
#include "sge_answer.h"
#include "sge_attr.h"
#include "sge_centry.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_str.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_feature.h"
#include "sge_href.h"
#include "sge_hgroup.h"
#include "sge_pe.h"
#include "sge_calendar.h"
#include "sge_ckpt.h"
#include "sge_qref.h"
#include "sge_range.h"
#include "sge_subordinate.h"
#include "sge_hostname.h"
#include "commlib.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define CQUEUE_LAYER TOP_LAYER

/* *INDENT-OFF* */

list_attribute_struct cqueue_attribute_array[] = {
   { CQ_seq_no,                  QU_seq_no,                 AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_SEQ_NO,            false,  false, NULL},
   { CQ_nsuspend,                QU_nsuspend,               AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_NSUSPEND,          false,  false, NULL},
   { CQ_job_slots,               QU_job_slots,              AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_SLOTS,             false,  false, NULL},
   { CQ_fshare,                  QU_fshare,                 AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_FSHARE,            true,   false, NULL},
   { CQ_oticket,                 QU_oticket,                AULNG_href,    AULNG_value,      NoName,     SGE_ATTR_OTICKET,           true,   false, NULL},

   { CQ_tmpdir,                  QU_tmpdir,                 ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_TMPDIR,            false,  false, NULL},
   { CQ_shell,                   QU_shell,                  ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_SHELL,             false,  false, NULL},
   { CQ_calendar,                QU_calendar,               ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_CALENDAR,          false,  false, cqueue_verify_calendar},
   { CQ_priority,                QU_priority,               ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_PRIORITY,          false,  true,  cqueue_verify_priority},
   { CQ_processors,              QU_processors,             ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_PROCESSORS,        false,  true,  cqueue_verify_processors},
   { CQ_prolog,                  QU_prolog,                 ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_PROLOG,            false,  false, NULL},
   { CQ_epilog,                  QU_epilog,                 ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_EPILOG,            false,  false, NULL},
   { CQ_shell_start_mode,        QU_shell_start_mode,       ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_SHELL_START_MODE,  false,  true,  cqueue_verify_shell_start_mode},
   { CQ_starter_method,          QU_starter_method,         ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_STARTER_METHOD,    false,  false, NULL},
   { CQ_suspend_method,          QU_suspend_method,         ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_SUSPEND_METHOD,    false,  false, NULL},
   { CQ_resume_method,           QU_resume_method,          ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_RESUME_METHOD,     false,  false, NULL},
   { CQ_terminate_method,        QU_terminate_method,       ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_TERMINATE_METHOD,  false,  false, NULL},
   { CQ_initial_state,           QU_initial_state,          ASTR_href,     ASTR_value,       NoName,     SGE_ATTR_INITIAL_STATE,     false,  true,  cqueue_verify_initial_state},
   
   { CQ_rerun,                   QU_rerun,                  ABOOL_href,    ABOOL_value,      NoName,     SGE_ATTR_RERUN,             false,  false, NULL},

   { CQ_s_fsize,                 QU_s_fsize,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_FSIZE,           false,  false, NULL},
   { CQ_h_fsize,                 QU_h_fsize,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_FSIZE,           false,  false, NULL},
   { CQ_s_data,                  QU_s_data,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_DATA,            false,  false, NULL},
   { CQ_h_data,                  QU_h_data,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_DATA,            false,  false, NULL},
   { CQ_s_stack,                 QU_s_stack,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_STACK,           false,  false, NULL},
   { CQ_h_stack,                 QU_h_stack,                AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_STACK,           false,  false, NULL},
   { CQ_s_core,                  QU_s_core,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_CORE,            false,  false, NULL},
   { CQ_h_core,                  QU_h_core,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_CORE,            false,  false, NULL},
   { CQ_s_rss,                   QU_s_rss,                  AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_RSS,             false,  false, NULL},
   { CQ_h_rss,                   QU_h_rss,                  AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_RSS,             false,  false, NULL},
   { CQ_s_vmem,                  QU_s_vmem,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_S_VMEM,            false,  false, NULL},
   { CQ_h_vmem,                  QU_h_vmem,                 AMEM_href,     AMEM_value,       NoName,     SGE_ATTR_H_VMEM,            false,  false, NULL},

   { CQ_s_rt,                    QU_s_rt,                   ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_S_RT,              false,  false, NULL},
   { CQ_h_rt,                    QU_h_rt,                   ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_H_RT,              false,  false, NULL},
   { CQ_s_cpu,                   QU_s_cpu,                  ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_S_CPU,             false,  false, NULL},
   { CQ_h_cpu,                   QU_h_cpu,                  ATIME_href,    ATIME_value,      NoName,     SGE_ATTR_H_CPU,             false,  false, NULL},

   { CQ_suspend_interval,        QU_suspend_interval,       AINTER_href,   AINTER_value,     NoName,     SGE_ATTR_SUSPEND_INTERVAL,  false,  false, NULL},
   { CQ_min_cpu_interval,        QU_min_cpu_interval,       AINTER_href,   AINTER_value,     NoName,     SGE_ATTR_MIN_CPU_INTERVAL,  false,  false, NULL},
   { CQ_notify,                  QU_notify,                 AINTER_href,   AINTER_value,     NoName,     SGE_ATTR_NOTIFY,            false,  false, NULL},

   { CQ_qtype,                   QU_qtype,                  AQTLIST_href,  AQTLIST_value,    NoName,     SGE_ATTR_QTYPE,             false,  false, NULL},

   { CQ_ckpt_list,               QU_ckpt_list,              ASTRLIST_href, ASTRLIST_value,   ST_name,    SGE_ATTR_CKPT_LIST,         false,  false, cqueue_verify_ckpt_list},
   { CQ_pe_list,                 QU_pe_list,                ASTRLIST_href, ASTRLIST_value,   ST_name,    SGE_ATTR_PE_LIST,           false,  false, cqueue_verify_pe_list},
 
   { CQ_owner_list,              QU_owner_list,             AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_OWNER_LIST,        false,  false, NULL},
   { CQ_acl,                     QU_acl,                    AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_USER_LISTS,        false,  false, cqueue_verify_user_list},
   { CQ_xacl,                    QU_xacl,                   AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_XUSER_LISTS,       false,  false, cqueue_verify_user_list},

   { CQ_projects,                QU_projects,               APRJLIST_href, APRJLIST_value,   UP_name,    SGE_ATTR_PROJECTS,          true,   false, cqueue_verify_project_list},
   { CQ_xprojects,               QU_xprojects,              APRJLIST_href, APRJLIST_value,   UP_name,    SGE_ATTR_XPROJECTS,         true,   false, cqueue_verify_project_list},

   { CQ_consumable_config_list,  QU_consumable_config_list, ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_COMPLEX_VALUES,    false,  false, NULL},
   { CQ_load_thresholds,         QU_load_thresholds,        ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_LOAD_THRESHOLD,    false,  false, NULL},
   { CQ_suspend_thresholds,      QU_suspend_thresholds,     ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_SUSPEND_THRESHOLD, false,  false, NULL},

   { CQ_subordinate_list,        QU_subordinate_list,       ASOLIST_href,  ASOLIST_value,    SO_name,    SGE_ATTR_SUBORDINATE_LIST,  false,  false, cqueue_verify_subordinate_list},

   { NoName,                     NoName,                    NoName,        NoName,           NoName,     NULL,                       false,  false, NULL}
};

/* *INDENT-ON* */

lList *Master_CQueue_List = NULL;

bool
cqueue_name_split(const char *name, 
                  dstring *cqueue_name, dstring *host_domain, 
                  bool *has_hostname, bool *has_domain)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_name_split");
   if (name != NULL && cqueue_name != NULL && 
       host_domain != NULL && has_hostname != NULL && has_domain != NULL) {
      int part = 0;
      const char *tmp_string;

      while (*name != '\0') {
         if (part == 1) {
            part = 2;
         } else if (part == 0 && *name == '@') {
            part = 1;
         }
         if (part == 0) {
            sge_dstring_sprintf_append(cqueue_name, "%c", name[0]);
         } else if (part == 2) {
            sge_dstring_sprintf_append(host_domain, "%c", name[0]);
         }
         name++;
      } 
      tmp_string = sge_dstring_get_string(host_domain);
      *has_hostname = false;
      *has_domain = false;
      if (tmp_string != NULL) {
         if (tmp_string[0] == '@') {
            *has_domain = true;
         } else {
            *has_hostname = true;
         }
      } 
   }
   DEXIT;
   return ret;
}

lEnumeration *
enumeration_create_reduced_cq(bool fetch_all_qi, bool fetch_all_nqi)
{
   lEnumeration *ret;
   dstring format_string = DSTRING_INIT;
   lDescr *descr = CQ_Type;
   int name_array[100];
   int names = -1;
   int attr;

   DENTER(CQUEUE_LAYER, "enumeration_create_reduced_cq");
   for_each_attr(attr, descr) {
      if (names == -1) {
         sge_dstring_sprintf(&format_string, "%s", "%T(");
      }
      if ((attr == CQ_name) ||
          (fetch_all_qi && attr == CQ_qinstances) ||
          (fetch_all_nqi && attr != CQ_qinstances)) {
         names++;
         name_array[names] = attr;
         sge_dstring_sprintf_append(&format_string, "%s", "%I");
      }
   }
   sge_dstring_sprintf_append(&format_string, "%s", ")");
   ret = _lWhat(sge_dstring_get_string(&format_string), CQ_Type, name_array, ++names);
   sge_dstring_free(&format_string);
   
   DEXIT;
   return ret;
}

lListElem *
cqueue_create(lList **answer_list, const char *name)
{
   lListElem *ret = NULL;

   DENTER(CQUEUE_LAYER, "cqueue_create");
   if (name != NULL) {
      ret = lCreateElem(CQ_Type);

      if (ret != NULL) {
         lSetString(ret, CQ_name, name);
      } else {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                                MSG_MEM_MEMORYALLOCFAILED_S, SGE_FUNC));
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_is_href_referenced(const lListElem *this_elem, const lListElem *href)
{
   bool ret = false;

   if (this_elem != NULL && href != NULL) {
      const char *href_name = lGetHost(href, HR_name);
      
      if (href_name != NULL) {
         lList *href_list = lGetList(this_elem, CQ_hostlist);
         lListElem *tmp_href = lGetElemHost(href_list, HR_name, href_name);

         if (tmp_href != NULL) {
            ret = true;
         }
      }
   }
   return ret;
} 

bool 
cqueue_is_hgroup_referenced(const lListElem *this_elem, const lListElem *hgroup)
{
   bool ret = false;

   if (this_elem != NULL && hgroup != NULL) {
      const char *name = lGetHost(hgroup, HGRP_name);
      
      if (name != NULL) {
         lList *href_list = lGetList(this_elem, CQ_hostlist);
         lListElem *tmp_href = lGetElemHost(href_list, HR_name, name);

         if (tmp_href != NULL) {
            ret = true;
         }
      }
   }
   return ret;
} 

bool 
cqueue_is_a_href_referenced(const lListElem *this_elem, const lList *href_list)
{
   bool ret = false;
  
   if (this_elem != NULL && href_list != NULL) { 
      lListElem *href;

      for_each(href, href_list) {
         if (cqueue_is_href_referenced(this_elem, href)) {
            ret = true;
            break;
         }
      }
   }
   return ret;
} 

bool
cqueue_set_template_attributes(lListElem *this_elem, lList **answer_list)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_set_template_attributes");
   if (this_elem != NULL) {
      /*
       * initialize u_long32 values
       */
      if (ret) {
         const u_long32 value[] = {
            0, 1, 1, 0 
         }; 
         const int attr[] = {
            CQ_seq_no, CQ_nsuspend, CQ_job_slots, NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, AULNG_href, 
                                                HOSTREF_DEFAULT, AULNG_Type);

            lSetUlong(attr_elem, AULNG_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize u_long32 values (SGEEE attributes)
       */
      if (ret && feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
         const u_long32 value[] = {
            0, 0, 0 
         }; 
         const int attr[] = {
            CQ_fshare, CQ_oticket, NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, AULNG_href, 
                                                HOSTREF_DEFAULT, AULNG_Type);

            lSetUlong(attr_elem, AULNG_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * qtype
       */
      if (ret) {
         const char *string = "BATCH INTERACTIVE";
         u_long32 value = 0;
         lList *attr_list = NULL;
         lListElem *attr_elem = lAddElemHost(&attr_list, AQTLIST_href,
                                             HOSTREF_DEFAULT, AQTLIST_Type);

         sge_parse_bitfield_str(string, queue_types, &value, "",
                                answer_list, true);

         lSetUlong(attr_elem, AQTLIST_value, value);
         lSetList(this_elem, CQ_qtype, attr_list);
      }

      /*
       * initialize bool values
       */
      if (ret) {
         lList *attr_list = NULL;
         lListElem *attr = lAddElemHost(&attr_list, ABOOL_href, 
                                        HOSTREF_DEFAULT, ABOOL_Type);

         lSetBool(attr, ABOOL_value, true);
         lSetList(this_elem, CQ_rerun, attr_list);
      }

      /*
       * initialize memory values
       */
      if (ret) {
         const char *value[] = {
            "INFINITY", "INFINITY", "INFINITY", "INFINITY",
            "INFINITY", "INFINITY", "INFINITY", "INFINITY",
            "INFINITY", "INFINITY", "INFINITY", "INFINITY",
            NULL
         }; 
         const int attr[] = {
            CQ_s_fsize, CQ_h_fsize, CQ_s_data, CQ_h_data,
            CQ_s_stack, CQ_h_stack, CQ_s_core, CQ_h_core,
            CQ_s_rss, CQ_h_rss, CQ_s_vmem, CQ_h_vmem,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, AMEM_href, 
                                                HOSTREF_DEFAULT, AMEM_Type);

            lSetString(attr_elem, AMEM_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }
      
      /*
       * initialize time values
       */
      if (ret) {
         const char *value[] = {
            "INFINITY", "INFINITY", "INFINITY", "INFINITY",
            NULL
         }; 
         const int attr[] = {
            CQ_s_rt, CQ_h_rt, CQ_s_cpu, CQ_h_cpu,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, ATIME_href, 
                                                HOSTREF_DEFAULT, ATIME_Type);

            lSetString(attr_elem, ATIME_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize interval values
       */
      if (ret) {
         const char *value[] = {
            "00:05:00", "00:05:00", "00:00:60",
            NULL
         }; 
         const int attr[] = {
            CQ_suspend_interval, CQ_min_cpu_interval, CQ_notify,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, AINTER_href, 
                                                HOSTREF_DEFAULT, AINTER_Type);

            lSetString(attr_elem, AINTER_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize string values
       */
      if (ret) {
         const char *value[] = {
            "/tmp", "/bin/csh", "NONE",
            "0", "UNDEFINED", "NONE",
            "NONE", "posix_compliant", "NONE",
            "NONE", "NONE", "NONE",
            "default", 
            NULL
         }; 
         const int attr[] = {
            CQ_tmpdir, CQ_shell, CQ_calendar,
            CQ_priority, CQ_processors, CQ_prolog,
            CQ_epilog, CQ_shell_start_mode, CQ_starter_method,
            CQ_suspend_method, CQ_resume_method, CQ_terminate_method,
            CQ_initial_state,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, ASTR_href, 
                                                HOSTREF_DEFAULT, ASTR_Type);

            lSetString(attr_elem, ASTR_value, value[index]);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize string-list values
       */
      if (ret) {
         const int attr[] = {
            CQ_pe_list, CQ_ckpt_list,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, ASTRLIST_href, 
                                                HOSTREF_DEFAULT, ASTRLIST_Type);

            lSetList(attr_elem, ASTRLIST_value, NULL);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize AUSRLIST_Type-list values
       */
      if (ret) {
         const int attr[] = {
            CQ_owner_list, CQ_acl, CQ_xacl,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, AUSRLIST_href, 
                                                HOSTREF_DEFAULT, AUSRLIST_Type);

            lSetList(attr_elem, AUSRLIST_value, NULL);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize APRJLIST_Type-list values (only sgeee mode)
       */
      if (ret && feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
         const int attr[] = {
            CQ_projects, CQ_xprojects,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, APRJLIST_href, 
                                                HOSTREF_DEFAULT, APRJLIST_Type);

            lSetList(attr_elem, APRJLIST_value, NULL);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize ACELIST_Type-list values
       */
      if (ret) {
         const int attr[] = {
            CQ_load_thresholds, CQ_suspend_thresholds, 
            CQ_consumable_config_list,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, ACELIST_href, 
                                                HOSTREF_DEFAULT, ACELIST_Type);

            lSetList(attr_elem, ACELIST_value, NULL);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }

      /*
       * initialize ASOLIST_Type-list values
       */
      if (ret) {
         const int attr[] = {
            CQ_subordinate_list,
            NoName
         };
         int index = 0;

         while (attr[index] != NoName) {
            lList *attr_list = NULL;
            lListElem *attr_elem = lAddElemHost(&attr_list, ASOLIST_href, 
                                                HOSTREF_DEFAULT, ASOLIST_Type);

            lSetList(attr_elem, ASOLIST_value, NULL);
            lSetList(this_elem, attr[index], attr_list);
            index++;
         }
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_list_add_cqueue(lListElem *queue)
{
   bool ret = false;
   static lSortOrder *so = NULL;
   lList **master_list = object_type_get_master_list(SGE_TYPE_CQUEUE);

   DENTER(TOP_LAYER, "cqueue_list_add_cqueue");

   if (queue != NULL) {
      if (so == NULL) {
         so = lParseSortOrderVarArg(CQ_Type, "%I+", CQ_name);
      }

      if (*master_list== NULL) {
         *master_list = lCreateList("", CQ_Type);
      }

      lInsertSorted(so, queue, *master_list);
      ret = true;
   } 
   DEXIT;
   return ret;
}

lListElem *
cqueue_list_locate(const lList *this_list, const char *name)
{
   return lGetElemStr(this_list, CQ_name, name);
}

lListElem *
cqueue_locate_qinstance(const lListElem *this_elem, const char *hostname)
{
   lList *qinstance_list = lGetList(this_elem, CQ_qinstances);

   return qinstance_list_locate(qinstance_list, hostname, NULL);
}

bool 
cqueue_verify_attributes(lListElem *cqueue, lList **answer_list,
                         lListElem *reduced_elem, bool in_master)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_verify_attributes");
   if (cqueue != NULL && reduced_elem != NULL) {
      int index = 0;

      while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
         if (!cqueue_attribute_array[index].is_sgeee_attribute ||
             feature_is_enabled(FEATURE_SGEEE)) {
            int pos = lGetPosViaElem(reduced_elem,
                                     cqueue_attribute_array[index].cqueue_attr);

            if (pos >= 0) {
               lList *list = NULL;

               list = lGetList(cqueue,
                               cqueue_attribute_array[index].cqueue_attr);

               /*
                * Configurations without default setting are rejected
                */
               if (ret) {
                  lListElem *elem = lGetElemHost(list, 
                      cqueue_attribute_array[index].href_attr, HOSTREF_DEFAULT);

                  if (elem == NULL) {
                     SGE_ADD_MSG_ID(sprintf(SGE_EVENT,
                                    MSG_CQUEUE_NODEFVALUE_S, 
                                    cqueue_attribute_array[index].name));
                     answer_list_add(answer_list, SGE_EVENT,
                                     STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
                     ret = false;
                  } 
               }

               /*
                * Reject multiple settings for one domain/host
                * and resolve all hostnames
                */
               if (ret) {
                  lListElem *elem = NULL;

                  for_each(elem, list) {
                     const char *hostname = NULL;
                     const void *iterator = NULL;
                     lListElem *first_elem = NULL;

                     hostname = lGetHost(elem, 
                           cqueue_attribute_array[index].href_attr);
                     first_elem = lGetElemHostFirst(list,
                           cqueue_attribute_array[index].href_attr,
                           hostname, &iterator);

                     if (elem != first_elem) {
                        SGE_ADD_MSG_ID(sprintf(SGE_EVENT,
                                      MSG_CQUEUE_MULVALNOTALLOWED_S, hostname));
                        answer_list_add(answer_list, SGE_EVENT,
                                        STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
                        ret = false;
                        break;
                     }
                     if (!sge_is_hgroup_ref(hostname)) {
                        char resolved_name[MAXHOSTLEN+1];
                        int back = getuniquehostname(hostname, resolved_name, 0);
#ifdef ENABLE_NGC
                        if (back == CL_RETVAL_OK)
#else
                        if (back == 0)
#endif
                        {
                           lSetHost(elem, 
                                    cqueue_attribute_array[index].href_attr, 
                                    resolved_name);
                        } else {
                           ERROR((SGE_EVENT, MSG_HGRP_UNKNOWNHOST, hostname));
                           answer_list_add(answer_list, SGE_EVENT,
                                         STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
                           ret = false;
                           break;
                        }
                     }
                  }
               }


#if 0
               /*
                * Reject settings for domains/hosts which are not part of
                * the hostlist specification
                */
               if (ret & in_master) {
                  lList *hostlist = lGetList(cqueue, CQ_hostlist);
                  lList *master_list = *(hgroup_list_get_master_list());
                  lList *hosts = NULL;
                  lList *groups = NULL;
                  lListElem *elem = NULL;

                  ret &= href_list_find_all_references(hostlist, answer_list,
                                                       master_list, &hosts,
                                                       &groups);
                  if (ret) {
                     for_each(elem, list) {
                        const char *host_hgroup = lGetHost(elem,
                              cqueue_attribute_array[index].href_attr);
                        lList *tmp_host_hgroup = NULL;
                        
                        href_list_add(&tmp_host_hgroup, 
                                      answer_list, host_hgroup);
                        href_list_resolve_hostnames(tmp_host_hgroup,
                                                    answer_list);
                        host_hgroup = lGetHost(lFirst(tmp_host_hgroup), 
                                               HR_name);
                        lSetHost(elem, cqueue_attribute_array[index].href_attr,
                                 host_hgroup);
                        if (strcmp(HOSTREF_DEFAULT, host_hgroup) != 0 &&
                            !href_list_has_member(hostlist, host_hgroup) &&
                            !href_list_has_member(hosts, host_hgroup) &&
                            !href_list_has_member(groups, host_hgroup)) {
                           SGE_ADD_MSG_ID(sprintf(SGE_EVENT,
                                          MSG_CQUEUE_NOTINHOSTLIST_S, 
                                          host_hgroup));
                           answer_list_add(answer_list, SGE_EVENT,
                                           STATUS_EUNKNOWN, 
                                           ANSWER_QUALITY_ERROR);
                           ret = false;
                           break;
                        }
                        tmp_host_hgroup = lFreeList(tmp_host_hgroup);
                     }
                  }
                  hosts = lFreeList(hosts);
                  groups = lFreeList(groups);
               }
#endif

               if (ret && 
                   cqueue_attribute_array[index].verify_function != NULL &&
                   (cqueue_attribute_array[index].verify_client || in_master)) {
                  lListElem *elem = NULL;

                  for_each(elem, list) {
                     ret &= cqueue_attribute_array[index].
                                    verify_function(cqueue, answer_list, elem);
                  }
               }
            }
         }
         index++;
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_mod_sublist(lListElem *this_elem, lList **answer_list,
                   lListElem *reduced_elem, int sub_command,
                   int attribute_name, int sublist_host_name,
                   int sublist_value_name, int subsub_key,
                   const char *attribute_name_str, 
                   const char *object_name_str) 
{
   bool ret = true;
   int pos;

   DENTER(CQUEUE_LAYER, "cqueue_mod_sublist");
  
   pos = lGetPosViaElem(reduced_elem, attribute_name);
   if (pos >= 0) {
      lList *mod_list = lGetPosList(reduced_elem, pos);
      lList *org_list = lGetList(this_elem, attribute_name);
      lListElem *mod_elem;

      /* 
       * Delete all configuration lists except the default-configuration
       * if sub_command is SGE_GDI_SET_ALL
       */
      if (sub_command == SGE_GDI_SET_ALL) {
         lListElem *elem, *next_elem;

         next_elem = lFirst(org_list);
         while ((elem = next_elem)) {
            const char *name = lGetHost(elem, sublist_host_name);

            next_elem = lNext(elem); 
            mod_elem = lGetElemHost(mod_list, sublist_host_name, name);
            if (mod_elem == NULL) {
               DPRINTF(("Removing attribute list for "SFQ"\n", name));
               lRemoveElem(org_list, elem);
            }
         }
      }

      /*
       * Do modifications for all given elements of 
       * domain/host-configuration list
       */
      for_each(mod_elem, mod_list) {
         const char *name = lGetHost(mod_elem, sublist_host_name);
         lListElem *org_elem = lGetElemHost(org_list, sublist_host_name, name);

         /*
          * Create element if it does not exist
          */
         if (org_elem == NULL) {
            if (org_list == NULL) {
               org_list = lCreateList("", lGetElemDescr(mod_elem));
               lSetList(this_elem, attribute_name, org_list);
            } 
            org_elem = lCreateElem(lGetElemDescr(mod_elem));
            /* EB: TODO: error message and error handling */
            lSetHost(org_elem, sublist_host_name, name);
            lAppendElem(org_list, org_elem);
         }

         /*
          * Modify sublist according to subcommand
          */
         if (org_elem != NULL) {
            if (subsub_key != NoName) {
               attr_mod_sub_list(answer_list, org_elem, sublist_value_name, 
                                 subsub_key, mod_elem, sub_command, 
                                 attribute_name_str, object_name_str, 0);
            } else {
               object_replace_any_type(org_elem, sublist_value_name, mod_elem);
            }
         }
      }
   }
 
   DEXIT;
   return ret;
}

bool
cqueue_list_find_all_matching_references(const lList *this_list,
                                         lList **answer_list,
                                         const char *cqueue_pattern,
                                         lList **qref_list)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_list_find_all_matching_references");
   if (this_list != NULL && cqueue_pattern != NULL && qref_list != NULL) {
      lListElem *cqueue;

      for_each(cqueue, this_list) {
         const char *cqueue_name = lGetString(cqueue, CQ_name);
         
         if (!fnmatch(cqueue_pattern, cqueue_name, 0)) {
            if (*qref_list == NULL) {
               *qref_list = lCreateList("", QR_Type);
            }
            if (*qref_list != NULL) {
               lAddElemStr(qref_list, QR_name, cqueue_name, QR_Type);
            }
         }
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_xattr_pre_gdi(lList *this_list, lList **answer_list) 
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_xattr_pre_gdi");
   if (this_list != NULL) {
      lListElem *cqueue = NULL;
   
      for_each(cqueue, this_list) {
         const char *name = lGetString(cqueue, CQ_name);
         dstring cqueue_name = DSTRING_INIT;
         dstring host_domain = DSTRING_INIT;
         bool has_hostname = false;
         bool has_domain = false;

         cqueue_name_split(name, &cqueue_name, &host_domain,
                           &has_hostname, &has_domain);
         if (has_domain || has_hostname) {
            int index = 0;

            /*
             * Change QI/QD name to CQ name
             */
            lSetString(cqueue, CQ_name, sge_dstring_get_string(&cqueue_name));

            /*
             * Make sure that there is only a default entry
             * and change that default entry to be a QD/QI entry
             */
            while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
               int pos = lGetPosViaElem(cqueue,
                                  cqueue_attribute_array[index].cqueue_attr);

               if (pos >= 0) {
                  lList *list = lGetPosList(cqueue, pos);
                  lListElem *elem = NULL;

                  for_each(elem, list) {
                     const char *attr_hostname = lGetHost(elem, 
                                       cqueue_attribute_array[index].href_attr);

                     if (strcmp(HOSTREF_DEFAULT, attr_hostname)) {
                        SGE_ADD_MSG_ID(sprintf(SGE_EVENT,
                                       MSG_CQUEUE_NONDEFNOTALLOWED));
                        answer_list_add(answer_list, SGE_EVENT,
                                        STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR); 
                        ret = false;
                     } else {
                        lSetHost(elem, cqueue_attribute_array[index].href_attr,
                                 sge_dstring_get_string(&host_domain));
                     }
                  }
               }
               index++;
            }
         }
         sge_dstring_free(&host_domain);
         sge_dstring_free(&cqueue_name);
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_priority(lListElem *cqueue, lList **answer_list, 
                       lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_priority");
   if (cqueue != NULL && attr_elem != NULL) {
      const char *priority_string = lGetString(attr_elem, ASTR_value);

      if (priority_string != NULL) {
         const int priority = atoi(priority_string);

         if (priority == 0 && priority_string[0] != '0') {
            answer_list_add(answer_list, MSG_CQUEUE_WRONGCHARINPRIO, 
                            STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            ret = false;
         } else if (priority < -20 || priority > 20 ) {
            answer_list_add(answer_list, MSG_CQUEUE_PRIORITYNOTINRANGE, 
                            STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_processors(lListElem *cqueue, lList **answer_list, 
                         lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_priority");
   if (cqueue != NULL && attr_elem != NULL) {
      const char *processors_string = lGetString(attr_elem, ASTR_value);

      if (processors_string != NULL) {
         lList *range_list = NULL;

         range_list_parse_from_string(&range_list, answer_list, 
                                      processors_string,
                                      JUST_PARSE, 0, INF_ALLOWED);
         /* EB: TODO: CLEANUP: range_list_parse_from_string should return error */
         if (*answer_list) {
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_pe_list(lListElem *cqueue, lList **answer_list, 
                       lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_pe_list");
   if (cqueue != NULL && attr_elem != NULL) {
      lList *pe_list = lGetList(attr_elem, ASTRLIST_value);

      if (pe_list != NULL) {
         const lList *master_list = *(pe_list_get_master_list());

         if (!pe_list_do_all_exist(master_list, answer_list, pe_list)) {
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_ckpt_list(lListElem *cqueue, lList **answer_list, 
                        lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_ckpt_list");
   if (cqueue != NULL && attr_elem != NULL) {
      lList *ckpt_list = lGetList(attr_elem, ASTRLIST_value);

      if (ckpt_list != NULL) {
         const lList *master_list = *(ckpt_list_get_master_list());

         if (!ckpt_list_do_all_exist(master_list, answer_list, ckpt_list)) {
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_user_list(lListElem *cqueue, lList **answer_list, 
                        lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_user_list");
   if (cqueue != NULL && attr_elem != NULL) {
      lList *user_list = lGetList(attr_elem, AUSRLIST_value);

      if (user_list != NULL) {
         if (userset_list_validate_acl_list(user_list, answer_list) == STATUS_EUNKNOWN) {
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_verify_project_list(lListElem *cqueue, lList **answer_list,
                           lListElem *attr_elem)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_verify_project_list");
   if (cqueue != NULL && attr_elem != NULL) {
      lList *project_list = lGetList(attr_elem, APRJLIST_value);

      if (project_list != NULL) {
         const lList *master_list = *(prj_list_get_master_list());

         if (!prj_list_do_all_exist(master_list, answer_list, project_list)) {
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_verify_subordinate_list(lListElem *cqueue, lList **answer_list,
                               lListElem *attr_elem)
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_verify_subordinate_list");
   if (cqueue != NULL && attr_elem != NULL) {
      const lList *master_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
      const char *cqueue_name = lGetString(cqueue, CQ_name);
      lList *so_list = lGetList(attr_elem, ASOLIST_value);
      lListElem *so;

      for_each(so, so_list) {
         const char *so_name = lGetString(so, SO_name);
  
         /*
          * Check for recursions to ourself
          */ 
         if (strcmp(cqueue_name, so_name) != 0) {
            const lListElem *cqueue = NULL;

            /*
             * Check if cqueue exists
             */
            cqueue = cqueue_list_locate(master_list, so_name);
            if (cqueue != NULL) {
               /*
                * Success
                */
               ;
            } else {
               ERROR((SGE_EVENT, MSG_CQUEUE_UNKNOWNSUB_SS, 
                      so_name, cqueue_name));
               answer_list_add(answer_list, SGE_EVENT, 
                               STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
               ret = false;
            }
         } else {
            ERROR((SGE_EVENT, MSG_CQUEUE_SUBITSELF_S, cqueue_name));
            answer_list_add(answer_list, SGE_EVENT, 
                            STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_calendar(lListElem *cqueue, lList **answer_list, 
                       lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_calendar");
   if (cqueue != NULL && attr_elem != NULL) {
      const char *name = lGetString(attr_elem, ASTR_value);

      if (name != NULL && strcasecmp("none", name)) {
         lListElem *calendar = calendar_list_locate(Master_Calendar_List, name);

         if (calendar == NULL) {
            sprintf(SGE_EVENT, MSG_CQUEUE_UNKNOWNCALENDAR_S, name);
            answer_list_add(answer_list, SGE_EVENT, 
                            STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_initial_state(lListElem *cqueue, lList **answer_list, 
                            lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_initial_state");
   if (cqueue != NULL && attr_elem != NULL) {
      const char *names[] = {"default", "enabled", "disabled", NULL};
      const char *name = lGetString(attr_elem, ASTR_value);
      bool found = false;
      int i = 0;

      while (names[i] != NULL) {
         if (!strcasecmp(name, names[i])) {
            found = true;
         }
         i++;
      }
      if (!found) {
         sprintf(SGE_EVENT, MSG_CQUEUE_UNKNOWNINITSTATE_S, name);
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         ret = false;
      }
   }
   DEXIT;
   return ret;
}

bool 
cqueue_verify_shell_start_mode(lListElem *cqueue, lList **answer_list, 
                               lListElem *attr_elem)
{
   bool ret = true;
   
   DENTER(CQUEUE_LAYER, "cqueue_verify_shell_start_mode");
   if (cqueue != NULL && attr_elem != NULL) {
      const char *names[] = {
         "unix_behavior", "posix_compliant", "script_from_stdin", 
         NULL
      };
      const char *name = lGetString(attr_elem, ASTR_value);
      bool found = false;
      int i = 0;

      while (names[i] != NULL) {
         if (!strcasecmp(name, names[i])) {
            found = true;
         }
         i++;
      }
      if (!found) {
         sprintf(SGE_EVENT, MSG_CQUEUE_UNKNOWNSTARTMODE_S, name);
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         ret = false;
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_list_find_hgroup_references(const lList *this_list, lList **answer_list,
                                   const lListElem *hgroup, lList **string_list)
{
   bool ret = true;
   lListElem *cqueue;

   DENTER(CQUEUE_LAYER, "cqueue_list_find_hgroup_references");
   if (this_list != NULL && hgroup != NULL && string_list != NULL) {
      for_each(cqueue, this_list) {
         if (cqueue_is_hgroup_referenced(cqueue, hgroup)) {
            const char *name = lGetString(cqueue, CQ_name);

            lAddElemStr(string_list, ST_name, name, ST_Type);
         }
      }
   }
   DEXIT;
   return ret;
}

void
cqueue_list_set_tag(lList *this_list, u_long32 tag_value, bool tag_qinstances)
{
   DENTER(TOP_LAYER, "cqueue_list_set_tag");
   if (this_list != NULL) {
      lListElem *cqueue = NULL;

      for_each(cqueue, this_list) {
         lSetUlong(cqueue, CQ_tag, tag_value);
         if (tag_qinstances) {
            lList *qinstance_list = lGetList(cqueue, CQ_qinstances);

            qinstance_list_set_tag(qinstance_list, tag_value);
         }
      }
   }
   DEXIT;
}

lListElem *
cqueue_list_locate_qinstance(lList *cqueue_list, const char *full_name) 
{
   lListElem *ret = NULL;
   lListElem *cqueue = NULL;
   lList *qinstance_list = NULL;
   dstring cqueue_name_buffer = DSTRING_INIT;
   dstring host_domain_buffer = DSTRING_INIT;
   const char *cqueue_name = NULL;
   const char *hostname = NULL;
   bool has_hostname = false;
   bool has_domain = false;

   DENTER(TOP_LAYER, "cqueue_list_locate_qinstance");
   cqueue_name_split(full_name, &cqueue_name_buffer, 
                     &host_domain_buffer, &has_hostname, &has_domain);
   cqueue_name = sge_dstring_get_string(&cqueue_name_buffer);
   hostname = sge_dstring_get_string(&host_domain_buffer);
   DPRINTF(("CQ: "SFN"\n", cqueue_name));
   if (has_hostname) {
      DPRINTF(("hostname: "SFN"\n", hostname));
   } else if (has_domain) {
      DPRINTF(("domain: "SFN"\n", hostname));
   } else {
      DPRINTF(("no host or domain\n"));
   }
   cqueue = lGetElemStr(cqueue_list, CQ_name, cqueue_name);
   qinstance_list = lGetList(cqueue, CQ_qinstances);
   ret = lGetElemHost(qinstance_list, QU_qhostname, hostname);
   sge_dstring_free(&cqueue_name_buffer);
   sge_dstring_free(&host_domain_buffer);
   DEXIT;
   return ret;
}

bool
cqueue_find_used_href(lListElem *this_elem, lList **answer_list, 
                      lList **href_list) 
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_find_used_href");
   if (this_elem != NULL) {
      int index=0;

      while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
         int pos = lGetPosViaElem(this_elem,
                            cqueue_attribute_array[index].cqueue_attr);

         if (pos >= 0) {
            lList *list = lGetPosList(this_elem, pos);
            lListElem *elem = NULL;

            for_each(elem, list) {
               const char *attr_hostname = lGetHost(elem, 
                                 cqueue_attribute_array[index].href_attr);

               ret = href_list_add(href_list, answer_list, attr_hostname);
            }
         }
         index++;
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_trash_used_href_setting(lListElem *this_elem, lList **answer_list, 
                               const char *hgroup_or_hostname) 
{
   bool ret = true;

   DENTER(CQUEUE_LAYER, "cqueue_trash_used_href_setting");
   if (this_elem != NULL) {
      int index=0;

      while (cqueue_attribute_array[index].cqueue_attr != NoName && ret) {
         int pos = lGetPosViaElem(this_elem,
                            cqueue_attribute_array[index].cqueue_attr);

         if (pos >= 0) {
            lList *list = lGetPosList(this_elem, pos);
            lListElem *elem = NULL;
            lListElem *next_elem = NULL;

            next_elem = lFirst(list);
            while ((elem = next_elem) != NULL) {
               const char *attr_hostname = lGetHost(elem, 
                                 cqueue_attribute_array[index].href_attr);

               next_elem = lNext(elem);

               if (!sge_hostcmp(hgroup_or_hostname, attr_hostname)) {
                  lRemoveElem(list, elem);
               }
            }
         }
         index++;
      }
   }
   DEXIT;
   return ret;
}
