#ifndef __PDI_CLIENT_H__
#define __PDI_CLIENT_H__

#include "pdi.h"

int pdi_client_init(const char* host, unsigned int port);
int pdi_client_close();
int pdi_client_send_command(pdi_msg_t *msg);

#endif
