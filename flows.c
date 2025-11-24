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

typedef struct flow {
    int id;
    Coords val;
} Flow;

typedef struct flow_distance {
    int ids[2];
    double dist;
} FlowDist;

typedef struct cluster {
    int cardinality;
    int capacity;
    int *id_arr;
} Cluster;

typedef struct cluster_array {
    int size;
    Cluster **arr;
} ClusterArr;

int factorial(int n) {
    if(n == 0) return 1;
    return n*factorial(n-1);
}

void flow_init(Flow *flow, int id, Coords coords) {
    flow->id = id;
    flow->val.bytes = coords.bytes;
    flow->val.time = coords.time;
    flow->val.delay = coords.delay;
    flow->val.size = coords.size;
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

int data_read_line(FILE *file, Flow *flow) {
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
    flow_init(flow, id, coords);
    return EXIT_SUCCESS;
}

double flow_dist_calc(Flow *flow1, Flow *flow2, Coords w_vect) {
    double dist_bytes = flow1->val.bytes - flow2->val.bytes;
    dist_bytes *= dist_bytes;
    double dist_time = flow1->val.time - flow2->val.time;
    dist_time *= dist_time;
    double dist_delay = flow1->val.delay - flow2->val.delay;
    dist_delay *= dist_delay;
    double dist_size = flow1->val.size - flow2->val.size;
    dist_size *= dist_size;

    double dist = w_vect.bytes * dist_bytes + w_vect.time * dist_time +
                  w_vect.delay * dist_delay + w_vect.size * dist_size;
    return dist;
}

FlowDist *flowdist_arr_ctor(int flow_count) {
    FlowDist *arr = calloc(factorial(flow_count), sizeof(FlowDist));
    if(arr == NULL) {
        fprintf(stderr, "FlowDist array allocation failed!\n");
        return NULL;
    }
    return arr;
}

FlowDist *find_distances(Flow *flow_arr, Coords w_vect, int count) {
    FlowDist *arr = flowdist_arr_ctor(count);
    if(arr == NULL) {
        fprintf(stderr, "FlowDist array allocation failed!\n");
        return NULL;
    }
    int idx = 0;
    for(int i = 0; i < count - 1; i++) {
        for(int j = i + 1; j < count; j++){
            double dist = flow_dist_calc(&flow_arr[i], &flow_arr[j], w_vect);
            arr[idx].dist = dist;
            arr[idx].ids[0] = i;
            arr[idx].ids[1] = j;
            idx++;
        }
    }
    return arr;
}

Cluster *cluster_ctor(int capacity) {
    if(capacity < 1) {
        fprintf(stderr, "Warning: ctor received size < 1, proceeding with size = 1\n");
        capacity = 1;
    }
    Cluster *cluster = malloc(sizeof(Cluster));
    if(cluster == NULL){
        fprintf(stderr, "Cluster allocation failed!\n");
        return NULL;
    }
    cluster->id_arr = calloc(capacity, sizeof(int));
    if(cluster->id_arr == NULL) {
        free(cluster);
        fprintf(stderr, "Cluster allocation failde, could not allocate id_arr!\n");
        return NULL;
    }
    cluster->capacity = capacity;
    return cluster;
}

void cluster_dtor(Cluster *cluster) {
    free(cluster->id_arr);
    free(cluster);
}

int cluster_reserve(Cluster *cluster, int capacity) {
    int *tmp = realloc(cluster->id_arr, capacity * sizeof(int));
    if(tmp == NULL) {
        fprintf(stderr, "Error: realloc failed\n");
        return EXIT_FAILURE;
    }
    cluster->id_arr = tmp;
    return EXIT_SUCCESS;
}

void flow_to_cluster(Flow *flow, Cluster *cluster) {
    cluster->cardinality = 1;
    cluster->id_arr[0] = flow->id;
}

ClusterArr *cluster_array_ctor(int size) {
    ClusterArr *cluster_arr = malloc(sizeof(ClusterArr));
    if(cluster_arr == NULL){
        fprintf(stderr, "ClusterArray allocation failed!\n");
        return NULL;
    }
    cluster_arr->arr = calloc(size, sizeof(Cluster));
    if(cluster_arr->arr == NULL) {
        fprintf(stderr, "ClusterArray array allocation failed!\n");
        free(cluster_arr);
        return NULL;
    }
    for(int i = 0; i < size; i++) {
        cluster_arr->arr[i] = cluster_ctor(1);
        if(cluster_arr->arr[i] == NULL) {
            for(int j = i - 1; j >= 0; j--) {
                free(cluster_arr->arr[i]);
            }
            free(cluster_arr->arr);
            free(cluster_arr);
            return NULL;
        }
    }
    cluster_arr->size = size;
    return cluster_arr;
}

// Veeeeeery unsafe!!!!
void int_copy_into(int *src, int *dst, int pos, int count) {
    for(int i = 0; i < count; i++) {
        dst[pos + i] = src[i];
    }
}

int cluster_merge_into(Cluster *cluster1, Cluster *cluster2) {
    if(cluster1->capacity < (cluster1->cardinality + cluster2->cardinality)) {
        if(cluster_reserve(cluster1, cluster1->cardinality + cluster2->cardinality) == EXIT_FAILURE) {
            return EXIT_FAILURE;
        }
    }
    // Look at this again!!!
    int_copy_into(cluster2->id_arr, cluster1->id_arr, cluster1->cardinality, cluster2->cardinality);
    //
    cluster1->cardinality += cluster2->cardinality;
    cluster_dtor(cluster2);
    return EXIT_SUCCESS;
}

int cluster_arr_fill_holes(ClusterArr *cluster_arr, int hole_id) {
    cluster_arr->size--;
    for(int i = hole_id; i < cluster_arr->size; i++) {
        cluster_arr->arr[i] = cluster_arr->arr[i + 1];
    }
    return EXIT_SUCCESS;
}

void cluster_merge_in_array(ClusterArr *cluster_array, int flow_ids[2]) {
    int found = 0;
    int cluster_ids[2];
    for(int i = 0; found < 2 && i < cluster_array->size; i++) {
        for(int j = 0; j < cluster_array->arr[i]->cardinality; j++) {
            int tmp_id = cluster_array->arr[i]->id_arr[j]; 
            if(tmp_id == flow_ids[0] || tmp_id == flow_ids[1]) {
                cluster_ids[found] = i;
                found++;
            }
        }
    }
    // check return value
    cluster_merge_into(cluster_array->arr[cluster_ids[0]], cluster_array->arr[cluster_ids[1]]);
    cluster_arr_fill_holes(cluster_array, cluster_ids[1]);
}

int find_closest_dist(FlowDist *dist_arr, int count, int *closest_ids) {
    double min = dist_arr[0].dist;
    int min_id = 0;
    for(int i = 1; i < count; i++) {
        if(dist_arr[i].dist < min && dist_arr[i].dist != (double)-1) {
            min = dist_arr[i].dist;
            min_id = i;
            closest_ids[0] = dist_arr->ids[0];
            closest_ids[1] = dist_arr->ids[1];
        }
    }
    return min_id;
}

void merge_closest_in_arr(ClusterArr *clusters, FlowDist *distances, int count) {
    int closest_flows[2];
    int dist_id = find_closest_dist(distances, count, closest_flows);
    cluster_merge_in_array(clusters, closest_flows);
    distances[dist_id].dist = -1;
}

void print_clusters(ClusterArr *cluster_array) {
    fprintf(stdout, "Clusters:\n");
    for(int i = 0; i < cluster_array->size; i++) {
        Cluster *cluster = cluster_array->arr[i];
        fprintf(stdout, "Cluster %d:", i);
        for(int j = 0; j < cluster->cardinality; j++) {
            fprintf(stdout, " %d", cluster->id_arr[j]);
        }
        putchar('\n');
    }
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
    FILE *file = fopen(argv[1], "r");
    if(file == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    int count = data_read_init(file);
    Flow flow_arr[count];
    ClusterArr *cluster_arr = cluster_array_ctor(count);
    for(int i = 0; i < count; i++) {
        data_read_line(file, &flow_arr[i]);
        flow_to_cluster(&flow_arr[i], cluster_arr->arr[i]);
    }
    
    if(argc == 7) {
        int f_count = atoi(argv[2]);
        Coords w_vector = {
        .bytes = strtod(argv[3], NULL),
        .time = strtod(argv[4], NULL),
        .delay = strtod(argv[5], NULL),
        .size = strtod(argv[6], NULL)
        };
        
        FlowDist *distances = find_distances(flow_arr, w_vector, count);
        (void)distances;
        (void)f_count;
        
        // join them

    }
    print_clusters(cluster_arr);
    return EXIT_SUCCESS;
}