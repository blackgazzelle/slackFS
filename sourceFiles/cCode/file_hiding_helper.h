#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <strings.h>
#include <limits.h>
#include <errno.h>
#include <argp.h>
#include <erasurecode.h>
#include <dirent.h>

// GLOBALS
#define CMD_LEN 1024
enum ARGS {OP=1, MAP_FILE=2, DISK_FILE=3, COVER_FILE=4};


// PROTOTYPES
int strip_null(char * null_map_file, char * disk_file);
int file_encode(char * filename, int ec_id, int k, int m, int w, int hd);
int hide_file(char * frag_dir_name, char * cover_files);
int retrieve_file(char * filename);
int file_decode(char * filename);
int restore_null(char * filename);