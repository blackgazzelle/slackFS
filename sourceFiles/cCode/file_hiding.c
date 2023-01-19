#include <file_hiding.h>

int main(int argc, char * argv[]) {
    char *map_file, *disk_file, *cover_file;
    // Argument Check
    if (argc < 3) {
        puts("USAGE: file_hiding <OP> <OPERATION SPECIFIC OPTIONS"
            "USAGE file_hiding strip <map_file> <disk_file> <cover_file>"
            "USAGE file_hiding hide <map_file> <disk_file> <cover_file>"
            "USAGE file_hiding hide_null <map_file> <disk_file> <cover_file>"
            "USAGE file_hiding retrieve <map_file> <disk_file> <cover_file>"
            "USAGE file_hiding restore <map_file> <disk_file> <cover_file>"
            "USAGE file_hiding restore_null <map_file> <disk_file> <cover_file>");
        return 0;
    }

    // Grab operation
    char * operation = argv[OP];

    // Process operation
    int ret;
    if (strcmp(operation, "strip") == 0) {
        // Check for additional arguments
        if (argc < 6) {
            puts("USAGE file_hiding strip <map_file> <disk_file> <cover_file>");
            return 0;
        }

        // Grab filenames
        map_file = argv[MAP_FILE];
        disk_file = argv[DISK_FILE];
        cover_file = argv[COVER_FILE];

        // Strip file and exit if error
        ret = strip_null(map_file, disk_file, cover_file);
        if (ret < 0) {
            fprintf(stderr, "ERROR: in strip_null function, error code %d\n", ret);
            return 0;
        }
    }
    else if (strcmp(operation, "hide") == 0) {
        ret = strip_null(map_file, disk_file, cover_file);
    }
    else if (strcmp(operation, "hide_null") == 0) {}
    else if (strcmp(operation, "retrieve") == 0) {}
    else if (strcmp(operation, "restore") == 0) {}
    else if (strcmp(operation, "restore_null") == 0) {}
    else {
        puts("ERROR: proper operations are hide, strip, hide_null, restore,\
                retrieve, restore_null");
        return 0;
    }
    return 0;
}

/*
 * Take a map file, a disk file, and a coverfile list, use the coverfile list
 * to correctly map the disk file and strip the null bytes, storing those 
 * positions in the map file.
 */
int strip_null(char * map_file, char * disk_file, char * cover_file) {
    FILE * map_fp, *disk_fp, *cover_fp, *dst_fp;
    char ch;
    struct stat disk_st;
    int stat_disk_file_size, null_len = 0, null_add = 0;

    // Get Stat info for disk file
    stat(disk_file, &disk_st);
    stat_disk_file_size = disk_st.st_size;

    // Open the map file to write into it
    map_fp = fopen(map_file, "w");
    if (map_fp == NULL) {
        perror("ERROR: map file");
        return -errno;
    }

    // Open disk file to read and remove null bytes
    disk_fp = fopen(disk_file, "rb");
    if (disk_fp == NULL) {
        perror("ERROR: disk file");
        return -errno;
    }

    // Open new destination file for stripped disk
    char * dst_file = malloc(strlen(disk_file)+10);
    dst_file = sprintf(dst_file, "%s_stripped", disk_file);
    dst_fp = fopen(dst_file, "wb");
    if (dst_fp == NULL) {
        perror("ERROR: destination file");
        return -errno;
    }

    // Parse through disk file to remove all NULL bytes
    for (int byte_offset = 0; byte_offset < stat_disk_file_size-1;) {
        ch = fgetc(disk_fp);

        null_len = 0;
        // if char is null, then compute the length
        if (ch == '\00') {
            null_add = ftell(disk_fp)-1;

            // Calulate how many nulls in a sequence exist
            do {
                null_len++;

                // Verify not beyond filesize
                if (ftell(disk_fp)-1 > stat_disk_file_size-2) {

                    // Make final entry to map file
                    fprintf(map_fp, "%d %d\n", null_add, null_len);
                    fclose(disk_fp);
                    fclose(dst_fp);
                }

                ch = fgetc(disk_fp);

            } while (ch == '\00');

            // Make an entry into map file
            fprintf(map_fp, "%d %d\n", null_add, null_len);
            byte_offset = ftell(disk_fp)-1;

            // exiting while loop we must write non-null char
            fputc(ch, dst_fp);
        }
        // else write non-null char to dst file
        else {
            fputc(ch, dst_fp);
            byte_offset++;
        }
    }

    fclose(map_fp);
    fclose(disk_fp);
    fclose(dst_fp);
    free(dst_file);
    return 0;
}

int hide_file(char * filename){}
int retrieve_file(char * filename){}
int restore_null(char * filename){}