#ifndef __PDI_SERVER_H__
#define __PDI_SERVER_H__
#include <inttypes.h>
#include <unistd.h>

#include "pdi.h"


#define SHOW_INT                "%-35.35s: %" PRIu64 "\n"
#define SHOW_INT_IND            "  %-33.35s: %" PRIu64 "\n"
#define SHOW_INT_2XIND          "    %-31.35s: %" PRIu64 "\n"
#define SHOW_HEX                "%-35.35s: %#" PRIx64 "\n"
#define SHOW_HEX_IND            "  %-33.35s: %#" PRIx64 "\n"
#define SHOW_STR                "%-35.35s: %-43.43s\n"
#define SHOW_STR_IND            "  %-33.35s: %-43.43s\n"
#define SHOW_STR_2XIND          "    %-31.35s: %-43.43s\n"
#define SHOW_PTR                "%-35.35s: %p\n"
#define SHOW_PTR_IND            "  %-33.35s: %p\n"
#define SHOW_3_STR              "%-35.35s: %14s%14s%14s\n"
#define SHOW_3_INT              "%-35.35s: %14" PRIu64 "%14" PRIu64 "%14" PRIu64 "\n"

#define SEPARATOR               "--------------------------------------------------------------------------------"

#define PRINT_BANNER            do { print_banner(__FUNCTION__); } while (0)


#define PDI_SYM(tag)            {{NULL}, # tag, (char *) tag, 0, N_EXT | N_TEXT}
#define PDI_SYM_TAG(tag, sym)   {{NULL},   tag, (char *) sym, 0, N_EXT | N_TEXT}
#define PDI_SYM_SEG(tag, seg)   {{NULL}, # tag, (char *) tag, 0, N_EXT | (seg)}


extern int lkup(void);
extern int lkAddr(void);
extern int pdi_mem_show(void);

#define PDI_SERVER_SYM_DEFAULT   PDI_SYM_TAG("d", pdi_mem_show), \
                                 PDI_SYM(lkup), \
                                 PDI_SYM(lkAddr)

void print_banner(const char *func);
void print_sub_banner(const char *format, ...);
void pdi_server_init(unsigned int server_port, PDI_THREAD_CREATE_FUNCPTR thread_create_func, const char* name);

#endif
