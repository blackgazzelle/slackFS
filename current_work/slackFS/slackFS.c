#include "slackFS_helper.h"

/*
 * Author: Christian Rose
 * Take a map file and a disk file. Correctly map the disk file and
 * strip the null bytes, storing those positions in the map file.
 */
int strip_null(char *null_map_file, char *disk_file) {
    FILE *map_fp, *disk_fp, *dst_fp;
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
    char *dst_file = malloc(strlen(disk_file) + 10);
    sprintf(dst_file, "%s_stripped", disk_file);
    dst_fp = fopen(dst_file, "wb");
    if (dst_fp == NULL) {
        perror("ERROR: destination file");
        printf("%s\n", dst_file);
        return -errno;
    }

    // Parse through disk file to remove all NULL bytes
    for (int byte_offset = 0; byte_offset < stat_disk_file_size - 1;) {
        ch = fgetc(disk_fp);

        null_len = 0;
        // if char is null, then compute the length
        if (ch == '\00') {
            null_add = ftell(disk_fp) - 1;

            // Calculate how many nulls in a sequence exist
            do {
                null_len++;

                // Verify not beyond filesize
                if (ftell(disk_fp) - 1 > stat_disk_file_size - 2) {
                    // Make final entry to map file
                    fprintf(map_fp, "%d %d\n", null_add, null_len);
                    fclose(map_fp);
                    fclose(dst_fp);
                    fclose(disk_fp);
                    return 0;
                }

                ch = fgetc(disk_fp);

            } while (ch == '\00');

            // Make an entry into map file
            fprintf(map_fp, "%d %d\n", null_add, null_len);
            byte_offset = ftell(disk_fp) - 1;

            // exiting while loop we must write non-null char
            fputc(ch, dst_fp);
        }
        // else write non-null char to dst file
        else {
            fputc(ch, dst_fp);
            byte_offset++;
        }
    }

    return 0;
}

fragments *file_encode(char *filename, int backend_id, int num_frags, int num_parity, int checksum) {
    // Setup the backend
    ec_backend_id_t id = backend_id;

    // Creating a new instance of the erasure coder
    struct ec_args args;
    args.k = num_frags;
    args.m = num_parity;
    args.hd = num_parity;
    args.ct = checksum;
    int instance_descriptor = liberasurecode_instance_create(id, &args);

    // Read in file to hide and store that in an array of structs
    char *orig_data;
    uint64_t orig_data_size;
    uint64_t read_bytes = 0;
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("ERROR: file to encode");
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    orig_data_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    orig_data = malloc(orig_data_size);
    fread(orig_data, orig_data_size, 1, fp);
    fclose(fp);

    // Call liberasurecode_encode to actually encode the data now
    char **encoded_data, **encoded_parity;
    uint64_t fragment_len;
    int ret = liberasurecode_encode(instance_descriptor, orig_data, orig_data_size, &encoded_data, &encoded_parity,
                                    &fragment_len);

    // Make struct to hold the fragments
    fragments *frags = malloc(sizeof(fragments));
    frags->num_of_frags = num_frags + num_parity;
    frags->info_arr = calloc(frags->num_of_frags, sizeof(fragment_info *));

    // Save off regular frags
    int i = 0;
    for (; i < args.k; i++) {
        frags->info_arr[i] = malloc(sizeof(fragment_info));
        frags->info_arr[i]->data = strndup(encoded_data[i], fragment_len);
        frags->info_arr[i]->size = fragment_len;
        fprintf(stderr, "Encoding data of size %d", frags->info_arr[i]->size);
    }
    // Save off parity frags
    for (int j = 0; j < args.m; j++) {
        frags->info_arr[i] = malloc(sizeof(fragment_info));
        frags->info_arr[i]->data = strndup(encoded_parity[j],fragment_len);
        frags->info_arr[i]->size = fragment_len;
        fprintf(stderr, "Encoding data of size %d", frags->info_arr[i]->size);
        i++;
    }

    // Clean up data
    liberasurecode_encode_cleanup(instance_descriptor, encoded_data, encoded_parity);
    liberasurecode_instance_destroy(instance_descriptor);
    free(orig_data);
    return frags;
}

int hide_file(fragments *frags, char *cover_files) {
    char *cmd = malloc(CMD_LEN);
    struct stat st_frag;
    int ret;
    char *tmp = "tmp.txt";

    // Loop over each fragment and hide each in a series of cover files, noting each cover file in the struct
    // Open cover file list
    FILE *cover_fp = fopen(cover_files, "r"), *tmp_file;
    if (cover_fp == NULL) {
        perror("ERROR: cannot open cover file list");
        return -errno;
    }

    for (int i = 0; i < frags->num_of_frags; i++) {
        int available_slack = 0, skip_size = 0, block_size, bytes_written = 0, max_files = 2;
        // Initialize the list of filenames
        frags->info_arr[i]->file_pairs = calloc(max_files, sizeof(file_pair*));
        frags->info_arr[i]->num_of_files = 0;
        do {
            // Get cover file and size
            char *cover_file = malloc(FILENAME_MAX);
            fscanf(cover_fp, "%s%i", cover_file, &available_slack);
            struct stat st_cover;

            // Make sure cover file is a regular file
            if ((stat(cover_file, &st_cover) == -1) && S_ISREG(st_cover.st_mode) != 0) {
                free(cover_file);
                continue;
            }

            // Write to tmp file
            tmp_file = fopen(tmp, "w");
            fwrite(frags->info_arr[i]->data+bytes_written, available_slack, 1, tmp_file);
            fclose(tmp_file);

            // Use bmap and write the fragment to the slack space
            block_size = available_slack;
            sprintf(cmd,
                    "dd iflag=skip_bytes count=1 if=%s bs=%d skip=%d | "
                    "sudo bmap --mode putslack %s",
                    tmp, block_size, skip_size, cover_file);
            ret = system(cmd);
            if (!ret) {
                // Account for hiding available slack amount of the fragment
                fprintf(stderr, "Hiding %d amount", available_slack);
                bytes_written += available_slack;

                // Note the cover file we wrote to and update the amount of files in use
                frags->info_arr[i]->file_pairs[frags->info_arr[i]->num_of_files]->filename = strdup(cover_file);
                frags->info_arr[i]->file_pairs[frags->info_arr[i]->num_of_files]->size_written = available_slack;

                frags->info_arr[i]->num_of_files++;
                if (frags->info_arr[i]->num_of_files >= max_files) {
                    max_files *= 2;
                    frags->info_arr[i]->file_pairs = realloc(frags->info_arr[i]->file_pairs, max_files * sizeof(file_pair*));
                }

            } else {
                fprintf(stderr, "%s Command failed with code: %d", cmd, ret);
            }

            free(cover_file);
        } while (bytes_written < frags->info_arr[i]->size);
    }

    // Write data about cover files to a mapping file to be read later
    FILE * map_file = fopen("hiding_map.txt", "wb");
    fprintf(map_file, "#\tnum_of_files\tfilename\tsize_written\n");
    for (int i = 0; i < frags->num_of_frags; i++) {
        fprintf(map_file, "%d\t", i);
        fprintf(map_file, "%d\t", frags->info_arr[i]->num_of_files);
        for (int j = 0; j < frags->info_arr[i]->num_of_files; j++) {
            fprintf(map_file, "%s\t", frags->info_arr[i]->file_pairs[j]->filename);
            fprintf(map_file, "%d\t", frags->info_arr[i]->file_pairs[j]->size_written);
        }
        fprintf(map_file, "\n");

    }


    fclose(cover_fp);
    free(cmd);
    return 0;
}

fragments *retrieve_file(char *map_file) {
    char *cmd = malloc(CMD_LEN);
    char *tmp_file = "tmp.txt";
    int ret;

    // Read in frags structure from map_file

    // Loop over frags and read from each file to build the frags
    //// Use bmap and retrieve the fragment from the slack space
    //block_size = available_slack;
    //sprintf(cmd,
    //        "sudo bmap --mode slack %s %d | dd of=%s "
    //        "oflag=seek_bytes count=1 bs=%d seek=%d conv=notrunc",
    //        cover_file, frag_size, tmp_file, block_size, skip_size);
    //ret = system(cmd);
    //if (!ret) {
    //    // Read from tmp file and store in array
    //    FILE *tmp_fp = fopen(tmp_file, "rb");
    //    if (tmp_fp == NULL) {
    //        perror("ERROR: cannot temp file");
    //        return NULL;
    //    }
    //}
    //free(cmd);
    //remove(tmp_file);
    //return frags;
}

int file_decode(char *out_file, fragments *frags, int backend_id, int num_frags, int num_parity, int checksum) {
    // Set up backend
    ec_backend_id_t id = backend_id;
    // Creating a new instance of the erasure coder
    struct ec_args args;
    args.k =  num_frags;
    args.m = num_parity;
    args.hd = num_parity;
    args.ct = checksum;

    // Combine all the data into one array
    char * data[frags->num_of_frags];
    for(int i = 0; i < frags->num_of_frags; i++) {
        data[i] = frags->info_arr[i]->data;
    }

    char *out_data;
    u_int64_t out_data_len;
    int instance_descriptor = liberasurecode_instance_create(id, &args);
    int ret = liberasurecode_decode(instance_descriptor, data, frags->num_of_frags, frags->info_arr[0]->size, 0, &out_data,
                                    &out_data_len);

    // Write to outfile
    FILE *out_fp = fopen(out_file, "wb");
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

int restore_null(char *input_file, char *out_file, char *map_file, char *orig_file) {
    FILE *map_fp, *input_fp, *out_fp;

    struct stat st;
    // computing size of the original filesystem ".dmg" file
    // you need to update this accordingly
    stat(orig_file, &st);
    int statFileSize = st.st_size;

    // open the map file
    map_fp = fopen(map_file, "r");
    if (map_fp == NULL) {
        perror(map_file);
        return -errno;
    }
    // image that needs to be restored to full size
    // by reinserting null-byte(s) referencing *fpMap
    input_fp = fopen(input_file, "rb");
    if (input_fp == NULL) {
        perror(input_file);
        return -errno;
    }

    // file to write the restored filesystem image
    out_fp = fopen(out_file, "wb");
    if (out_fp == NULL) {
        perror(out_file);
        return -errno;
    }

    // variables to track start address and length
    // null bytes sequence entries in mapFile
    int nullADD = 0, nullLEN = 0;

    // variables to track start address and length
    // of data bytes in the srcIMG (.dmg without NULL bytes)
    int dataADD = 0, dataLEN = 0;

    while (1) {
        nullADD = 0, nullLEN = 0, dataLEN = 0;
        if (!feof(map_fp)) {
            fscanf(map_fp, "%d %d", &nullADD, &nullLEN);

            // checking NULL byte start address and then deciding how
            // many bytes to read from the srcIMG with no NULL
            dataLEN = abs(nullADD - dataADD);
            if (dataADD + dataLEN > statFileSize) {
                dataLEN = statFileSize - dataADD;
            }

            char *tempDataBuff = malloc(dataLEN * sizeof(char));
            fread(tempDataBuff, dataLEN, sizeof(char), input_fp);
            if (!fseek(out_fp, dataADD, SEEK_SET)) perror(out_file);

            if (!fwrite(tempDataBuff, dataLEN, sizeof(char), out_fp)) perror(out_file);
            memset(tempDataBuff, 0x00, dataLEN);

            char *tempNullBuff = malloc(nullLEN * sizeof(char));
            memset(tempNullBuff, 0x00, nullLEN);
            if (!fseek(out_fp, nullADD, SEEK_SET)) perror(out_file);
            if (!fwrite(tempNullBuff, nullLEN, sizeof(char), out_fp)) perror(out_file);

            if (((dataADD + dataLEN) > statFileSize) || ((nullADD + nullLEN) > statFileSize) || (feof(map_fp))) {
                fclose(map_fp);
                fclose(input_fp);
                fclose(out_fp);
                return 0;
            } else {
                dataADD = (nullADD + nullLEN);
            }
        }
    }

    return 0;
}

int free_data(fragments * frags) {
    return 0;
}