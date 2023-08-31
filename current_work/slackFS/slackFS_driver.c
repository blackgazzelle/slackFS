#include "slackFS_helper.h"

/*
 * Author: Christian Rose
 * Main driver for the slackFS framework, takes in arguments and based off of them
 * will either hide or retrieve a file into or from the slack space designated in
 * the cover file.
 */


int main(int argc, char * argv[]) {
    // Set debug stream
    char *map_file, *disk_file, *cover_file, *cover_map_file;
    // Argument Check
    if (argc < 2) {
        puts("USAGE: slackFS <OP> <OPERATION SPECIFIC OPTION>\n"
            "USAGE: slackFS hide <map_file> <disk_file> <cover_file>\n"
            "USAGE: slackFS retrieve <map_file> <disk_file> <cover_file>\n");
        return 0;
    }

    // Grab operation
    char * operation = argv[OP];

    // Process operation
    int ret;
    if (strcmp(operation, "hide") == 0) {
        // Check for additional arguments
        if (argc < 5) {
            puts("USAGE slackFS hide <map_file> <disk_file> <cover_file>");
            return 0;
        }

        // Grab filenames
        map_file = argv[MAP_FILE];
        disk_file = argv[DISK_FILE];
        cover_file = argv[COVER_FILE];

        // Strip file and exit if error
        ret = strip_null(map_file, disk_file);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in strip_null function, error code %d\n%s\n", ret, strerror(-ret));
            return -1;
        }
        
        // Get Stripped filename
        char * stripped_filename = malloc(FILENAME_MAX+10);
        sprintf(stripped_filename, "%s_stripped", disk_file);

        // Encode frags
        fragments * frags = file_encode(stripped_filename, 1, 16, 8, 2);
        if (frags == NULL) {
            fprintf(stderr, "ERROR: in file_encode function");
            return -1;
        }

        // Hide frags
        ret = hide_file(frags, cover_file);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in hide file function error code %d\n%s\n", ret, strerror(-ret));
            return -1;
        }
    }
    else if (strcmp(operation, "retrieve") == 0) {
        // Check for additional arguments
        if (argc < 5) {
            puts("USAGE slackFS retrieve <map_file> <disk_file> <cover_map_file>");
            return -1;
        }
        
        // Grab filenames
        map_file = argv[MAP_FILE];
        disk_file = argv[DISK_FILE];
        cover_map_file = argv[COVER_FILE];

        // Retrieve data
        fragments * frags;
        char * retrieved_filename = malloc(FILENAME_MAX+10);
        char * restored_filename = malloc(FILENAME_MAX+10);
        sprintf(retrieved_filename, "%s_retrieved", disk_file);
        sprintf(restored_filename, "%s_restored", disk_file);
        frags = retrieve_file(cover_map_file);
        if (frags == NULL) {
            fprintf(stderr, "ERROR: in retrieve function");
            return 0;
        }

        // Decode Data
        ret = file_decode(retrieved_filename, frags, 1, 16, 8, 2);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in decode file function error code %d\n%s\n", ret, strerror(-ret));
            return 0;
        }
        free_data(frags);
        // restore nulls in data
        ret = restore_null(retrieved_filename, restored_filename, map_file, disk_file);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in restore file function error code %d\n%s\n", ret, strerror(-ret));
            return 0;
        }
    }
    else {
        puts("ERROR: proper operations are hide and retrieve");
        return 0;
    }
    return 0;
}