/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/* ================================================================ */
/*                                                                  */
/* Licensed Materials - Property of IBM                             */
/*                                                                  */
/* Blue Gene/Q                                                      */
/*                                                                  */
/* (C) Copyright IBM Corp.  2011, 2012                              */
/*                                                                  */
/* US Government Users Restricted Rights -                          */
/* Use, duplication or disclosure restricted                        */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/* This software is available to you under the                      */
/* Eclipse Public License (EPL).                                    */
/*                                                                  */
/* ================================================================ */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */

//! \file  Thread.h
//! \brief Declaration and inline methods for bgcios::Thread class.

#ifndef COMMON_THREAD_H
#define COMMON_THREAD_H

// Includes
#include <pthread.h>
#include <signal.h>
#include <errno.h>

namespace bgcios
{

//! \brief Manage a secondary thread.

class Thread
{
public:

   //! \brief  Default constructor.

   Thread()
   {
      _threadId = 0;
      pthread_attr_init(&_attributes);
      pthread_attr_setdetachstate(&_attributes, PTHREAD_CREATE_DETACHED);
      _done = 0;
      _lastError = 0;
      _exitValue = NULL;
   }


   //! \brief  Default destructor.

   virtual ~Thread() { }

   //! \brief  Start the thread.
   //! \return 0 when successful, errno when unsuccessful.

   int start(void);

   //! \brief  Run the thread.
   //!         Each derived class provides a run() method to implement the function of the thread.
   //! \return Pointer to return value.

   virtual void * run(void) { return NULL; }

   //! \brief  Tell thread to stop.
   //! \return 0 when successful, errno when unsuccessful.

   int stop(void)
   {
      if (!isStarted()) {
         return ESRCH;
      }
      _done = 1;
      pthread_kill(_threadId, SIGUSR1); // Wakeup thread from a blocked system call.
      return 0;
   }

   //! \brief  Interrupt the thread.
   //! \note   A handler for signal (default SIGUSR2) should be installed in the process.
   //! \return 0 when successful, errno when unsuccessful.

   int interrupt(int signal=SIGUSR2)
   {
      if (!isStarted()) {
         return ESRCH;
      }
      pthread_kill(_threadId, signal); // Wakeup thread from a blocked system call.
      return 0;
   }

   //! \brief  Wait for thread id and reclaim resources.
   //! \return 0 when successful, errno when unsuccessful.

   int join(void)
   {
      if (!isStarted()) {
         return ESRCH;
      }
      return pthread_join(_threadId, &_exitValue);
   }

   //! \brief  Get the exit value of the thread.
   //! \return Exit value.

   void * getExitValue(void) const { return _exitValue; }

   //! \brief  Get the thread id.
   //! \return Thread id.

   pthread_t getThreadId(void) const { return _threadId; }

   //! \brief  Set the thread's detach state to detached.
   //! \return 0 when successful, errno when unsuccessful.

   int setDetached(void);

   //! \brief  Check if thread's detach state is detached.
   //! \return True if detach state is detached, otherwise false.

   bool isDetached(void);

   //! \brief  Set the thread's stack size.
   //! \param  stackSize Size of thread's stack in bytes.
   //! \return 0 when successful, errno when unsuccessful.

   int setStackSize(size_t stackSize);

   //! \brief  Get the thread's stack size.
   //! \return Size of thread's stack in bytes.

   size_t getStackSize(void);

   //! \brief  Check if thread is started.
   //! \return True if thread is started, otherwise false.

   bool isStarted(void) const { return _threadId != 0 ? true : false; }

   //! \brief  Check if thread is done.
   //! \return True if thread is done, otherwise false.

   bool isDone(void) const { return _done; }

   //! \brief  Get the last error recorded by thread.
   //! \return Last error value.

   int getLastError(void) const { return _lastError; }

protected:

   //! \brief  Thread start function.
   //! \param  p Pointer to opaque data.
   //! \return Pointer to return value or NULL.

   static void * startWrapper(void *p);

   //! Thread id.
   pthread_t _threadId;

   //! Thread attributes.
   pthread_attr_t _attributes;

   //! Indicator to stop processing in thread.
   volatile int _done;

   //! Last error recorded by thread.
   int _lastError;

   //! Exit value returned by thread.
   void *_exitValue;
};

} // namespace bgcios

#endif // COMMON_THREAD_H
