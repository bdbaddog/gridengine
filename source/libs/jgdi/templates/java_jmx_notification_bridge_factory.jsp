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
/**
 *  Generated from java_event_client.jsp
 *  !!! DO NOT EDIT THIS FILE !!!
 */
<%
  com.sun.grid.cull.JavaHelper jh = (com.sun.grid.cull.JavaHelper)params.get("javaHelper");
  com.sun.grid.cull.CullDefinition cullDef = (com.sun.grid.cull.CullDefinition)params.get("cullDef");
%>
package com.sun.grid.jgdi.management;

import javax.management.MBeanNotificationInfo;
import com.sun.grid.jgdi.EventClient;
import java.util.*;

/** 
 *  Helper class for mapping JGDI Events to JMX notifications
 *
 */
public class NotificationBridgeFactory {

    public static NotificationBridge newInstance(String url) {
    
        NotificationBridge ret = new NotificationBridge(url);
    
<%
    
    java.util.Iterator iter = cullDef.getObjectNames().iterator();
    com.sun.grid.cull.CullObject cullObj = null;
    String name = null;

    while( iter.hasNext() ) {
      name = (String)iter.next();
      cullObj = cullDef.getCullObject(name); 
      
      name = cullObj.getIdlName();
      
//      System.out.println("name = " + name + ", cullname = " + cullObj.getName() + " " + 
//                         (cullObj.hasAddEvent() ? "A" : "") + 
//                         (cullObj.hasDeleteEvent() ? "D" : "") +
//                         (cullObj.hasGetListEvent() ? "L" : "") +
//                         (cullObj.hasGetEvent() ? "G" : "") +
//                         (cullObj.hasModifyEvent() ? "M" : ""));
      
      if(name == null) {
         throw new IllegalStateException("Have no idl name for " + cullObj.getName());
      }
      String notifType = null;
      String classname = null;
      if(cullObj.hasAddEvent()) {
%>
      ret.registerEvent("Add<%=name%>", com.sun.grid.jgdi.event.<%=cullObj.getIdlName()%>AddEvent.class);
<%    } 
      
      if(cullObj.hasDeleteEvent()) {
%>
      ret.registerEvent("Del<%=name%>", com.sun.grid.jgdi.event.<%=cullObj.getIdlName()%>DelEvent.class);
<%
      }
      if(cullObj.hasModifyEvent()) {
%>
      ret.registerEvent("Mod<%=name%>", com.sun.grid.jgdi.event.<%=cullObj.getIdlName()%>ModEvent.class);
<%
      }
      if(cullObj.hasGetListEvent()) {
%>
      ret.registerEvent("List<%=name%>", com.sun.grid.jgdi.event.<%=cullObj.getIdlName()%>ListEvent.class);
<%
      }
   } // end of while 

   String [] specialEvents = {
       "QmasterGoesDown",
       "SchedulerRun",
       "Shutdown",
       "JobFinish",
       "JobUsage",
       "JobFinalUsage",
       "JobPriorityMod",
       "QueueInstanceSuspend",
       "QueueInstanceUnsuspend"
   };
   
   for(int i = 0; i < specialEvents.length; i++) {

%>
      ret.registerEvent("<%=specialEvents[i]%>", com.sun.grid.jgdi.event.<%=specialEvents[i]%>Event.class);
<%
   } // end of for special events
%>
      return ret;
   }
}
