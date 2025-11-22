#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#define IP_LEN 16 // 4 numbers with 3 characters + 3 spaces in between + one \0 char = 16 characters
#define LINE_SIZE 256 // 
#define LINE_FORMAT "%d %15s %15s %d %d %d %f" // IP_LEN is 16 with \0 so without (for sscanf) 15


typedef struct flow {
    int id;
    char *src_ip;
    char *dst_ip;
    int total_bytes;
    int duration;
    int packet_count;
    float avg_interarrival_time;
} Flow;

float flow_packet_size(Flow flow) {
    if(flow.packet_count == 0) return flow.total_bytes;
    float packet_size = flow.total_bytes / flow.packet_count;
    return packet_size;
}

float flow_euc_dist(Flow flow1, Flow flow2, int wb, int wt, int wd, int ws) {
    int b = (flow1.total_bytes - flow2.total_bytes);
    int t = (flow1.duration - flow2.duration);
    float d = (flow1.avg_interarrival_time - flow2.avg_interarrival_time);
    float s = (flow_packet_size(flow1) - flow_packet_size(flow2));

    float distance = wb*(b*b) + wt*(t*t) + wd*(d*d) + ws*(s*s);
    return distance;
}

Flow *flow_arr_ctor(int len) {
    Flow *flow_arr = (Flow *)calloc(len, sizeof(Flow));
    if( flow_arr == NULL) {
        fprintf(stderr, "Failed to allocate flow array!\n");
    } else {
        for(int i = 0; i < len; i++) {
            flow_arr[i].src_ip = (char *)calloc(IP_LEN, sizeof(char));
            flow_arr[i].dst_ip = (char *)calloc(IP_LEN, sizeof(char));
        }
    }
    return flow_arr;
}

int flows_read_init(FILE *file) {
    int count = 0;
    if(fscanf(file, "count=%d\n", &count) != 1) {
        fprintf(stderr, "Invalid file format, failed to read count!\n");
    }
    return count;
}

int flow_read_line(FILE *file, Flow *flow) {
    char line[LINE_SIZE] = "";
    if(fgets(line, sizeof(line), file) == NULL) {
        return EXIT_FAILURE;
    }
    fprintf(stdout, "%s", line);
    sscanf(line, LINE_FORMAT,
        &flow->id,
        flow->src_ip,
        flow->dst_ip,
        &flow->total_bytes,
        &flow->duration,
        &flow->packet_count,
        &flow->avg_interarrival_time
    );
    return EXIT_SUCCESS;
}

int flows_read_file(FILE *file, Flow *flow_arr, int count) {
    for(int i = 0; i < count; i++) {
        if(flow_read_line(file, &flow_arr[i]) == EXIT_FAILURE) {
            fprintf(stderr, "Line read failed on line %d, file read stopped.\n", i+2);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

void print_flow(Flow flow) {
    fprintf(stdout, "ID: %d\n", flow.id);
    fprintf(stdout, "Source IP: %s\n", flow.src_ip);
    fprintf(stdout, "Destination IP: %s\n", flow.dst_ip);
    fprintf(stdout, "Bytes: %d\n", flow.total_bytes);
    fprintf(stdout, "Duration %d\n", flow.duration);
    fprintf(stdout, "Packet count: %d\n", flow.packet_count);
    fprintf(stdout, "Interarrival time: %f\n", flow.avg_interarrival_time);
}

int main(int argc, char **argv) {
    // DEBUG
    #if DEBUG
    fprintf(stdout, "Argc: %d\n", argc);
    for(int i = 0; i < argc; i++) {
        fprintf(stdout, "Argv[%d]: %s\n", i, argv[i]);
    }
    #endif

    FILE *file = fopen(argv[1], "r");
    if(!file) {
        fprintf(stderr, "Failed to open file: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    int flow_count = flows_read_init(file);
    Flow *flow_arr = flow_arr_ctor(flow_count);

    if(flows_read_file(file, flow_arr, flow_count) == EXIT_FAILURE) {
        fprintf(stderr, "File read failed!\n");
        return EXIT_FAILURE;
    }

    fclose(file);

    print_flow(flow_arr[0]);
    print_flow(flow_arr[1]);
    print_flow(flow_arr[2]);
    print_flow(flow_arr[3]);
    return EXIT_SUCCESS;
}