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

/*
 * Author: Christian Rose
 * Header file defining slackFS file hiding and retrieving functions
 *
 */
// STRUCTS
typedef struct _decode_data
{
    int num_of_frags;
    int frag_size;
    char **data;
} decode_data;

// GLOBALS
#define CMD_LEN 1024
enum ARGS
{
    OP = 1,
    MAP_FILE = 2,
    DISK_FILE = 3,
    COVER_FILE = 4
};

// PROTOTYPES
int strip_null(char *null_map_file, char *disk_file);
int file_encode(char *filename, int ec_id, int k, int m, int hd, int ct);
int hide_file(char *frag_dir_name, char *cover_files);
decode_data *retrieve_file(char *cover_files, int frag_size, int num_of_frags);
int file_decode(char *out_file, decode_data *dc, int ec_id, int k, int m, int hd, int ct);
int restore_null(char *input_file, char *out_file, char *map_file, char *orig_file);
int free_data(decode_data *dc);