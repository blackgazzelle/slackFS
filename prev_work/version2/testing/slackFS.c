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
    uint64_t fragment_len = 0;
    uint64_t other_frag_len = 0;
    int ret = liberasurecode_encode(instance_descriptor, orig_data, orig_data_size, &encoded_data, &encoded_parity,
                                    &fragment_len);

    if (ret != 0) {
        fprintf(stderr, "ERROR: encode_file, liberasure_encode: %d\n", ret);
        return NULL;
    }

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
        fprintf(stderr, "Encoding data of size %d\n", frags->info_arr[i]->size);
    }
    // Save off parity frags
    for (int j = 0; j < args.m; j++) {
        frags->info_arr[i] = malloc(sizeof(fragment_info));
        frags->info_arr[i]->data = strndup(encoded_parity[j], fragment_len);
        frags->info_arr[i]->size = fragment_len;
        fprintf(stderr, "Encoding data of size %d\n", frags->info_arr[i]->size);
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
    // char *tmp = "tmp.txt";

    // Loop over each fragment and hide each in a series of cover files, noting each cover file in the struct
    // Open cover file list
    FILE *cover_fp = fopen(cover_files, "r"), *tmp_file;
    if (cover_fp == NULL) {
        perror("ERROR: cannot open cover file list");
        return -errno;
    }

    for (int i = 0; i < frags->num_of_frags; i++) {
        int available_slack = 0, block_size, bytes_written = 0, max_files = 2;
        // Initialize the list of filenames
        frags->info_arr[i]->file_pairs = malloc(max_files * sizeof(file_pair *));
        frags->info_arr[i]->num_of_files = 0;
        do {
            // Get cover file and size
            char *cover_file = malloc(FILENAME_MAX);
            fscanf(cover_fp, "%s%i", cover_file, &available_slack);
            struct stat st_cover;

            int size_of_write = 0;
            if (available_slack < frags->info_arr[i]->size) {
                size_of_write = available_slack;
            } else {
                size_of_write = frags->info_arr[i]->size;
            }

            // Make sure cover file is a regular file
            if ((stat(cover_file, &st_cover) == -1) && S_ISREG(st_cover.st_mode) != 0) {
                free(cover_file);
                continue;
            }

            // Write to tmp file
            char *tmp = malloc(CMD_LEN);
            sprintf(tmp, "frags/tmp_e_%d_%d", i, frags->info_arr[i]->num_of_files);

            tmp_file = fopen(tmp, "w");
            fwrite(frags->info_arr[i]->data + bytes_written, size_of_write, 1, tmp_file);
            fclose(tmp_file);

            // Use bmap and write the fragment to the slack space
            block_size = size_of_write;
            sprintf(cmd,
                    "dd iflag=skip_bytes count=1 if=%s bs=%d | "
                    "sudo bmap --mode putslack %s",
                    tmp, block_size, cover_file);
            ret = system(cmd);
            if (!ret) {
                // Account for hiding available slack amount of the fragment
                fprintf(stderr, "Hiding %d amount\n", size_of_write);
                bytes_written += size_of_write;

                // Note the cover file we wrote to and update the amount of files in use
                frags->info_arr[i]->file_pairs[frags->info_arr[i]->num_of_files] = malloc(sizeof(file_pair));
                frags->info_arr[i]->file_pairs[frags->info_arr[i]->num_of_files]->filename = strdup(cover_file);
                frags->info_arr[i]->file_pairs[frags->info_arr[i]->num_of_files]->size_written = size_of_write;
                frags->info_arr[i]->file_pairs[frags->info_arr[i]->num_of_files]->available_slack = available_slack;

                frags->info_arr[i]->num_of_files++;
                if (frags->info_arr[i]->num_of_files >= max_files) {
                    max_files *= 2;
                    frags->info_arr[i]->file_pairs =
                        realloc(frags->info_arr[i]->file_pairs, max_files * sizeof(file_pair *));
                }

            } else {
                fprintf(stderr, "%s Command failed with code: %d", cmd, ret);
            }

            free(cover_file);
        } while (bytes_written < frags->info_arr[i]->size);
    }

    for (int i = 0; i < frags->num_of_frags; i++) {
        char *hash_cmd = malloc(CMD_LEN);
        sprintf(hash_cmd, "hashes/encode_%d", i);
        FILE *hash_file = fopen(hash_cmd, "w");
        fwrite(frags->info_arr[i]->data, frags->info_arr[i]->size, 1, hash_file);
        free(hash_cmd);
        fclose(hash_file);
    }

    // Write data about cover files to a mapping file to be read later
    FILE *map_file = fopen("hiding_map.txt", "w");
    fprintf(map_file, "#\tfrag_size\tnum_of_files\tfilename\tsize_written\tavailable_slack\n");
    for (int i = 0; i < frags->num_of_frags; i++) {
        fprintf(map_file, "%d\t%d\t%d\t", i, frags->info_arr[i]->size, frags->info_arr[i]->num_of_files);
        for (int j = 0; j < frags->info_arr[i]->num_of_files; j++) {
            fprintf(map_file, "%s\t", frags->info_arr[i]->file_pairs[j]->filename);
            fprintf(map_file, "%d\t", frags->info_arr[i]->file_pairs[j]->size_written);
            fprintf(map_file, "%d\t", frags->info_arr[i]->file_pairs[j]->available_slack);
        }
        fprintf(map_file, "\n");
    }

    fclose(cover_fp);
    free(cmd);
    return 0;
}

fragments *retrieve_file(char *map_file, int num_frags) {
    char *cmd = malloc(CMD_LEN);
    char *tmp_str = malloc(CMD_LEN);
    // char *tmp_file = "tmp.txt";
    int ret;

    // Read in frags structure from map_file
    fragments *frags = malloc(sizeof(fragments));
    frags->num_of_frags = num_frags;
    frags->info_arr = malloc(sizeof(fragment_info *) * num_frags);
    int frag_num, num_of_files, frag_size, block_size, skip_size = 0, size_written = 0;
    FILE *map_fp = fopen(map_file, "r");
    fscanf(map_fp, "%s%s%s%s%s%s", tmp_str, tmp_str, tmp_str, tmp_str, tmp_str, tmp_str);
    while (fscanf(map_fp, "%d%d%d", &frag_num, &frag_size, &num_of_files) != EOF) {
        // set up file pairs and fragment_info
        frags->info_arr[frag_num] = malloc(sizeof(fragment_info));
        frags->info_arr[frag_num]->file_pairs = malloc(num_of_files * sizeof(file_pair *));
        frags->info_arr[frag_num]->size = frag_size;
        frags->info_arr[frag_num]->data = malloc(frag_size + 1);
        frags->info_arr[frag_num]->num_of_files = num_of_files;

        // Read in all of the file pairs and store that data
        for (int i = 0; i < num_of_files; i++) {
            frags->info_arr[frag_num]->file_pairs[i] = malloc(sizeof(file_pair));
            frags->info_arr[frag_num]->file_pairs[i]->filename = malloc(FILENAME_MAX);
            fscanf(map_fp, "%s%d%d", frags->info_arr[frag_num]->file_pairs[i]->filename,
                   &frags->info_arr[frag_num]->file_pairs[i]->size_written,
                   &frags->info_arr[frag_num]->file_pairs[i]->available_slack);

            char *tmp_file = malloc(CMD_LEN);
            sprintf(tmp_file, "frags/tmp_d_%d_%d", frag_num, i);

            fprintf(stderr, "Reading in file %s, for frag size of %d\n",
                    frags->info_arr[frag_num]->file_pairs[i]->filename,
                    frags->info_arr[frag_num]->file_pairs[i]->size_written);
            // Get data from the file and add to data array
            block_size = frags->info_arr[frag_num]->file_pairs[i]->size_written;
            sprintf(cmd,
                    "sudo bmap --mode slack %s %d | dd of=%s "
                    "oflag=seek_bytes count=1 bs=%d conv=notrunc",
                    frags->info_arr[frag_num]->file_pairs[i]->filename, frag_size, tmp_file, block_size);
            ret = system(cmd);
            fprintf(stderr, "RET: %d CMD: %s\n", ret, cmd);
            if (!ret) {
                // Read from tmp file and store in array

                FILE *tmp_fp = fopen(tmp_file, "rb");
                if (tmp_fp == NULL) {
                    perror("ERROR: cannot temp file");
                    return NULL;
                }
                fread(frags->info_arr[frag_num]->data + size_written,
                      frags->info_arr[frag_num]->file_pairs[i]->size_written, 1, tmp_fp);

                fclose(tmp_fp);
                // remove(tmp_file);
            }
        }
    }

    for (int i = 0; i < frags->num_of_frags; i++) {
        char *hash_cmd = malloc(CMD_LEN);
        sprintf(hash_cmd, "hashes/retrieve_%d", i);
        FILE *hash_file = fopen(hash_cmd, "w");
        fwrite(frags->info_arr[i]->data, frags->info_arr[i]->size, 1, hash_file);
        free(hash_cmd);
        fclose(hash_file);
    }

    free(cmd);
    return frags;
}

int file_decode(char *out_file, fragments *frags, int backend_id, int num_frags, int num_parity, int checksum) {
    // Set up backend
    ec_backend_id_t id = backend_id;
    // Creating a new instance of the erasure coder
    struct ec_args args;
    args.k = num_frags;
    args.m = num_parity;
    args.hd = num_parity;
    args.ct = checksum;

    int instance_descriptor = liberasurecode_instance_create(id, &args);
    // Combine all the data into one array
    char **data = malloc(sizeof(char *) * frags->num_of_frags);
    for (int i = 0; i < frags->num_of_frags; i++) {
        data[i] = frags->info_arr[i]->data, frags->info_arr[i]->size;
        char *hash_cmd = malloc(CMD_LEN);
        sprintf(hash_cmd, "hashes/decode_%d", i);
        FILE *hash_file = fopen(hash_cmd, "w");
        fwrite(frags->info_arr[i]->data, frags->info_arr[i]->size, 1, hash_file);
        free(hash_cmd);
        fclose(hash_file);
        fprintf(stderr, "Is valid frag:%d\n", is_invalid_fragment(instance_descriptor, data[i]));
    }

    char *out_data;
    u_int64_t out_data_len;

    fprintf(stderr, "num: %d\n", frags->num_of_frags);
    fprintf(stderr, "metadata check: %d\n",
            liberasurecode_verify_stripe_metadata(instance_descriptor, data, frags->num_of_frags));
    int ret = liberasurecode_decode(instance_descriptor, data, frags->num_of_frags, frags->info_arr[0]->size, 1,
                                    &out_data, &out_data_len);

    if (ret != 0) {
        fprintf(stderr, "ERROR: File Decode: %d\n", ret);
        return ret;
    }
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

int free_data(fragments *frags) { return 0; }