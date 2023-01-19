#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <erasurecode.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <strings.h>
#include <limits.h>
#include <errno.h>
#include <argp.h>

// GLOBALS
#define FILENAME 128;
enum ARGS {OP=1, MAP_FILE=2, DISK_FILE=3, COVER_FILE=4};


// PROTOTYPES
int strip_null(char * map_file, char * disk_file);
int hide_file(char * filename);
int retrieve_file(char * filename);
int restore_null(char * filename);