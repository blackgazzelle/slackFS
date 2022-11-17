#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <erasurecode.h>

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

    // Read in a file and store that data
    puts("##############READ IN DATA AND DATA SIZE################");
    char * orig_data;
    uint64_t orig_data_size;
    uint64_t read_bytes = 0;
    FILE* fp = fopen("ToDo.md", "rb");
    if (fp == NULL) {
        puts("Bad file");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    orig_data_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    printf("Original Data Size is %ld\n", orig_data_size);

    orig_data = malloc(orig_data_size);
    fread(orig_data, orig_data_size, 1, fp);

    printf("Data read-in was\n%s\n\n\n\n\n", orig_data);

    // Call liberasurecode_encode to actualy encode the data now
    puts("###############ENCODING THE DATA################");
    char **encoded_data, **encoded_parity;
    uint64_t fragment_len;
    int ret = liberasurecode_encode(instance_descriptor, orig_data, orig_data_size, &encoded_data, &encoded_parity, &fragment_len);
    printf("Encoding was a %s, code was: %d\n", ret ? "Failure" : "Success", ret);
    printf("Fragment Len is %ld\n", fragment_len);

    for (int i = 0; i < args.k; i++) {
        printf("Encoded Fragment is %d: ", i);
        for (int j = 0; j < fragment_len; j++) {
            if (j > 0) printf(":");
            printf("%02X", encoded_data[i][j]);
        }
        puts("");
    } 
    printf("\n\n\n\n");
    puts("#################DECODING THE DATA#################");
    char * out_data;
    u_int64_t out_data_len;
    ret = 
    ret = liberasurecode_decode(instance_descriptor, encoded_data, args.k, fragment_len, 0, &out_data, &out_data_len);
    printf("Decoding was a %s, code was: %d\n", ret ? "Failure" : "Success", ret);
    printf("Data size is %ld\nData is\n%s\n", out_data_len, out_data);
    puts("\n\n\n");
    puts("##################TESTING MISSING THE FIRST FRAG################");
    char **missing_1 = encoded_data+1;
    char * out_fragment;
    // ret = liberasurecode_reconstruct_fragment(instance_descriptor, encoded_parity, 8, fragment_len, 0, out_fragment);
    // printf("Reconstruction was a %s, code was: %d\n", ret ? "Failure" : "Success", ret);
    ret = liberasurecode_decode(instance_descriptor, encoded_data+1, args.k-1, fragment_len, 0, &out_data, &out_data_len);
    printf("Decoding was a %s, code was: %d\n", ret ? "Failure" : "Success", ret);
    printf("Data size is %ld\nData is\n%s\n", out_data_len, out_data);
    puts("\n\n\n");
    fclose(fp);
    liberasurecode_decode_cleanup(instance_descriptor, out_data);
    liberasurecode_encode_cleanup(instance_descriptor, encoded_data, encoded_parity);
    free(orig_data);
    liberasurecode_instance_destroy(instance_descriptor);
    return 0;
}