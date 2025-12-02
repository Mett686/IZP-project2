// TODO
// check for duplicit flows IDs
// sort output, maybe?
// revisit merging

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 256 // Maximal supported line size
#define FLOW_DIMENSIONS 4 // Number of values in flow
#define ARGV_FILE_ID 1 // Position in argv where path to data is stored
#define ARGV_COUNT_ID 2 // Position in argv where number of resulting clusters is stored
#define ARGV_WEIGHTS_ID 3 // Position in argv where weights for euclidian distance are stored
#define LINE_FORMAT "%d %*s %*s %lf %lf %lf %lf" // Format of one entry in data, ignores both IPs ( %*s )

// Contains flow id which must be unique
// and values we are interested in comparing
typedef struct flow {
    int id;
    double values[FLOW_DIMENSIONS];
} Flow;

// Structure for storing distance between two flows
typedef struct flow_distance {
    int id1;
    int id2;
    double dist;
} FlowDist;

// Dynamic array of flow IDs
typedef struct cluster {
    int size;
    int capacity;
    int *id_arr;
} Cluster;

// Dynamic array of pointers to Cluster type
typedef struct cluster_array {
    int size;
    int capacity;
    Cluster **arr;
} ClustArr;

// Calculates triangular number
// Needed for calculating number of distances between flows
int triangular_number(int n) {
    return (n*(n + 1))/2;
}

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
void print_flows(Flow *flow_arr, int len);

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

// Prints distances
void print_distances(FlowDist *dist_arr, int len) {
    for(int i = 0; i < len; i++) {
        fprintf(stdout, "%d: Distance between %d, %d:  %lf\n", i, dist_arr[i].id1, dist_arr[i].id2, dist_arr[i].dist);
    }
}

// find lowest distance between flows and return its idx in dist_arr
int lowest_dist(FlowDist *dist_arr, int len) {
    double min = -1;
    int min_id = -1;
    for(int i = 0; i < len; i++) {
        double value = dist_arr[i].dist;
        if((value != -1) && (value <= min || min_id == -1)) {
            min = value;
            min_id = i;
        }
    }
    return min_id;
}

// Merges two clusters into new cluster while dtoring the originals
Cluster *cluster_merge(Cluster *clust1, Cluster *clust2) {
    int size = clust1->size + clust2->size;
    Cluster *result = cluster_ctor(size);
    if(result == NULL) {
        fprintf(stderr, "Nowhere to merge into\n");
        return NULL;
    }
    result->size = size;
    int idx1 = 0;
    int idx2 = 0;
    while((idx1 + idx2) < size) {
        int id1 = clust1->id_arr[idx1];
        int id2 = clust2->id_arr[idx2];
        if(idx1 == clust1->size) {
            result->id_arr[idx1+idx2] = id2;
            idx2++;
            continue;
        }
        if(idx2 == clust2->size) {
            result->id_arr[idx1+idx2] = id1;
            idx1++;
            continue;
        }
        if(id1 < id2) {
            result->id_arr[idx1+idx2] = id1;
            idx1++;
        } else {
            result->id_arr[idx1+idx2] = id2;
            idx2++;
        }

    }
    cluster_dtor(clust1);
    cluster_dtor(clust2);
    return result;
}

// Return index of cluster containing flow_id 
int find_flow_id(ClustArr *cluster_arr, int flow_id) {
    for(int i = 0; i < cluster_arr->size; i++) {
        Cluster *cluster = cluster_arr->arr[i];
        for(int j = 0; j < cluster->size; j++) {
            if(cluster->id_arr[j] == flow_id) {
                return i;
            } else if(flow_id < cluster->id_arr[j]) {
                j=cluster->size;
            }
        }
    }
    return -1;
}

// Merges two clusters on given position in array storing it on id1
void cluster_arr_merge(ClustArr *cluster_arr, int id1, int id2) {
    cluster_arr->arr[id1] = cluster_merge(cluster_arr->arr[id1], cluster_arr->arr[id2]);
    // move clusters to fill empty space
    for(int i = id2; i < cluster_arr->size - 1; i++) {
        cluster_arr->arr[i] = cluster_arr->arr[i+1];
    }
    cluster_arr->size--;
}

// Merges clusters till theres only cluster_count of them
void merge_clusters(ClustArr *cluster_arr, FlowDist *dist_arr, int flow_count, int cluster_count) {
    int merge_count = flow_count - cluster_count;
    for(int i = 0; i < merge_count; i++) {


        int c_id1 = find_flow_id(cluster_arr, dist_arr[i].id1);
        int c_id2 = find_flow_id(cluster_arr, dist_arr[i].id2);
        if(c_id1 != c_id2) {
        cluster_arr_merge(cluster_arr, c_id1, c_id2);
        } else {
            i--;
        }
    }
}

// Compares flow ids and returns their delta
int cmp_flow_id(const void *flow1, const void *flow2) {
    const Flow *flow1_p = (const Flow *)flow1;
    const Flow *flow2_p = (const Flow *)flow2;
    return flow1_p->id - flow2_p->id;
}

#if 1
FlowDist *create_instructions(FlowDist *dist_arr, int flow_count, int merge_count) {
    FlowDist *inst = calloc(merge_count, sizeof(FlowDist));
    if(inst == NULL) {
        fprintf(stderr, "FlowDist instruction array allocation failed!\n");
        return NULL;
    }
    int dist_count = triangular_number(flow_count-1);
    int inst_id = 0;

    for(int i = 0; i < dist_count; i++) {
        double value = dist_arr[i].dist;

        // If value is lower than that of biggest member
        if(value < inst[inst_id].dist || i < merge_count) {
            // iterate from end
            int j = inst_id;
            while(j > 0 && value < inst[j-1].dist) {
                inst[j] = inst[j-1];
                j--;
            }
            inst[j] = dist_arr[i];
            if(inst_id < merge_count-1) inst_id++;
        }
    }
    return inst;
}
#endif



int main(int argc, char **argv) {
    // Check for correct arguments
    if( (argc != 2 && argc != 7)) {
        fprintf(stderr, "Invalid input!\n");
        return EXIT_FAILURE;
    }

    // Open file and read header
    FILE *file = fopen(argv[ARGV_FILE_ID], "r");
    int flow_count = read_header(file);
    
    // Read data from file
    Flow *flow_arr = flow_arr_ctor(flow_count);
    if(read_data(file, flow_count, flow_arr) == 0) {
        fprintf(stderr, "Error, read 0 lines from file!\n");
        return EXIT_FAILURE;
    }

    // Sort flows by ids and then initialize clusters that will be correctly ordered
    qsort(flow_arr, flow_count, sizeof(Flow), cmp_flow_id);
    ClustArr *cluster_arr = initialize_clusters(flow_arr, flow_count);

    // If merging should take place
    if(argc == 7) {
        // Check for correct input
        int cluster_count = atoi(argv[ARGV_COUNT_ID]);
        if(cluster_count < 1) {
            fprintf(stderr, "Number of clusters on output must be at least 1!");
            return EXIT_FAILURE;
        }
        // Get weights from argv
        double weights[FLOW_DIMENSIONS];
        for(int i = 0; i < FLOW_DIMENSIONS; i++) {
            weights[i] = strtod(argv[ARGV_WEIGHTS_ID + i], NULL);
        }

        // Calculate distances between all flows and store them in dist_arr
        FlowDist *dist_arr = get_flow_distances(flow_arr, flow_count, weights);
        // Crete instruction set
        FlowDist *instructions = create_instructions(dist_arr, flow_count, flow_count-cluster_count);
        (void)instructions;

        // Merge cluster
        merge_clusters(cluster_arr, instructions, flow_count, cluster_count);
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
    int len = triangular_number(n-1);
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

void print_flows(Flow *flow_arr, int len) {
    for(int i = 0; i < len; i++) {
        fprintf(stdout, "Flow ID: %d\n", flow_arr[i].id);
        for(int j = 0; j < FLOW_DIMENSIONS; j++) {
            fprintf(stdout, "   val[%d]: %lf\n", i, flow_arr[i].values[j]);
        }
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
        fprintf(stdout, "cluster %d:", i);
        // for each id
        Cluster *cluster = clusters->arr[i];
        for(int j = 0; j < cluster->size; j++) {
            fprintf(stdout, " %d", cluster->id_arr[j]);
        }
        putchar('\n');
    }
}
