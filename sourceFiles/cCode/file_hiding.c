#include "file_hiding_helper.h"

int main(int argc, char * argv[]) {
    char *map_file, *disk_file, *cover_file;
    // Argument Check
    if (argc < 2) {
        puts("USAGE: file_hiding <OP> <OPERATION SPECIFIC OPTION>\n"
            "USAGE: file_hiding strip <map_file> <disk_file>\n"
            "USAGE: file_hiding hide <map_file> <disk_file> <cover_file>\n"
            "USAGE: file_hiding hide_null <map_file> <disk_file> <cover_file>\n"
            "USAGE: file_hiding retrieve <map_file> <disk_file> <cover_file>\n"
            "USAGE: file_hiding restore <map_file> <disk_file> <cover_file>\n"
            "USAGE: file_hiding restore_null <map_file> <disk_file> <cover_file>");
        return 0;
    }

    // Grab operation
    char * operation = argv[OP];

    // Process operation
    int ret;
    if (strcmp(operation, "strip") == 0) {
        // Check for additional arguments
        if (argc < 4) {
            puts("USAGE file_hiding strip <map_file> <disk_file>");
            return 0;
        }

        // Grab filenames
        map_file = argv[MAP_FILE];
        disk_file = argv[DISK_FILE];

        // Strip file and exit if error
        ret = strip_null(map_file, disk_file);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in strip_null function, error code %d\n", ret);
            return 0;
        }
    }

    else if (strcmp(operation, "hide") == 0) {
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
        ret = file_encode(stripped_filename, 1, 16, 8, 8, 8);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in file_encode function, error code %d\n%s\n", ret, strerror(-ret));
            return 0;
        }

        // Hide frags
        char * frag_dir = "frags";
        ret = hide_file(frag_dir, cover_file);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in hide)file function error code %d\n%s\n", ret, strerror(-ret));
            return 0;
        }
    }
    
    else if (strcmp(operation, "hide_null") == 0) {}
    else if (strcmp(operation, "retrieve") == 0) {}
    else if (strcmp(operation, "restore") == 0) {}
    else if (strcmp(operation, "restore_null") == 0) {}
    else {
        puts("ERROR: proper operations are hide, strip, hide_null, restore,"
                " retrieve, restore_null");
        return 0;
    }
    return 0;
}