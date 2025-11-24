#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 256 // Maximal supported line size
#define LINE_FORMAT "%d %*s %*s %lf %lf %lf %lf" // Read everything important and ignore both IPs ( %*s )


typedef struct coordinates {
    double bytes;
    double time;
    double delay;
    double size;
} Coords;

typedef struct flow_cluster {
    int cardinality;
    int size;
    int *id_arr;
    Coords core;
} Cluster;

typedef struct cluster_array {
    int count;
    Cluster **arr;
} ClusterArray;

Cluster *cluster_ctor(int size) {
    if(size < 1) {
        fprintf(stderr, "Warning: ctor received size < 1, proceeding with size = 1");
        size = 1;
    }
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
    cluster->size = size;
    return cluster;
}

void cluster_dtor(Cluster *cluster) {
    free(cluster->id_arr);
    free(cluster);
}

int cluster_reserve(Cluster *cluster, int size) {
    int *tmp = realloc(cluster->id_arr, size * sizeof(int));
    if(tmp == NULL) {
        fprintf(stderr, "Error: realloc failed\n");
        return EXIT_FAILURE;
    }
    cluster->id_arr = tmp;
    return EXIT_SUCCESS;

}

void cluster_init(Cluster *cluster, int id, Coords point) {
    cluster->cardinality = 1;
    cluster->id_arr[0] = id;
    cluster->core.bytes = point.bytes;
    cluster->core.time = point.time;
    cluster->core.delay = point.delay;
    cluster->core.size = point.size;
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
    result->core.bytes = (d_size1 * cluster1->core.bytes - d_size2 * cluster2->core.bytes) / d_size;
    result->core.time = (d_size1 * cluster1->core.time - d_size2 * cluster2->core.time) / d_size;
    result->core.delay = (d_size1 * cluster1->core.delay - d_size2 * cluster2->core.delay) / d_size;
    result->core.size = (d_size1 * cluster1->core.size - d_size2 * cluster2->core.size) / d_size;

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
    if(fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "fgets failed!\n");
        return 0;
    }
    if(sscanf(line, "count=%d\n", &count) != 1) {
        fprintf(stderr, "Invalid file format, failed to read count!\n");
    }
    return count;
}

int parse_line(char *line, int *id, Coords *point) {
    double packet_count = 0;
    int result = sscanf(line, LINE_FORMAT,
        id,
        &point->bytes,
        &point->time,
        &packet_count,
        &point->delay
    );
    if(result != 5) {
        fprintf(stderr, "sscanf failed!\n");
        return result;
    }
    if(packet_count == 0) {
        point->size = point->bytes;
    } else {
    point->size = point->bytes / packet_count;
    }
    return result;
}

int data_read_line(FILE *file, Cluster *cluster) {
    char line[LINE_SIZE] = "";
    if(fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "fgets returned NULL\n");
        return EXIT_FAILURE;
    }
    int id = 0;
    Coords coords;
    if(parse_line(line, &id, &coords) != 5) {
        fprintf(stderr, "Line read failed because of failed parse_line!\n");
        return EXIT_FAILURE;
    }
    cluster_init(cluster, id, coords);
    return EXIT_SUCCESS;
}

ClusterArray *cluster_arr_ctor(int count, int size) {
    ClusterArray *cluster_array = malloc(sizeof(ClusterArray));
    if(cluster_array == NULL) {
        fprintf(stderr, "ClusterArray allocation failed!\n");
        return NULL;
    }
    cluster_array->arr = calloc(count, sizeof(Cluster));
    if(cluster_array->arr == NULL) {
        fprintf(stderr, "ClusterArray array allocation failed!\n");
        free(cluster_array);
        return NULL;
    }
    cluster_array->count = count;
    for(int i = 0; i < count; i++) {
        cluster_array->arr[i] = cluster_ctor(size);
        if(cluster_array->arr[i] == NULL) {
        fprintf(stderr, "ClusterArray cluster allocation failed!\n");
        for(int j = i - 1; j >= 0; j--) {
            free(cluster_array->arr[j]);
        }
        return NULL;
        }
    }
    return cluster_array;
}

int cluster_arr_init(FILE *file, ClusterArray *cluster_arr) {
    for(int i = 0; i < cluster_arr->count; i++) {
        if(data_read_line(file, cluster_arr->arr[i]) == EXIT_FAILURE) {
            fprintf(stderr, "Cluster array initialization failed when reading line!\n");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// test function
void print_coords(Coords coord) {
    fprintf(stdout, "Coords: [%lf, %lf, %lf, %lf]\n", coord.bytes, coord.time, coord.delay, coord.size);
}

void debug_print_cluster_array(ClusterArray *cluster_array) {
    for(int i = 0; i < cluster_array->count; i++) {
        Cluster *cluster = cluster_array->arr[i];
        fprintf(stdout, "Cluster[%d]:\n   ID: %d\n   Cardinality: %d\n   ", i, cluster->id_arr[0], cluster->cardinality);
        print_coords(cluster->core);
    }
}

void print_clusters(ClusterArray *cluster_array) {
    fprintf(stdout, "Clusters:\n");
    for(int i = 0; i < cluster_array->count; i++) {
        Cluster *cluster = cluster_array->arr[i];
        fprintf(stdout, "Cluster %d:", i);
        for(int j = 0; j < cluster->cardinality; j++) {
            fprintf(stdout, " %d", cluster->id_arr[j]);
        }
        putchar('\n');
    }
}

double cluster_dist(Cluster *cluster1, Cluster *cluster2, Coords w_vect) {
    double dist_bytes = cluster1->core.bytes - cluster2->core.bytes;
    dist_bytes *= dist_bytes;
    double dist_time = cluster1->core.time - cluster2->core.time;
    dist_time *= dist_time;
    double dist_delay = cluster1->core.delay - cluster2->core.delay;
    dist_delay *= dist_delay;
    double dist_size = cluster1->core.size - cluster2->core.size;
    dist_size *= dist_size;

    double dist = w_vect.bytes * dist_bytes + w_vect.time * dist_time +
                  w_vect.delay * dist_delay + w_vect.size * dist_size;
    return dist;
}

int main(int argc, char **argv) {
    // DEBUG ///////////////////////////////////////////////////////
    fprintf(stdout, "Argc: %d\n", argc);
    for(int i = 0; i < argc; i++) {
        fprintf(stdout, "Argv[%d]: %s\n", i, argv[i]);
    }

        Coords w_vector = {
        .bytes = strtod(argv[3], NULL),
        .time = strtod(argv[4], NULL),
        .delay = strtod(argv[5], NULL),
        .size = strtod(argv[6], NULL)
        };

////////////////////////////////////////////////////////////////////

    if(argc != 2 && argc != 7) {
        fprintf(stderr, "Invalid input!\n");
        return EXIT_FAILURE;
    }

    // initialize clusters
    FILE *file = fopen(argv[1], "r");
    if(file == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    int count = data_read_init(file);

    ClusterArray *cluster_arr = cluster_arr_ctor(count, 1);
    cluster_arr_init(file, cluster_arr);
    if(cluster_arr == NULL) {
        // TODO
        return EXIT_FAILURE;
    }
    if(argc == 7) {
        int count = atoi(argv[2]);
        Coords w_vector = {
        .bytes = strtod(argv[3], NULL),
        .time = strtod(argv[4], NULL),
        .delay = strtod(argv[5], NULL),
        .size = strtod(argv[6], NULL)
        };
        
        // join them

        // DEBUG ///////////////////////////////////////////////////
        fprintf(stdout, "Resulting number of clusters: %d\n", count);
        print_coords(w_vector);
    }   
    

    // print result
    // debug_print_cluster_array(cluster_arr);
    fprintf(stdout, "Distance 10 and 11: %lf\n", cluster_dist(cluster_arr->arr[0], cluster_arr->arr[1], w_vector));
    fprintf(stdout, "Distance 10 and 12: %lf\n", cluster_dist(cluster_arr->arr[0], cluster_arr->arr[2], w_vector));
    fprintf(stdout, "Distance 10 and 13: %lf\n", cluster_dist(cluster_arr->arr[0], cluster_arr->arr[3], w_vector));
    fprintf(stdout, "Distance 11 and 12: %lf\n", cluster_dist(cluster_arr->arr[1], cluster_arr->arr[2], w_vector));
    fprintf(stdout, "Distance 11 and 13: %lf\n", cluster_dist(cluster_arr->arr[1], cluster_arr->arr[3], w_vector));
    fprintf(stdout, "Distance 12 and 13: %lf\n", cluster_dist(cluster_arr->arr[2], cluster_arr->arr[3], w_vector));
    print_clusters(cluster_arr);
    return EXIT_SUCCESS;
}