#include "file_hiding_helper.h"

int main(int argc, char * argv[]) {
    char *map_file, *disk_file, *cover_file;
    // Argument Check
    if (argc < 2) {
        puts("USAGE: file_hiding <OP> <OPERATION SPECIFIC OPTION>\n"
            "USAGE: file_hiding hide <map_file> <disk_file> <cover_file>\n"
            "USAGE: file_hiding retrieve <map_file> <disk_file> <cover_file>\n");
        return 0;
    }

    // Grab operation
    char * operation = argv[OP];

    // Process operation
    int ret;
    if (strcmp(operation, "hide") == 0) {
        // Check for additional arguments
        if (argc < 5) {
            puts("USAGE file_hiding hide <map_file> <disk_file> <cover_file>");
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
            return 0;
        }
        
        // Get Stripped filename
        char * stripped_filename = malloc(FILENAME_MAX+10);
        sprintf(stripped_filename, "%s_stripped", disk_file);

        // Encode frags
        ret = file_encode(stripped_filename, 1, 16, 8, 8, 8, 1);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in file_encode function, error code %d\n%s\n", ret, strerror(-ret));
            return 0;
        }

        // Hide frags
        char * frag_dir = "frags";
        ret = hide_file(frag_dir, cover_file);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in hide file function error code %d\n%s\n", ret, strerror(-ret));
            return 0;
        }
    }
    else if (strcmp(operation, "retrieve") == 0) {
        // Check for additional arguments
        if (argc < 5) {
            puts("USAGE file_hiding retrieve <map_file> <disk_file> <cover_file>");
            return 0;
        }
        
        // Grab filenames
        map_file = argv[MAP_FILE];
        disk_file = argv[DISK_FILE];
        cover_file = argv[COVER_FILE];

        // Retrieve data
        decode_data * dc;
        char * retrieved_filename = malloc(FILENAME_MAX+10);
        char * restored_filename = malloc(FILENAME_MAX+10);
        sprintf(retrieved_filename, "%s_retrieved", disk_file);
        sprintf(restored_filename, "%s_restored", disk_file);
        dc = retrieve_file(cover_file, 92, 24);
        if (dc == NULL) {
            fprintf(stderr, "ERROR: in retrieve function");
            return 0;
        }

        // Decode Data
        ret = file_decode(retrieved_filename, dc, 1, 16, 8, 8, 8, 1);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in decode file function error code %d\n%s\n", ret, strerror(-ret));
            return 0;
        }
        free_data(dc);
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