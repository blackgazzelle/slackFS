#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <erasurecode.h>


void free_data(char **data, int num_frags) {
    for (int i = 0; i < num_frags; i++) {
        free(data[i]);
    }
    free(data);
}

int main() {

    puts("###############SETTING UP ERASURE CODE INSTANCE###############");
    // Setting backend to Jerasure_RS_VAND 
    ec_backend_id_t id = 1;
    printf("Backend ID %d is available %s\n", id, liberasurecode_backend_available(id) ? "true" : "false");

    // Creating a new instance of the erasure coder
    struct ec_args args;
    args.k = 16;
    args.m = 8;
    args.w = 8;
    args.hd = args.m;
    int instance_descriptor = liberasurecode_instance_create(id, &args);
    printf("Instance is: %d\n", instance_descriptor);
    printf("K=%d, M=%d, W=%d\n\n\n\n\n", args.k, args.m, args.w);

    // Read in a files and store that data
    puts("##############READ IN DATA FRAGS################");
    struct dirent *de;
    DIR *dr = opendir("test");
    int fragment_len = 91;
    int num_frags = 0;
    if (dr == NULL) {
        puts("Could not open directory");
        return -1;
    }
    // get number of files in directory
    while ((de = readdir(dr)) != NULL) {
        if (de->d_type == DT_REG) {
            num_frags++;
        }
    } 

    closedir(dr);

    dr = opendir("test");
    if (dr == NULL) {
        puts("Could not open directory");
        return -1;
    }

    // Read each file in directory and insert that into our data
    char **data = calloc(num_frags, sizeof(char*));
    int i = 0;
    while ((de = readdir(dr)) != NULL) {
        if (de->d_type == DT_REG) {
            printf("On file %s\n", de->d_name);
            char filename[24];
            sprintf(filename, "test/%s", de->d_name);
            FILE * tmp_fp = fopen(filename, "r+");
            if (tmp_fp == NULL) {
                printf("Bad FILE\n");
                return -1;
            }
            data[i] = malloc(fragment_len+1);
            fread(data[i], fragment_len, 1, tmp_fp);
            fclose(tmp_fp);
            i++;
        }
    }
    puts("");
    for (i = 0; i < num_frags; i++) {
        printf("Encoded Fragment is %d: ", i);
        for (int j = 0; j < fragment_len; j++) {
            if (j > 0) printf(":");
            printf("%02X", data[i][j]);
        }
        puts("");
    }
    closedir(dr);

    puts("\n\n\n");
    puts("#################DECODING THE DATA#################");
    char * out_data;
    u_int64_t out_data_len;
    int ret = liberasurecode_decode(instance_descriptor, data, num_frags, fragment_len, 0, &out_data, &out_data_len);
    printf("Decoding was a %s, code was: %d\n", ret ? "Failure" : "Success", ret);
    printf("Data size is %ld\nData is\n%s\n", out_data_len, out_data);


    free_data(data, num_frags);
    liberasurecode_decode_cleanup(instance_descriptor, out_data);
    liberasurecode_instance_destroy(instance_descriptor);
    return 0;
}