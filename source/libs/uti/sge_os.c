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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>   
#include <sys/time.h>
#include <sys/resource.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#ifdef SIGTSTP
#   include <sys/file.h>
#endif
 
#if defined(SOLARIS)
#   include <sys/termios.h>
#endif
 
#if defined(__sgi) || defined(ALPHA)
#   include <rpcsvc/ypclnt.h>
#endif
 
#if defined(AIX)
#   include <sys/select.h>
#endif    

#include "sgermon.h"
#include "sge_unistd.h"
#include "sge_stdio.h"
#include "sge_os.h"
#include "sge_prog.h"  
#include "sge_log.h"
#include "sig_handlers.h"

#include "msg_common.h"
#include "msg_utilib.h"

/* pipe for sge_daemonize_prepare() and sge_daemonize_finalize() */
static int fd_pipe[2];

/****** sge_os/sge_get_pids() *************************************************
*  NAME
*     sge_get_pids() -- Return all "pids" of a running processes 
*
*  SYNOPSIS
*     int sge_get_pids(pid_t *pids, int max_pids, const char *name, 
*                      const char *pscommand) 
*
*  FUNCTION
*     Return all "pids" of a running processes with given "name". 
*     Only first 8 characters of "name" are significant.
*     Checks only basename of command after "/".
*
*  INPUTS
*     pid_t *pids           - pid array
*     int max_pids          - size of pid array
*     const char *name      - name 
*     const char *pscommand - ps commandline
*
*  RESULT
*     int - Result 
*         0 - No program with given name found
*        >0 - Number of processes with "name" 
*        -1 - Error
*
*  NOTES
*     MT-NOTES: sge_get_pids() is not MT safe
******************************************************************************/
int sge_get_pids(pid_t *pids, int max_pids, const char *name, 
             const char *pscommand) 
{
   FILE *fp_in, *fp_out, *fp_err;
   char buf[10000], *ptr;
   int num_of_pids = 0, last, len;
   pid_t pid, command_pid;

   DENTER(TOP_LAYER, "sge_get_pids");
   
   command_pid = sge_peopen("/bin/sh", 0, pscommand, NULL, NULL, 
                        &fp_in, &fp_out, &fp_err, false);

   if (command_pid == -1) {
      DEXIT;
      return -1;
   }

   while (!feof(fp_out) && num_of_pids<max_pids) {
      if ((fgets(buf, sizeof(buf), fp_out))) {
         if ((len = strlen(buf))) {

            /* handles first line of ps command */
            if ((pid = (pid_t) atoi(buf)) <= 0)
               continue;

            /* strip off trailing white spaces */
            last = len - 1;
            while (last >= 0 && isspace((int) buf[last])) {
               buf[last] = '\0';
               last--;
            }
            
            /* set pointer to first character of process name */
            while (last >= 0 && !isspace((int) buf[last]))
               last--;
            last++;

            /* DPRINTF(("pid: %d - progname: >%s<\n", pid, &buf[last])); */
            
            /* get basename of program */
            ptr = strrchr(&buf[last], '/');
            if (ptr)
               ptr++;
            else
               ptr = &buf[last];                  
   
            /* check if process has given name */
            if (!strncmp(ptr, name, 8))
               pids[num_of_pids++] = pid;
         }
      }
   }            

   sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);
   return num_of_pids;
}

/****** sge_os/sge_contains_pid() *********************************************
*  NAME
*     sge_contains_pid() -- Checks whether pid array contains pid 
*
*  SYNOPSIS
*     int sge_contains_pid(pid_t pid, pid_t *pids, int npids) 
*
*  FUNCTION
*     whether pid array contains pid 
*
*  INPUTS
*     pid_t pid   - process id 
*     pid_t *pids - pid array 
*     int npids   - number of pids in array 
*
*  RESULT
*     int - result state
*         0 - pid was not found
*         1 - pid was found
*
*  NOTES
*     MT-NOTES: sge_contains_pid() is MT safe
******************************************************************************/
int sge_contains_pid(pid_t pid, pid_t *pids, int npids) 
{
   int i;

   for (i = 0; i < npids; i++) {
      if (pids[i] == pid) {
         return 1;
      }
   }
   return 0;
}

/****** sge_os/sge_checkprog() ************************************************
*  NAME
*     sge_checkprog() -- Has "pid" of a running process the given "name" 
*
*  SYNOPSIS
*     int sge_checkprog(pid_t pid, const char *name, 
*                       const char *pscommand) 
*
*  FUNCTION
*     Check if "pid" of a running process has given "name".
*     Only first 8 characters of "name" are significant.
*     Check only basename of command after "/". 
*
*  INPUTS
*     pid_t pid             - process id 
*     const char *name      - process name 
*     const char *pscommand - ps commandline 
*
*  RESULT
*     int - result state
*         0 - Process with "pid" has "name"
*         1 - No such pid or pid has other name
*        -1 - error occurred (mostly sge_peopen() failed) 
*
*  NOTES
*     MT-NOTES: sge_checkprog() is not MT safe
******************************************************************************/
int sge_checkprog(pid_t pid, const char *name, const char *pscommand) 
{
   FILE *fp_in, *fp_out, *fp_err;
   char buf[1000], *ptr;
   pid_t command_pid, pidfound;
   int len, last, notfound;

   DENTER(TOP_LAYER, "sge_checkprog");

   command_pid = sge_peopen("/bin/sh", 0, pscommand, NULL, NULL, 
                        &fp_in, &fp_out, &fp_err, false);

   if (command_pid == -1) {
      DEXIT;
      return -1;
   }

   notfound = 1;
   while (!feof(fp_out)) {
      if ((fgets(buf, sizeof(buf), fp_out))) {
         if ((len = strlen(buf))) {
            pidfound = (pid_t) atoi(buf);

            if (pidfound == pid) {
               last = len - 1;
               DPRINTF(("last pos in line: %d\n", last));
               while (last >= 0 && isspace((int) buf[last])) {
                  buf[last] = '\0';
                  last--;
               }

               /* DPRINTF(("last pos in line now: %d\n", last)); */
               
               while (last >= 0 && !isspace((int) buf[last]))
                  last--;
               last++;

               /* DPRINTF(("pid: %d - progname: >%s<\n", pid, &buf[last])); */ 

               /* get basename of program */
               ptr = strrchr(&buf[last], '/');
	       if (ptr)
	          ptr++;
	       else
	          ptr = &buf[last];

               if (!strncmp(ptr, name, 8)) {
                  notfound = 0;
                  break;
               }
               else
                  break;
            }
         }
      }
   }

   sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);

   DEXIT;
   return notfound;
}

/****** sge_os/sge_daemonize_prepare() *****************************************
*  NAME
*     sge_daemonize_prepare() -- prepare daemonize of process
*
*  SYNOPSIS
*     int sge_daemonize_prepare(void) 
*
*  FUNCTION
*     The parent process will wait for the child's successful daemonizing.
*     The client process will report successful daemonizing by a call to
*     sge_daemonize_finalize().
*     The parent process will exit with one of the following exit states:
*
*     typedef enum uti_deamonize_state_type {
*        SGE_DEAMONIZE_OK           = 0,  ok 
*        SGE_DAEMONIZE_DEAD_CHILD   = 1,  child exited before sending state 
*        SGE_DAEMONIZE_TIMEOUT      = 2   timeout whild waiting for state 
*     } uti_deamonize_state_t;
*
*     Daemonize the current application. Throws ourself into the
*     background and dissassociates from any controlling ttys.
*     Don't close filedescriptors mentioned in 'keep_open'.
*      
*     sge_daemonize_prepare() and sge_daemonize_finalize() will replace
*     sge_daemonize() for multithreaded applications.
*     
*     sge_daemonize_prepare() must be called before starting any thread. 
*
*
*  INPUTS
*     void - none
*
*  RESULT
*     int - true on success, false on error
*
*  SEE ALSO
*     sge_os/sge_daemonize_finalize()
*******************************************************************************/
int sge_daemonize_prepare(void) {
   pid_t pid;
   fd_set keep_open;
#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX) || defined(INTERIX))
   int fd;
#endif

#if defined(__sgi) || defined(ALPHA) || defined(HP1164)
#  if defined(ALPHA)
   extern int getdomainname(char *, int);
#  endif
   char domname[256];
#endif

   DENTER(TOP_LAYER, "sge_daemonize_prepare");

#ifndef NO_SGE_COMPILE_DEBUG
   if (TRACEON) {
      DEXIT;
      return false;
   }
#endif

   if (uti_state_get_daemonized()) {
      DEXIT;
      return true;
   }

   /* create pipe */
   if ( pipe(fd_pipe) < 0) {
      CRITICAL((SGE_EVENT, MSG_UTI_DAEMONIZE_CANT_PIPE));
      DRETURN(false);
   }

   if ( fcntl(fd_pipe[0], F_SETFL, O_NONBLOCK) != 0) {
      CRITICAL((SGE_EVENT, MSG_UTI_DAEMONIZE_CANT_FCNTL_PIPE));
      DRETURN(false);
   }

   /* close all fd's expect pipe and first 3 */
   FD_ZERO(&keep_open);
   FD_SET(0,&keep_open);
   FD_SET(1,&keep_open);
   FD_SET(2,&keep_open);
   FD_SET(fd_pipe[0],&keep_open);
   FD_SET(fd_pipe[1],&keep_open);
   sge_close_all_fds(&keep_open);

   /* first fork */
   pid=fork();
   if (pid <0) {
      CRITICAL((SGE_EVENT, MSG_PROC_FIRSTFORKFAILED_S , strerror(errno)));
      DEXIT;
      exit(1);
   }

   if ( pid > 0) {
      char line[256];
      int line_p = 0;
      int retries = 60;
      int exit_status = SGE_DAEMONIZE_TIMEOUT;
      int back;
      int errno_value = 0;

      /* close send pipe */
      close(fd_pipe[1]);

      /* check pipe for message from child */
      while (line_p < 4 && retries-- > 0) {
         errno = 0;
         back = read(fd_pipe[0], &line[line_p], 1);
         errno_value = errno;
         if (back > 0) {
            line_p++;
         } else {
            if (back != -1) {
               if (errno_value != EAGAIN ) {
                  retries=0;
                  exit_status = SGE_DAEMONIZE_DEAD_CHILD;
               }
            }
            DPRINTF(("back=%d errno=%d\n",back,errno_value));
            sleep(1);
         }
      }
         
      if (line_p >= 4) {
         line[3] = 0;
         exit_status = atoi(line);
         DPRINTF(("received: \"%d\"\n", exit_status));
      }

      switch(exit_status) {
         case SGE_DEAMONIZE_OK:
            INFO((SGE_EVENT, MSG_UTI_DAEMONIZE_OK));
            break;
         case SGE_DAEMONIZE_DEAD_CHILD:
            WARNING((SGE_EVENT, MSG_UTI_DAEMONIZE_DEAD_CHILD));
            break;
         case SGE_DAEMONIZE_TIMEOUT:
            WARNING((SGE_EVENT, MSG_UTI_DAEMONIZE_TIMEOUT));
            break;
      }
      /* close read pipe */
      close(fd_pipe[0]);
      DEXIT;
      exit(exit_status); /* parent exit */
   }

   /* child */
   SETPGRP;

#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX) || defined(INTERIX))
   if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
      /* disassociate contolling tty */
      ioctl(fd, TIOCNOTTY, (char *) NULL);
      close(fd);
   }
#endif
   

   /* second fork */
   pid=fork();
   if (pid < 0) {
      CRITICAL((SGE_EVENT, MSG_PROC_SECONDFORKFAILED_S , strerror(errno)));
      DEXIT;
      exit(1);
   }
   if ( pid > 0) {
      /* close read and write pipe for second child and exit */
      close(fd_pipe[0]);
      close(fd_pipe[1]);
      exit(0);
   }

   /* child of child */

   /* close read pipe */
   close(fd_pipe[0]);

#if defined(__sgi) || defined(ALPHA) || defined(HP1164)
   /* The yp library may have open sockets
      when closing all fds also the socket fd of the yp library gets closed
      when called again yp library functions are confused since they
      assume fds are already open. Thus we shutdown the yp library regularly
      before closing all sockets */
   getdomainname(domname, sizeof(domname));
   yp_unbind(domname);
#endif

   
   DRETURN(true);
}

/****** sge_os/sge_daemonize_finalize() ****************************************
*  NAME
*     sge_daemonize_finalize() -- finalize daemonize process
*
*  SYNOPSIS
*     int sge_daemonize_finalize(fd_set *keep_open) 
*
*  FUNCTION
*     report successful daemonizing to the parent process and close
*     all file descriptors. Set file descirptors 0, 1 and 2 to /dev/null 
*
*     sge_daemonize_prepare() and sge_daemonize_finalize() will replace
*     sge_daemonize() for multithreades applications.
*
*     sge_daemonize_finalize() must be called by the thread who have called
*     sge_daemonize_prepare().
*
*  INPUTS
*     fd_set *keep_open - file descriptor set to keep open
*
*  RESULT
*     int - true on success
*
*  SEE ALSO
*     sge_os/sge_daemonize_prepare()
*******************************************************************************/
int sge_daemonize_finalize(void) {
   char tmp_buffer[4];

   DENTER(TOP_LAYER, "sge_daemonize_finalize");

   /* don't call this function twice */
   if (uti_state_get_daemonized()) {
      DEXIT;
      return true;
   }

   /* The response id has 4 byte, send it to father process */
   snprintf(tmp_buffer, 4, "%3d", SGE_DEAMONIZE_OK );
   write(fd_pipe[1], tmp_buffer, 4);

   sleep(2); /* give father time to read the status */

   /* close write pipe */
   close(fd_pipe[1]);

   /* close first three file descriptors */
#ifndef __INSURE__
   close(0);
   close(1);
   close(2);
   
   /* new descriptors acquired for stdin, stdout, stderr should be 0,1,2 */
   if (open("/dev/null",O_RDONLY,0)!=0) {
      SGE_EXIT(0);
   }
   if (open("/dev/null",O_WRONLY,0)!=1) {
      SGE_EXIT(0);
   }
   if (open("/dev/null",O_WRONLY,0)!=2) {
      SGE_EXIT(0);
   }
#endif

   SETPGRP;

   /* now have finished daemonizing */
   uti_state_set_daemonized(1);

   DRETURN(true);
}





/****** sge_os/sge_daemonize() ************************************************
*  NAME
*     sge_daemonize() -- Daemonize the current application
*
*  SYNOPSIS
*     int sge_daemonize(fd_set *keep_open)
*
*  FUNCTION
*     Daemonize the current application. Throws ourself into the
*     background and dissassociates from any controlling ttys.
*     Don't close filedescriptors mentioned in 'keep_open'.
*
*  INPUTS
*     fd_set *keep_open - bitmask
*
*  RESULT
*     int - Successfull?
*         1 - Yes
*         0 - No
*
*  NOTES
*     MT-NOTES: sge_daemonize() is not MT safe
******************************************************************************/
int sge_daemonize(fd_set *keep_open)
{
#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX) || defined(INTERIX))
   int fd;
#endif
 
#if defined(__sgi) || defined(ALPHA) || defined(HP1164)
#  if defined(ALPHA)
   extern int getdomainname(char *, int);
#  endif
   char domname[256];
#endif
   pid_t pid;
   int failed_fd;
 
   DENTER(TOP_LAYER, "sge_daemonize");
 
#ifndef NO_SGE_COMPILE_DEBUG
   if (TRACEON) {
      DEXIT;
      return 0;
   }
#endif
 
   if (uti_state_get_daemonized()) {
      DEXIT;
      return 1;
   }
 
   if ((pid=fork())!= 0) {             /* 1st child not pgrp leader */
      if (pid<0) {
         CRITICAL((SGE_EVENT, MSG_PROC_FIRSTFORKFAILED_S , strerror(errno)));
      }
      exit(0);
   }
 
   SETPGRP;                      
 
#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX) || defined(INTERIX))
   if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
      /* disassociate contolling tty */
      ioctl(fd, TIOCNOTTY, (char *) NULL);
      close(fd);
   }
#endif
 
   if ((pid=fork())!= 0) {
      if (pid<0) {
         CRITICAL((SGE_EVENT, MSG_PROC_SECONDFORKFAILED_S , strerror(errno)));
      }
      exit(0);
   }
 
#if defined(__sgi) || defined(ALPHA) || defined(HP1164)
   /* The yp library may have open sockets
      when closing all fds also the socket fd of the yp library gets closed
      when called again yp library functions are confused since they
      assume fds are already open. Thus we shutdown the yp library regularly
      before closing all sockets */
  getdomainname(domname, sizeof(domname));
  yp_unbind(domname);
#endif
 
   /* close all file descriptors */
   sge_close_all_fds(keep_open);
 
   /* new descriptors acquired for stdin, stdout, stderr should be 0,1,2 */
   failed_fd = sge_occupy_first_three();
   if (failed_fd  != -1) {
      CRITICAL((SGE_EVENT, MSG_CANNOT_REDIRECT_STDINOUTERR_I, failed_fd));
      SGE_EXIT(0);
   }

   SETPGRP;
 
   uti_state_set_daemonized(1);
 
   DEXIT;
   return 1;
}     

/****** sge_os/redirect_to_dev_null() ******************************************
*  NAME
*     redirect_to_dev_null() -- redirect a channel to /dev/null
*
*  SYNOPSIS
*     int redirect_to_dev_null(int target, int mode) 
*
*  FUNCTION
*     Attaches a certain filedescriptor to /dev/null.
*
*  INPUTS
*     int target - file descriptor
*     int mode   - mode for open
*
*  RESULT
*     int - target fd number if everything was ok,
*           else -1
*
*  NOTES
*     MT-NOTE: redirect_to_dev_null() is MT safe 
*
*******************************************************************************/
int redirect_to_dev_null(int target, int mode)
{
   SGE_STRUCT_STAT buf;

   if (SGE_FSTAT(target, &buf)) {
      if ((open("/dev/null", mode, 0)) != target) {
         return target;
      }
   }

   return -1;
}

/****** sge_os/sge_occupy_first_three() ***************************************
*  NAME
*     sge_occupy_first_three() -- Open descriptor 0, 1, 2 to /dev/null
*
*  SYNOPSIS
*     int sge_occupy_first_three(void)
*
*  FUNCTION
*     Occupy the first three filedescriptors, if not available. This is done
*     to be sure that a communication by a socket will not get any "forgotten"
*     print output from code.
*
*  RESULT
*     int - error state
*        -1 - OK
*         0 - there are problems with stdin
*         1 - there are problems with stdout
*         2 - there are problems with stderr
*
*  NOTES
*     MT-NOTE: sge_occupy_first_three() is MT safe
*
*  SEE ALSO
*     sge_os/redirect_to_dev_null()
*     sge_os/sge_close_all_fds()
******************************************************************************/
int sge_occupy_first_three(void)
{
   int ret = -1;

   DENTER(TOP_LAYER, "occupy_first_three");

   ret = redirect_to_dev_null(0, O_RDONLY);

   if (ret == -1) {
      ret = redirect_to_dev_null(1, O_WRONLY);
   }

   if (ret == -1) {
      ret = redirect_to_dev_null(2, O_WRONLY);
   }

   DEXIT;
   return ret;
}  

/****** sge_os/sge_close_all_fds() ********************************************
*  NAME
*     sge_close_all_fds() -- close (all) file descriptors
*
*  SYNOPSIS
*     void sge_close_all_fds(fd_set *keep_open)
*
*  FUNCTION
*     Close all filedescriptors but ignore those mentioned
*     in 'keep_open'.
*
*  INPUTS
*     fd_set *keep_open - bitmask
*
*  NOTES
*     MT-NOTE: sge_close_all_fds() is MT safe
*
*  SEE ALSO
*     sge_os/sge_occupy_first_three()
******************************************************************************/
#ifdef __INSURE__
extern int _insure_is_internal_fd(int);
#endif

void sge_close_all_fds(fd_set *keep_open)
{
   int fd;
   int maxfd;
   bool ignore = false;

   DENTER(TOP_LAYER, "sge_close_all_fds");

#ifndef WIN32NATIVE
   maxfd = sysconf(_SC_OPEN_MAX) > FD_SETSIZE ? \
     FD_SETSIZE : sysconf(_SC_OPEN_MAX);
#else /* WIN32NATIVE */
   maxfd = FD_SETSIZE;
   /* detect maximal number of fds under NT/W2000 (env: Files)*/
#endif /* WIN32NATIVE */
 
   for (fd = 0; fd < maxfd; fd++) {
      ignore = false;

      if (keep_open != NULL) {
         if (FD_ISSET(fd, keep_open)) {
            ignore = true;
         }
      }
#ifdef __INSURE__
      if (_insure_is_internal_fd(fd)) {
         WARNING((SGE_EVENT, "INSURE: fd %d will not be closed", fd));
         ignore = true;
      }
#endif

      if (ignore == false) {
#ifndef WIN32NATIVE
         close(fd);
#else /* WIN32NATIVE */
         closesocket(fd);
#endif /* WIN32NATIVE */
      }
   }

   DEXIT;
   return;
}  

