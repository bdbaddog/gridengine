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
package com.sun.grid.jgdi.event;

import java.util.Iterator;
import java.util.LinkedList;
import java.util.logging.Logger;


/**
 * Helper class for event test cases.
 *
 */
public class WaitForEventThread extends Thread implements EventListener {
   
   private Logger logger = Logger.getLogger("com.sun.grid.jgdi.event");
   private Object object;
   private LinkedList events = new LinkedList();
   
   public WaitForEventThread(Object object) {
      this.object = object;
   }
   
   /**
    *
    * @param evt the event
    */
   public void eventOccured(Event evt) {
      logger.fine("Got event " + evt + "(" + evt.getClass() + ")");
      synchronized(events) {
         events.add(evt);
         events.notifyAll();
      }
   }
   
   private Object addEventSync = new Object();
   private boolean hasAddEvent = false;
   
   public boolean waitForAddEvent(long timeout) throws InterruptedException {
      synchronized(addEventSync) {
         if (!hasAddEvent) {
            addEventSync.wait(timeout);
         }
      }
      return hasAddEvent;
   }
   
   private Object modEventSync = new Object();
   private boolean hasModEvent = false;
   
   public boolean waitForModEvent(long timeout) throws InterruptedException {
      synchronized(modEventSync) {
         if (!hasModEvent) {
            modEventSync.wait(timeout);
         }
      }
      return hasModEvent;
   }
   
   private Object delEventSync = new Object();
   private boolean hasDelEvent = false;
   
   public boolean waitForDelEvent(long timeout) throws InterruptedException {
      synchronized(delEventSync) {
         if (!hasDelEvent) {
            delEventSync.wait(timeout);
         }
      }
      return hasDelEvent;
   }
   
   public void run() {
      
      try {
         while (true) {
            Event evt = null;
            synchronized(events) {
               while (evt == null) {
                  while (events.isEmpty()) {
                     events.wait();
                  }
                  evt = (Event)events.removeFirst();
               }
            }
            
            if (evt.getType().equals(EventType.SGE_EMA_ADD)) {
               
               AddEvent addEvt = (AddEvent)evt;
               if (this.object.equals(addEvt.getChangedObject())) {
                  synchronized(addEventSync) {
                     hasAddEvent = true;
                     addEventSync.notifyAll();
                  }
               }
            } else if (evt.getType().equals(EventType.SGE_EMA_MOD)) {
               
               ModEvent modEvt = (ModEvent)evt;
               if (this.object.equals(modEvt.getChangedObject())) {
                  synchronized(modEventSync) {
                     hasModEvent = true;
                     modEventSync.notifyAll();
                  }
               }
               
            } else if (evt.getType().equals(EventType.SGE_EMA_DEL)) {
               
               DelEvent delEvt = (DelEvent)evt;
               if (delEvt.hasDeletedObject(object)) {
                  synchronized(delEventSync) {
                     hasDelEvent = true;
                     delEventSync.notifyAll();
                  }
               }
            }
            evt = null;
         }
      } catch (InterruptedException ire) {
         logger.fine("wait thread has been interrupted");
      }
   }
   
}
