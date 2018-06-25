#ifndef __PDI_SEM_H__
#define __PDI_SEM_H__
#include <pthread.h>

#include "pdi_internal.h"

STATUS pdi_sem_create(pthread_mutex_t *mutex_p, int options);
STATUS pdi_sem_take (pthread_mutex_t *mutex_p);
STATUS pdi_sem_give (pthread_mutex_t *mutex_p);

#endif
