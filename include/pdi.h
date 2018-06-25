 #ifndef __PDI_H__
#define __PDI_H__

#include <stdio.h>
#include <string.h>

#define FAST                register
#define IMPORT              extern
#define LOCAL               static
#define EOS                 '\0'
#define PDI_OK              (0)
#define PDI_ERROR           (-1)

#define  STD_IN             (0)
#define  STD_OUT            (1)
#define  STD_ERR            (2)

#define PDI_MAX_LINE_LEN    (128)

#define N_TEXT              0x4
#define N_DATA              0x6
#define N_BSS               0x8
#define N_EXT               01     //external bit, or'ed in
#define	N_TYPE	            0x1e   //mask for all the type bits


typedef unsigned char SYM_TYPE;

typedef enum {
    DEBUG_TOOL_NULL,
    DEBUG_TOOL_COMMAND,
    DEBUG_TOOL_REPLY,
    DEBUG_TOOL_ERROR
} pdi_command_id_e;

typedef struct {
    pdi_command_id_e        command_id;                      // message type 
    char                    tx_proc[PDI_MAX_LINE_LEN+1];     // process sending the message
    char                    stdout_path[PDI_MAX_LINE_LEN+1]; // file path for stdout
    char                    buff[PDI_MAX_LINE_LEN+1];        // message payload
} pdi_msg_t;

typedef struct slnode{                                     // Node of a linked list.
    struct slnode           *next;                           // Points at the next node in the list
} SL_NODE;

typedef struct{
    SL_NODE                 hash_node;                          // hash node (must come first)
    const char              *name;                              // pointer to symbol name
    void                    *value;                             // symbol value
    unsigned short          group;                              // symbol group
    SYM_TYPE                type;                               // symbol type
} SYMBOL; //SYMBOL: entry in symbol table




extern SYMBOL               standTbl[];                         // standalone symbol table array
extern unsigned int         standTblSize;                       // symbols in standalone table

typedef void*   (*PDI_THREAD_FUNCPTR)(void *); 
typedef int     (*PDI_THREAD_CREATE_FUNCPTR)(PDI_THREAD_FUNCPTR);

#endif
