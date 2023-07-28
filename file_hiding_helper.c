#include "file_hiding_helper.h"

/*
 * Author: Christian Rose
 * Take a map file and a disk file. Correctly map the disk file and
 * strip the null bytes, storing those positions in the map file.
 */
int strip_null(char * null_map_file, char * disk_file) {
    printf("inside strip_null\n");

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
        printf("%s\n", null_map_file);
        return -errno;
    }

    // Open disk file to read and remove null bytes
    disk_fp = fopen(disk_file, "rb");
    if (disk_fp == NULL) {
        perror("ERROR: disk file");
        printf("%s\n", disk_file);
        return -errno;
    }

    // Open new destination file for stripped disk
    char * dst_file = malloc(strlen(disk_file)+10);
    sprintf(dst_file, "%s_stripped", disk_file);
    dst_fp = fopen(dst_file, "wb");
    if (dst_fp == NULL) {
        perror("ERROR: destination file");
        printf("%s\n", dst_file);
        return -errno;
    }

    // Parse through disk file to remove all NULL bytes
    for (int byte_offset = 0; byte_offset < stat_disk_file_size-1;) {
        ch = fgetc(disk_fp);

        null_len = 0;
        // if char is null, then compute the length
        if (ch == 0x00) {
            null_add = ftell(disk_fp)-1;

            // Calulate how many nulls in a sequence exist
            do {
                null_len++;

                // Verify not beyond filesize
                if (ftell(disk_fp)-1 > stat_disk_file_size-2) {

                    // Make final entry to map file
                    fprintf(map_fp, "%d %d\n", null_add, null_len);

                    // ADDED by Avinash Srinivasan --> START-1
                    fclose(map_fp);
                    fclose(dst_fp);
                    fclose(disk_fp);
                    return 0;
                    // ADDED by Avinash Srinivasan --> END-1

                    // break;
                }

                ch = fgetc(disk_fp);

            } while (ch == 0x00);

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
    // fclose(map_fp);
    // fclose(disk_fp);
    // fclose(dst_fp);
    return 0;
}

int file_encode(char * filename, int ec_id, int k, int m, int w, int hd, int ct) {
    fprintf(stdout, "inside file_encode\n");
    // Setting backend to Jerasure_RS_VAND
    ec_backend_id_t id = ec_id;

    // Creating a new instance of the erasure coder
    struct ec_args args;
    args.k = k;
    args.m = m;
    args.w = w;
    args.hd = hd;
    args.ct = ct;
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
    printf("orig_data: %s\norig_data_size: %ld\n", orig_data, orig_data_size);
    // fflush(stdout);

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
    stat("./frags/frag.0", &st);
    // fprintf(stdout, "%ld\n", st.st_size );
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
    liberasurecode_instance_destroy(instance_descriptor);
    free(orig_data);
    return 0;
}

int hide_file(char * frag_dir_name, char * cover_files) {
    fprintf(stdout, "inside hide_file\n");
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
                            "sudo bmap --mode putslack %s",
                        frag_dir_name,
                        dir_entry->d_name,
                        block_size,
                        skip_size,
                        cover_file);
                ret = system(cmd);
                if(!ret) {
                    free(cover_file);
                    cover_file_counter++;
                    break;
                }

                free(cover_file);
            } while(1);
        }
    }

    // Delete all frag files and the directory
    frag_dir = opendir(frag_dir_name);
    if (frag_dir == NULL) {
        perror("ERROR: could not open fragment directory");
        return -errno;
    }
    while ((dir_entry = readdir(frag_dir)) != NULL) {
        if (dir_entry->d_type == DT_REG) {
            remove(dir_entry->d_name);
        }
    }
    rmdir(frag_dir_name);
    fclose(cover_fp);
    free(cmd);
    return 0;
}

decode_data* retrieve_file(char* cover_files, int frag_size, int num_of_frags){
    fprintf(stdout, "inside retrieve_file\n");
    char * cmd = malloc(CMD_LEN);
    char * tmp_file = "frag_tmp.txt";
    int ret;
    // Set up decode data Struct
    decode_data * dc_data = malloc(sizeof(decode_data*));
    dc_data->data = calloc(num_of_frags+1, sizeof(char*));
    dc_data->num_of_frags = num_of_frags;
    dc_data->frag_size = frag_size;

    // Open cover file list
    FILE * cover_fp = fopen(cover_files, "r");
    if (cover_fp == NULL) {
        perror("ERROR: cannot open cover file list");
        return NULL;
    }

    /* For each frag retrieve it from the cover file, in the same order that
     * they were stored
    */
    for (int i = 0; i < num_of_frags; i++) {
        int available_slack = 0, skip_size=0, block_size;
        do {
            // Get cover file and size
            char * cover_file = malloc(FILENAME_MAX);
            fscanf(cover_fp, "%s %i", cover_file, &available_slack);
            struct stat st_cover;

            // Make sure cover file is a regular file
            if ((stat(cover_file, &st_cover) == -1) &&
                    S_ISREG(st_cover.st_mode) != 0) {
                    free(cover_file);
                    continue;
            }

            // Make sure the available slack is big enough for the fragment
            if (frag_size > available_slack) {
                free(cover_file);
                continue;
            }

            // Use bmap and retrieve the fragment from the slack space
            block_size = available_slack;


            // ADDED by Avinash Srinivasan --> START-2
            sprintf(cmd, "sudo bmap --mode slack %s %d | dd of=%s "
                          "oflag=seek_bytes count=1 bs=%d seek=%d conv=notrunc",
                    cover_file,
                    frag_size,
                    tmp_file,
                    block_size,
                    skip_size);
            // ADDED by Avinash Srinivasan --> END-2


            // sprintf(cmd, "sudo bmap --mode slack %s %d | dd of=%s "
            //         "oflag=seek_bytes count=1 bs=%d seek=%d",
            //         cover_file,
            //         frag_size,
            //         tmp_file,
            //         block_size,
            //         skip_size);
            ret = system(cmd);
            if(!ret) {
                // Read from tmp file and store in array
                FILE * tmp_fp = fopen(tmp_file, "rb");
                if (tmp_fp == NULL) {
                    perror("ERROR: cannot temp file");
                    return NULL;
                }

                // Allocate space for this fragment and read it
                dc_data->data[i] = malloc(frag_size+1);
                fread(dc_data->data[i], 1, frag_size, tmp_fp);
                fclose(tmp_fp);
                break;
            }

            free(cover_file);
        } while(1);
    }
    fclose(cover_fp);
    free(cmd);
    remove(tmp_file);
    return dc_data;
}

int file_decode(char* out_file, decode_data * dc, int ec_id, int k, int m, int w, int hd, int ct) {
    fprintf(stdout, "inside file_decode\n");
    // Set up backend
    ec_backend_id_t id = ec_id;
    // Creating a new instance of the erasure coder
    struct ec_args args;
    args.k = k;//24; //16;
    args.m = m; //12; //8;
    args.w = w;
    args.hd = args.m;
    args.ct = ct;
    char * out_data;
    u_int64_t out_data_len;
    int instance_descriptor = liberasurecode_instance_create(id, &args);
    int ret = liberasurecode_decode(instance_descriptor, dc->data,
                    dc->num_of_frags, dc->frag_size, 0, &out_data, &out_data_len);
    printf("\n\nout_data: %s\n\nout_data_len: %ld\n\n", out_data, out_data_len);
    // printf("\n\ndc->num_of_frags:%d\n\ndc->frag_size: %d\n\n", dc->num_of_frags, dc->frag_size);
    // Write to outfile
    FILE * out_fp = fopen(out_file, "wb");
    if (out_fp == NULL) {
        perror("ERROR: cannot open cover file list");
        return -errno;
    }
    fwrite(out_data, 1, out_data_len, out_fp);

    // Clean up data
    fclose(out_fp);
    liberasurecode_decode_cleanup(id, out_data);
    liberasurecode_instance_destroy(instance_descriptor);
    return 0;
}

int restore_null(char * input_file, char * out_file, char * map_file, char * orig_file)
{
    fprintf(stdout, "inside restore_NULL\n");
    fprintf(stdout, "\n==================================================\n");
    FILE *map_fp, *input_fp, *out_fp;

    struct stat st;
    // computing size of the original filesystem ".dmg" file
    // you need to update this accordingly
    stat(orig_file, &st);
    int statFileSize = st.st_size;

    // open the map file
    map_fp = fopen(map_file, "r");
    if (map_fp == NULL)
    {
        ferror(map_fp);
        fclose(map_fp);
        return -errno;
    }
    // image that needs to be restored to full size
    // by reinserting null-byte(s) referencing *fpMap
    input_fp = fopen(input_file, "rb");
    if (input_fp == NULL)
    {
        ferror(input_fp);
        fclose(input_fp);
        return -errno;
    }

    // file to write the restored filesystem image
    out_fp = fopen(out_file, "wb");
    if (out_fp == NULL)
    {
        ferror(out_fp);
        fclose(out_fp);
        return -errno;
    }

    // variables to track start address and length
    // null bytes sequence entries in mapFile
    int nullADD = 0, nullLEN = 0;

    // variables to track start address and length
    // of data bytes in the srcIMG (.dmg without NULL bytes)
    int dataADD = 0, dataLEN = 0;

    while (1)
    {
        nullADD = 0, nullLEN = 0, dataLEN = 0;
        if (!feof(map_fp))
        {
            fscanf(map_fp, "%d %d", &nullADD, &nullLEN);

            // checking NULL byte start address and then deciding how
            // many bytes to read from the srcIMG with no NULL
            dataLEN = abs(nullADD - dataADD);
            if (dataADD + dataLEN > statFileSize)
            {
                dataLEN = statFileSize - dataADD;
            }

            char *tempDataBuff = malloc(dataLEN * sizeof(char));
            fread(tempDataBuff, dataLEN, sizeof(char), input_fp);
            if (!fseek(out_fp, dataADD, SEEK_SET))
                ferror(out_fp);

            if (!fwrite(tempDataBuff, dataLEN, sizeof(char), out_fp))
                ferror(out_fp);
            memset(tempDataBuff, 0x00, dataLEN);

            char *tempNullBuff = malloc(nullLEN * sizeof(char));
            memset(tempNullBuff, 0x00, nullLEN);
            if (!fseek(out_fp, nullADD, SEEK_SET))
                ferror(out_fp);
            if (!fwrite(tempNullBuff, nullLEN, sizeof(char), out_fp))
                ferror(out_fp);

            if (((dataADD + dataLEN) > statFileSize) ||
                ((nullADD + nullLEN) > statFileSize) || (feof(map_fp)))
            {
              //ADDED by Avinash Srinivasan--Start-3
              fclose(map_fp);
              fclose(input_fp);
              fclose(out_fp);
              return 0;
              //ADDED by Avinash Srinivasan--END-3
              // break;
            }
            else
            {
                dataADD = (nullADD + nullLEN);
            }
        }
    }

    // fclose(map_fp);
    // fclose(input_fp);
    // fclose(out_fp);
    // return 0;
}

int free_data(decode_data * dc) {
    for (int i = 0; i < dc->num_of_frags; i++) {
        free(dc->data[i]);
    }
    free(dc->data);
    free(dc);
    return 0;
}
