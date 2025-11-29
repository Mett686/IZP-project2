#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 256 // Maximal supported line size
#define FLOW_DIMENSIONS 4 // Number of values in flow
#define ARGV_FILE_ID 1
#define ARGV_COUNT_ID 2
#define ARGV_WEIGHTS_ID 3
#define LINE_FORMAT "%d %*s %*s %lf %lf %lf %lf" // Read everything important and ignore both IPs ( %*s )


typedef struct flow {
    int id;
    double values[FLOW_DIMENSIONS];
} Flow;

typedef struct flow_distance {
    int id1;
    int id2;
    double dist;
} FlowDist;

typedef struct cluster {
    int size;
    int capacity;
    int *id_arr;
} Cluster;

typedef struct cluster_array {
    int size;
    int capacity;
    Cluster **arr;
} ClustArr;

// Recursive calculation of factorial
int factorial(int n);

// Allocates array of Flows (or one flow)
Flow *flow_arr_ctor(int length);

// Frees Flow (just free)
void flow_arr_dtor(Flow *flow_arr);

// Writes values into flow
void flow_write(Flow *flow, int id, int *values);

// Colculates weighted euclidean distance between two flows
// needs to have dimensions defined
double calculate_flow_dist(Flow *flow1, Flow *flow2, double *weights);

// Creates array of FlowDist types that stores distances between all Flows in flow_arr
// n is the number of elements in flow_arr
// needs to have dimensions defined
FlowDist *get_flow_distances(Flow *flow_arr, int n, double *weights);

// Prints flow
void print_flow(Flow *flow);

// Reads header from file, returning number of flows in file
int read_header(FILE *file);

// Parses string into flow
// Need to edit for different flow dimensions
int parse_line(char *line, Flow *flow);

// Reads file line and parses its contents into flow
int read_line(FILE *file, Flow *flow);

// Reads data from file into flow_arr
int read_data(FILE *file, int flow_count, Flow *flow_arr);

// Allocates cluster with given capacity
Cluster *cluster_ctor(int capacity);

// Frees cluster
void cluster_dtor(Cluster *cluster);

// Allocates and fills cluster with id from flow
Cluster *flow_to_cluster(Flow *flow);

// Allocates cluster array and fills it with flow to cluster,
// executed on given flow_arr
ClustArr *initialize_clusters(Flow *flow_arr, int flow_count);

// Prints all clusters in cluster arr
void print_clusters(ClustArr *clusters);

void print_distances(FlowDist *dist_arr, int len) {
    for(int i = 0; i < len; i++) {
        fprintf(stdout, "%d: Distance between %d, %d:  %lf\n", i, dist_arr[i].id1, dist_arr[i].id2, dist_arr[i].dist);
    }
}

// find lowest distance between flows and return its idx in dist_arr
int lowest_dist(FlowDist *dist_arr, int len) {
    double min = dist_arr[0].dist;
    int min_id = 0;
    for(int i = 1; i < len; i++) {
        double value = dist_arr[i].dist;
        if(value < min && value >=0) {
            min = value;
            min_id = i;
        }
    }
    return min_id;
}

Cluster *cluster_merge(Cluster *clust1, Cluster *clust2) {
    int size = clust1->size + clust2->size;
    Cluster *result = cluster_ctor(size);
    if(result == NULL) {
        fprintf(stderr, "Nowhere to merge into\n");
        return NULL;
    }
    result->size = size;
    for(int i = 0; i < clust1->size; i++) {
        result->id_arr[i] = clust1->id_arr[i];
    }
    for(int i = 0; i < clust2->size; i++) {
        result->id_arr[clust1->size + i] = clust2->id_arr[i];
    }
    cluster_dtor(clust1);
    cluster_dtor(clust2);
    return result;
}

int find_flow_id(ClustArr *cluster_arr, int flow_id) {
    for(int i = 0; i < cluster_arr->size; i++) {
        Cluster *cluster = cluster_arr->arr[i];
        for(int j = 0; j < cluster->size; j++) {
            if(cluster->id_arr[j] == flow_id) {
                return i;
            }
        }
    }
    return -1;
}

void cluster_arr_merge(ClustArr *cluster_arr, int id1, int id2) {
    cluster_arr->arr[id1] = cluster_merge(cluster_arr->arr[id1], cluster_arr->arr[id2]);
    for(int i = id2; i < cluster_arr->size - 1; i++) {
        cluster_arr->arr[i] = cluster_arr->arr[i+1];
    }
    cluster_arr->size--;
}

void merge_clusters(ClustArr *cluster_arr, FlowDist *dist_arr, int flow_count, int cluster_count) {
    int merge_count = flow_count - cluster_count;
    int dist_count = factorial(flow_count - 1);
    for(int i = 0; i < merge_count; i++) {
        int lowest_id = lowest_dist(dist_arr, dist_count);
        int f_id1 = dist_arr[lowest_id].id1;
        int f_id2 = dist_arr[lowest_id].id2;

        int c_id1 = find_flow_id(cluster_arr, f_id1);
        int c_id2 = find_flow_id(cluster_arr, f_id2);
        if(c_id1 != c_id2) {
        cluster_arr_merge(cluster_arr, c_id1, c_id2);
        } else {
            i--;
        }
        dist_arr[lowest_id].dist = -1;
    }
}



int main(int argc, char **argv) {
    // DEBUG ///////////////////////////////////////////////////////
    #if 0
    fprintf(stdout, "Argc: %d\n", argc);
    for(int i = 0; i < argc; i++) {
        fprintf(stdout, "Argv[%d]: %s\n", i, argv[i]);
    }
    #endif
    ////////////////////////////////////////////////////////////////

    if(argc != 2 && argc != 7) {
        fprintf(stderr, "Invalid input!\n");
        return EXIT_FAILURE;
    }

    // Read info from file
    FILE *file = fopen(argv[ARGV_FILE_ID], "r");
    int flow_count = read_header(file);
    Flow *flow_arr = flow_arr_ctor(flow_count);
    if(read_data(file, flow_count, flow_arr) == 0) {
        fprintf(stderr, "Error, read 0 lines from file!\n");
        return EXIT_FAILURE;
    }

    ClustArr *cluster_arr = initialize_clusters(flow_arr, flow_count);
    
    if(argc == 7) {
        int cluster_count = atoi(argv[ARGV_COUNT_ID]);
        double weights[FLOW_DIMENSIONS];
        for(int i = 0; i < FLOW_DIMENSIONS; i++) {
            weights[i] = strtod(argv[ARGV_WEIGHTS_ID + i], NULL);
        }

        FlowDist *dist_arr = get_flow_distances(flow_arr, flow_count, weights);
        
        merge_clusters(cluster_arr, dist_arr, flow_count, cluster_count);


        // join them

    }

    print_clusters(cluster_arr);
    return EXIT_SUCCESS;
}

int factorial(int n) {
    if(n == 0) return 1;
    return n*factorial(n-1);
}

Flow *flow_arr_ctor(int length) {
    Flow *flow = calloc(length, sizeof(Flow));
    if(flow == NULL) {
        fprintf(stderr, "Error: flow_create, malloc failed!\n");
        return NULL;
    }
    return flow;
}

void flow_arr_dtor(Flow *flow_arr) {
    free(flow_arr);
}

void flow_write(Flow *flow, int id, int *values) {
    flow->id = id;
    for(int i = 0; i < FLOW_DIMENSIONS; i++) {
        flow->values[i] = values[i];
    }
}

double calculate_flow_dist(Flow *flow1, Flow *flow2, double *weights) {
    double dist = 0;
    for(int i = 0; i < FLOW_DIMENSIONS; i++) {
        double value_dist = flow1->values[i] - flow2->values[i];
        value_dist *= weights[i] * value_dist; // value_dist = weight*(val1 - val2)^2
        dist += value_dist;
    }
    return dist;
}

FlowDist *get_flow_distances(Flow *flow_arr, int n, double *weights) {
    int len = factorial(n-1);
    FlowDist *dist_arr = calloc( len, sizeof(FlowDist));
    int idx = 0;
    for(int i = 0; i < (n - 1); i++) {
        for(int j = (i + 1); j < n; j++) {
            dist_arr[idx].id1 = flow_arr[i].id;
            dist_arr[idx].id2 = flow_arr[j].id;
            dist_arr[idx].dist = calculate_flow_dist(&flow_arr[i], &flow_arr[j], weights);
            idx++;
        }
    }
    return dist_arr;
}

void print_flow(Flow *flow) {
    fprintf(stdout, "Flow ID: %d\n", flow->id);
    for(int i = 0; i < FLOW_DIMENSIONS; i++) {
        fprintf(stdout, "   val[%d]: %lf\n", i, flow->values[i]);
    }
}

int read_header(FILE *file) {
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


// Need to edit for different flow dimensions
int parse_line(char *line, Flow *flow) {
    double packet_count = 0;
    int result = sscanf(line, LINE_FORMAT,
        &flow->id,
        &flow->values[0],
        &flow->values[1],
        &packet_count,
        &flow->values[2]
    );
    if(result != 5) {
        fprintf(stderr, "sscanf failed!\n");
        return EXIT_FAILURE;
    }
    if(packet_count == 0) {
        flow->values[3] = flow->values[0];
    } else {
    flow->values[3] = flow->values[0] / packet_count;
    }
    return EXIT_SUCCESS;
}

int read_line(FILE *file, Flow *flow) {
    char line[LINE_SIZE] = "";
    if(fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "fgets retunred NULL\n");
        return EXIT_FAILURE;
    }
    if(parse_line(line, flow) != EXIT_SUCCESS) {
        fprintf(stderr, "parse line did not return exit succes\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int read_data(FILE *file, int flow_count, Flow *flow_arr) {
    int succesful_reads = 0;
    for(int i = 0; i < flow_count; i++) {
        if(read_line(file, &flow_arr[i]) == EXIT_FAILURE) {
            fprintf(stderr, "Readline failed on line %d\n", i + 2);
        } else {
            succesful_reads += 1;
        }
    }
    return succesful_reads;
}

Cluster *cluster_ctor(int capacity) {
    Cluster *cluster = malloc(sizeof(Cluster));
    if(cluster == NULL) {
        fprintf(stderr, "Cluster allocation failed!\n");
        return NULL;
    }
    cluster->id_arr = calloc(capacity, sizeof(int));
    if(cluster->id_arr == NULL) {
        fprintf(stderr, "Cluster id_arr allocation failed!\n");
        free(cluster);
        return NULL;
    }
    cluster->capacity = capacity;
    return cluster;
}

void cluster_dtor(Cluster *cluster) {
    free(cluster->id_arr);
    free(cluster);
}

Cluster *flow_to_cluster(Flow *flow) {
    Cluster *cluster = cluster_ctor(1);
    if(cluster == NULL) {
        return NULL;
    }
    cluster->id_arr[0] = flow->id;
    cluster->size = 1;
    return cluster;
}

ClustArr *initialize_clusters(Flow *flow_arr, int flow_count) {
    ClustArr *cluster_array = malloc(sizeof(ClustArr));
    if(cluster_array == NULL) {
        fprintf(stderr, "Cluster array allocation failed!\n");
        return NULL;
    }
    cluster_array->arr = calloc(flow_count, sizeof(Cluster *));
    if(cluster_array->arr == NULL) {
        fprintf(stderr, "Cluster_array array allocation failed!\n");
        free(cluster_array);
        return NULL;
    }
    cluster_array->capacity = flow_count;
    for(int i = 0; i < flow_count; i++) {
        cluster_array->arr[i] = flow_to_cluster(&flow_arr[i]);
        if(cluster_array->arr[i] == NULL) {
            fprintf(stderr, "Failed to create cluster from flow at line %d\n", i + 2);
            return NULL;
        }
    }
    cluster_array->size = flow_count;
    return cluster_array;
}

void print_clusters(ClustArr *clusters) {
    fprintf(stdout, "Clusters:\n");
    // for each cluster
    for(int i = 0; i < clusters->size; i++) {
        fprintf(stdout, "Cluster %d:", i);
        // for each id
        Cluster *cluster = clusters->arr[i];
        for(int j = 0; j < cluster->size; j++) {
            fprintf(stdout, " %d", cluster->id_arr[j]);
        }
        putchar('\n');
    }
}
