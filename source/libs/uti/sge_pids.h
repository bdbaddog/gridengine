#ifndef __SGE_PIDS_H
#define __SGE_PIDS_H
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

#if defined(SUN4) || defined(LINUX) || defined(DARWIN) || defined(FREEBSD) || defined(NETBSD)
#  define PSCMD "/bin/ps -axc"
#elif defined(ALPHA)
#  define PSCMD "/bin/ps axo pid,ucomm"
#elif defined(SOLARIS)
#  define PSCMD "/bin/ps -eo pid,fname"
#else
#  define PSCMD "/bin/ps -e"
#endif

int get_pids(pid_t *, int, const char *, const char *);
int contains_pid(pid_t, pid_t *, int);
int checkprog(pid_t, const char *, const char *);
 
#endif /* __SGE_PIDS_H */
