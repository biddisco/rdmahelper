// Copyright (c) 2014-2015 John Biddiscombe
// Copyright (c) 2011,2012 IBM Corp.
//
// ================================================================
// Portions of this code taken from IBM BlueGene-Q source
// 
// This software is available to you under the
// Eclipse Public License (EPL).
//
// Please refer to the file "eclipse-1.0.txt" 
// ================================================================
//
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

//! \file  SystemLock.h
//! \brief Declaration and inline methods for SystemLock class.

#ifndef SYSTEMLOCK_H
#define SYSTEMLOCK_H

// Includes
#include <sys/stat.h>
#include <sys/sem.h>
#include <errno.h>
#include <memory>

#ifndef SEMUN
#define SEMUN
//! Optional fourth argument for semctl().
union semun {
   int val;                  //!< Value for SETVAL operation.
   struct semid_ds *buf;     //!< Buffer for IPC_STAT and IPC_SET operations.
   unsigned short *array;    //!< Array for GETALL and SETALL operations.
};
#endif

namespace bgcios
{

//! Key for lock to serialize allocating memory regions.
const int AllocateMemoryRegionKey = 42042;

//! \brief  Lock for synchronization between processes on an I/O node.
//!
//! This class implements a lock for controlling access to a resource between processes
//! using IPC semaphore sets.

class SystemLock
{
public:

   //! \brief  Default constructor.
   //! \param  key Key that identifies the semaphore set.

   SystemLock(int key)
   {
      // Create a semaphore set with one semaphore for the lock.
      _semid = semget(key, 1, IPC_CREAT|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
      _semnum = 0;
   }

   //! \brief  Obtain the lock.
   //! \return 0 when successful, errno when unsuccessful.

   int lock(void)
   {
      struct sembuf operation;
      operation.sem_num = _semnum;
      operation.sem_flg = SEM_UNDO;
      operation.sem_op = -1;
      int rc = semop(_semid, &operation, 1);
      if (rc != 0) rc = errno;
      return rc;
   }

   //! \brief  Release the lock.
   //! \return 0 when successful, errno when unsuccessful.

   int unlock(void)
   {
      struct sembuf operation;
      operation.sem_num = _semnum;
      operation.sem_flg = SEM_UNDO;
      operation.sem_op = 1;
      int rc = semop(_semid, &operation, 1);
      if (rc != 0) rc = errno;
      return rc;
   }

   //! \brief  Get the current value of the lock.
   //! \return Current value.

   int getValue(void)
   {
      return semctl(_semid, _semnum, GETVAL);
   }

   //! \brief  Set the value of the lock to a specific value.
   //! \param  value New value of lock.
   //! \return 0 when successful, errno when unsuccessful.

   int setValue(int value)
   {
      union semun arg;
      arg.val = value;
      int rc = semctl(_semid, _semnum, SETVAL, arg);
      if (rc != 0) rc = errno;
      return rc;
   }

private:

   int _semid;               //!< Identifier of semaphore set.
   int _semnum;              //!< Number of the semaphore in the set.
};

//! Smart pointer for SystemLock object.
typedef std::shared_ptr<SystemLock> SystemLockPtr;

} // namespace bgcios

#endif // SYSTEMLOCK_H

