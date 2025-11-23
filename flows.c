#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 256 // Maximal supported line size
#define LINE_FORMAT "%d %*s %*s %d %d %d %lf" // Read everything important and ignore both IPs ( %*s )


typedef struct coordinates {
    double b;
    double t;
    double d;
    double s;
} Coords;

typedef struct flow_cluster {
    int size;
    int *id_arr;
    Coords p;
} Cluster;

typedef struct clusters {
    int count;
    Cluster *arr[];
} Clusters;

Cluster *cluster_ctor(int size) {
    Cluster *cluster = malloc(sizeof(Cluster));
    if(cluster == NULL){
        fprintf(stderr, "Cluster allocation failed!\n");
        return NULL;
    }
    cluster->id_arr = calloc(size, sizeof(int));
    if(cluster->id_arr == NULL) {
        free(cluster);
        fprintf(stderr, "Cluster allocation failde, could not allocate id_arr!\n");
        return NULL;
    }
    return cluster;
}

void cluster_dtor(Cluster *cluster) {
    free(cluster->id_arr);
    free(cluster);
}

Cluster *cluster_init(int id, Coords point) {
    Cluster *cluster = cluster_ctor(1);
    if(cluster == NULL) {
        fprintf(stderr, "Could not initialize cluster, cluster_ctor failed!\n");
        return NULL;
    }
    cluster->size = 1;
    cluster->id_arr[0] = id;
    cluster->p.b = point.b;
    cluster->p.t = point.t;
    cluster->p.d = point.d;
    cluster->p.s = point.s;
    return cluster;
}

Cluster *cluster_merge(Cluster *cluster1, Cluster *cluster2) {
    int size = cluster1->size + cluster2->size;
    Cluster *result = cluster_ctor(size);
    if(result == NULL) {
        fprintf(stderr, "Could not merge clusters, cluster_ctor() failed!\n");
        return NULL;
    }
    result->size = size;
    memcpy(&result->id_arr[0]             , cluster1->id_arr, sizeof(int) * cluster1->size);
    memcpy(&result->id_arr[cluster1->size], cluster2->id_arr, sizeof(int) * cluster2->size);

    double d_size = (double)size;
    double d_size1 = (double)cluster1->size;
    double d_size2 = (double)cluster2->size;
    result->p.b = (d_size1 * cluster1->p.b - d_size2 * cluster2->p.b) / d_size;
    result->p.t = (d_size1 * cluster1->p.t - d_size2 * cluster2->p.t) / d_size;
    result->p.d = (d_size1 * cluster1->p.d - d_size2 * cluster2->p.d) / d_size;
    result->p.s = (d_size1 * cluster1->p.s - d_size2 * cluster2->p.s) / d_size;

    return result;
}

double avg_packet_size(int total_bytes, int packet_count) {
    if(packet_count == 0) return total_bytes;
    double packet_size = (double)total_bytes / (double)packet_count;
    return packet_size;
}

int data_read_init(FILE *file) {
    int count = 0;
    char line[LINE_SIZE] = "";
    if(fgets(line, sizeof(line), file) == NULL)
    if(sscanf(line, "count=%d\n", &count) != 1) {
        fprintf(stderr, "Invalid file format, failed to read count!\n");
    }
    return count;
}

int data_read_line(FILE *file/*, Cluster *cluster*/) {
    char line[LINE_SIZE] = "";
    if(fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "fgets returned NULL\n");
        return EXIT_FAILURE;
    }

    int id = 0;
    int bytes = 0;
    int duration = 0;
    int packet_count = 0;
    double interarrival = 0;
    sscanf(line, LINE_FORMAT,
        &id,
        &bytes,
        &duration,
        &packet_count,
        &interarrival
    );
    return EXIT_SUCCESS;
}

// test function
void print_coord(Coords coord) {
    fprintf(stdout, "Coord: [%lf, %lf, %lf, %lf]\n", coord.b, coord.t, coord.d, coord.s);
}

int main(int argc, char **argv) {
    // DEBUG ///////////////////////////////////////////////////////
    fprintf(stdout, "Argc: %d\n", argc);
    for(int i = 0; i < argc; i++) {
        fprintf(stdout, "Argv[%d]: %s\n", i, argv[i]);
    }

////////////////////////////////////////////////////////////////////

    if(argc != 2 && argc != 7) {
        fprintf(stderr, "Invalid input!\n");
        return EXIT_FAILURE;
    }

    // initialize clusters

    if(argc == 7) {
        int count = atoi(argv[2]);
        Coords w_vector = {
        .b = strtod(argv[3], NULL),
        .t = strtod(argv[4], NULL),
        .d = strtod(argv[5], NULL),
        .s = strtod(argv[6], NULL)
    };
        // join them

        // DEBUG ///////////////////////////////////////////////////
        fprintf(stdout, "Resulting number of clusters: %d\n", count);
        print_coord(w_vector);
    }   
    

    // print result

    return EXIT_SUCCESS;
}