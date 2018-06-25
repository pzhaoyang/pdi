/******************************************************************************
* Copyright (c) 2008 Redback Networks, Inc. All rights reserved.
* This software is the confidential and proprietary information of
* Redback Networks Inc.
*
* Description:
*
* The mutex semaphore interface for process debug infrastructure (PDI).
******************************************************************************/

#include "pdi_internal.h"
#include <pthread.h>

STATUS pdi_sem_create(pthread_mutex_t *mutex_p, int options);

STATUS pdi_sem_take (pthread_mutex_t *mutex_p);

STATUS pdi_sem_give (pthread_mutex_t *mutex_p);

