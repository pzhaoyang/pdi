/******************************************************************************
* Copyright (c) 2008 Redback Networks, Inc. All rights reserved.
* This software is the confidential and proprietary information of
* Redback Networks Inc.
*
* Description:
*
* The mutex semaphore implementation for process debug infrastructure (PDI).
******************************************************************************/

#include "pdi_sem.h"

/******************************************************************************
* FUNCTION: pdi_sem_create - create a mutex semaphore
*
* RETURNS:  PDI_OK or PDI_ERROR
******************************************************************************/
STATUS pdi_sem_create(pthread_mutex_t *mutex_p, int options)
{
    pthread_mutexattr_t mutexattr;  /* Mutex attribute variable */

    /* Initialize the mutex */
    if (pthread_mutexattr_init(&mutexattr)) return (PDI_ERROR);

    /* Set the mutex as a recursive mutex */
    if (pthread_mutexattr_settype(&mutexattr, options)) return (PDI_ERROR);

    /* create the mutex with the attributes set */
    if (pthread_mutex_init(mutex_p, &mutexattr)) return (PDI_ERROR);

    /* After initializing the mutex, the thread attribute can be destroyed */
    if (pthread_mutexattr_destroy(&mutexattr)) return (PDI_ERROR);

    return (PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sem_take - take a mutex semaphore
*
* RETURNS:  PDI_OK or PDI_ERROR
******************************************************************************/
STATUS pdi_sem_take (pthread_mutex_t *mutex_p)
{
    /* Acquire the mutex to access the shared resource */
    if (pthread_mutex_lock (mutex_p)) return (PDI_ERROR);

    return (PDI_OK);
}

/******************************************************************************
* FUNCTION: pdi_sem_give - give a mutex semaphore
*
* RETURNS:  PDI_OK or PDI_ERROR
******************************************************************************/
STATUS pdi_sem_give (pthread_mutex_t *mutex_p)
{
    /* Release the mutex and release the access to shared resource */
    if (pthread_mutex_unlock (mutex_p)) return (PDI_ERROR);

    return (PDI_OK);
}

