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
typedef struct _file_pair {
    char * filename;
    int size_written;
} file_pair;

typedef struct _fragment_info
{
    file_pair ** file_pairs;
    int num_of_files;
    int size;
    char * data;
} fragment_info;

typedef struct _fragments
{
    int num_of_frags;
    int num_of_parity;
    fragment_info ** info_arr;
} fragments;

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
fragments * file_encode(char *filename, int backend_id, int num_frags, int num_parity, int checksum);
int hide_file(fragments *frags, char *cover_files);
fragments *retrieve_file(char * map_file);
int file_decode(char *out_file, fragments *frags, int backend_id, int num_frags, int num_parity, int checksum);
int restore_null(char *input_file, char *out_file, char *map_file, char *orig_file);
int free_data(fragments *frags);
