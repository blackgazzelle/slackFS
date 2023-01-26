#include "file_hiding_helper.h"

/*
 * Take a map file and a disk file. Correctly map the disk file and
 * strip the null bytes, storing those positions in the map file.
 */
int strip_null(char * null_map_file, char * disk_file) {
    FILE * map_fp, *disk_fp, *dst_fp;
    char ch;
    struct stat disk_st;
    int stat_disk_file_size, null_len = 0, null_add = 0;

    // Get Stat info for disk file
    stat(disk_file, &disk_st);
    stat_disk_file_size = disk_st.st_size;

    // Open the map file to write into it
    map_fp = fopen(null_map_file, "w");
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
    sprintf(dst_file, "%s_stripped", disk_file);
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
                    break;
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
    return 0;
}

int file_encode(char * filename, int ec_id, int k, int m, int w, int hd) {
    // Setting backend to Jerasure_RS_VAND 
    ec_backend_id_t id = id;

    // Creating a new instance of the erasure coder
    struct ec_args args;
    args.k = k;
    args.m = m;
    args.w = w;
    args.hd = hd;
    int instance_descriptor = liberasurecode_instance_create(id, &args);

    // Read in a file and store that data
    char * orig_data;
    uint64_t orig_data_size;
    uint64_t read_bytes = 0;
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("ERROR: file to encode");
        return -errno;
    }

    fseek(fp, 0L, SEEK_END);
    orig_data_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    orig_data = malloc(orig_data_size);
    fread(orig_data, orig_data_size, 1, fp);
    fclose(fp);
    
    // Call liberasurecode_encode to actualy encode the data now
    char **encoded_data, **encoded_parity;
    uint64_t fragment_len;
    int ret = liberasurecode_encode(instance_descriptor, orig_data,
                                orig_data_size, &encoded_data,
                                &encoded_parity, &fragment_len);


    // Make directory for frags
    struct stat st = {0};

    if (stat("./frags", &st) == -1) {
        mkdir("./frags", 0777);
    }

    //Write encoded data
    int i = 0;
    char * frag_file = malloc(FILENAME_MAX);
    for (;i < args.k; i++) {
        sprintf(frag_file, "./frags/frag.%d", i);
        fp = fopen(frag_file, "w+");
        if (fp == NULL) {
            perror("ERROR: Bad frag file");
            return -errno;
        }
        fwrite(encoded_data[i], fragment_len, 1, fp);
        fclose(fp);
    }
    //Write parity frags
    for (int j = 0;j < args.m; j++) {
        sprintf(frag_file, "./frags/frag.%d", i);
        fp = fopen(frag_file, "w+");
        if (fp == NULL) {
            perror("ERROR: Bad frag file");
            return -errno;
        }
        fwrite(encoded_parity[j], fragment_len, 1, fp);
        i++;
        fclose(fp);
    }

    // Clean up data    
    liberasurecode_encode_cleanup(instance_descriptor, encoded_data, encoded_parity);
    free(orig_data);
    return 0;
}

int hide_file(char * frag_dir_name, char * cover_files) {
    char * cmd = malloc(CMD_LEN);
    struct stat st_frag;
    int ret, num_frags = 0, cover_file_counter = 0;

    // Open fragment directory, to get number of frags
    DIR * frag_dir = opendir(frag_dir_name);
    struct dirent * dir_entry;
    if (frag_dir == NULL) {
        perror("ERROR: could not open fragment directory");
        return -errno;
    }

    while((dir_entry = readdir(frag_dir)) != NULL) {
        if (dir_entry->d_type == DT_REG) {
            num_frags++;
        }
    }
    closedir(frag_dir);


    /*
    * Loop through fragments and list of cover files at the same time
    * store fragments in a cover file with enough space, continue till
    * all fragments are stored
    */
    frag_dir = opendir(frag_dir_name);
    if (frag_dir == NULL) {
        perror("ERROR: could not open fragment directory");
        return -errno;
    }


    // Open cover file list
    FILE * cover_fp = fopen(cover_files, "r");
    if (cover_fp == NULL) {
        perror("ERROR: cannot open cover file list");
        return -errno;
    }

    while ((dir_entry = readdir(frag_dir)) != NULL) {
        if (dir_entry->d_type == DT_REG) {
            int available_slack = 0, skip_size=0, block_size;
            
            // Get frag size
            stat(dir_entry->d_name, &st_frag);
            int frag_size = st_frag.st_size;
            
            do {            
                // Get cover file and size
                char * cover_file = malloc(FILENAME_MAX);
                fscanf(cover_fp, "%s", cover_file);
                struct stat st_cover;

                // Make sure cover file is a regular file
                if ((stat(cover_file, &st_cover) == -1) && 
                        S_ISREG(st_cover.st_mode) != 0) {
                        free(cover_file);
                        continue;
                }

                // Make sure the available slack is big enough for the fragment
                fscanf(cover_fp, "%i", &available_slack);
                if (frag_size > available_slack) {
                    free(cover_file);
                    continue;
                }

                // Use bmap and write the fragment to the slack space
                block_size = available_slack;
                sprintf(cmd, "dd iflag=skip_bytes count=1 if=%s/%s bs=%d skip=%d | "
                            "sudo ../../bmap/bmap --mode putslack %s",
                        frag_dir_name,
                        dir_entry->d_name,
                        block_size,
                        skip_size,
                        cover_file);
                if(ret = system(cmd)) {
                    free(cover_file);
                    break;
                }

                free(cover_file);
            } while(1);
        }
        else {
            fprintf(stderr, "ERROR: not a fragment file\n%s\n", strerror(errno));
            return -errno;
        }
    }
    free(cmd);
    return 0;
}

int retrieve_file(char * filename){
    return 0;
}

int file_decode(char * filename) {

}

int restore_null(char * filename){
    return 0;
}