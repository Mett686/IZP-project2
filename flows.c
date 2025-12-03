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

// Contains unique flow id
// and values
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
int triangular_number(int n) {
    return (n*(n + 1))/2;
}

// Allocates array of Flows
Flow *flow_arr_ctor(int length);

// Frees Flow (just free)
void flow_arr_dtor(Flow *flow_arr);

// Colculates weighted euclidean distance between two flows
// needs to have dimensions defined
double calculate_flow_dist(Flow *flow1, Flow *flow2, double *weights);

// Creates array of FlowDist types that stores distances between all Flows in flow_arr
// n is the number of elements in flow_arr
// needs to have dimensions defined
FlowDist *get_flow_distances(Flow *flow_arr, int n, double *weights);

// Reads header from file, returning number of flows in file
int read_header(FILE *file);

// Reads file line and saves its contents into flow
int read_line(FILE *file, Flow *flow);

// Reads data from file into flow_arr
// returns number of lines read
int read_data(FILE *file, int flow_count, Flow *flow_arr);

// Allocates cluster with given capacity
Cluster *cluster_ctor(int capacity);

// Frees cluster
void cluster_dtor(Cluster *cluster);

// Allocates and fills cluster with id from flow
Cluster *flow_to_cluster(Flow *flow);

// Allocates cluster array
// fills it with flows
ClustArr *initialize_clusters(Flow *flow_arr, int flow_count);

// Frees ClustArr
void cluster_array_dtor(ClustArr *cluster_arr);

// Prints all clusters in cluster arr
void print_clusters(ClustArr *clusters);

// Merges two clusters into new cluster while dtoring the originals
Cluster *cluster_merge(Cluster *clust1, Cluster *clust2);

// Return index of cluster containing flow_id 
int find_flow_id(ClustArr *cluster_arr, int flow_id);

// Merges two clusters on given position in array storing it on id1
void cluster_arr_merge(ClustArr *cluster_arr, int id1, int id2);

// Compares flow ids and returns their delta
int cmp_flow_id(const void *flow1, const void *flow2);

// Compares distances of flows
int cmp_flow_dist(const void *dist1, const void *dist2);


// MAIN
//======================================================================================
int main(int argc, char **argv) {
    // Check for correct arguments
    if( (argc != 2 && argc != 7)) {
        fprintf(stderr, "Invalid input!\n");
        return EXIT_FAILURE;
    }

    // Open file and read header
    FILE *file = fopen(argv[ARGV_FILE_ID], "r");
    if(file == NULL) {
        fprintf(stderr, "Invalid file path!\n");
        return EXIT_FAILURE;
    }
    int flow_count = read_header(file);
    
    // Read data from file into flow_arr
    Flow *flow_arr = flow_arr_ctor(flow_count);
    flow_count = read_data(file, flow_count, flow_arr);
    if(flow_count == 0) {
        fprintf(stderr, "Error, read 0 lines from file!\n");
        return EXIT_FAILURE;
    }
    fclose(file);

    // Sort flows by ids and then initialize clusters
    qsort(flow_arr, flow_count, sizeof(Flow), cmp_flow_id);
    ClustArr *cluster_arr = initialize_clusters(flow_arr, flow_count);

    if(argc == 7) {
        // Check for correct cluster count argument
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

        // Calculate distances between all flows
        int dist_count = triangular_number(flow_count - 1);
        FlowDist *dist_arr = get_flow_distances(flow_arr, flow_count, weights);
        // Ordered them based on their value
        qsort(dist_arr, dist_count, sizeof(FlowDist), cmp_flow_dist);

        // Merge clusters untill theres correct ammount
        int dist_idx = 0;
        while(cluster_arr->size > cluster_count) {
            int c_id1 = find_flow_id(cluster_arr, dist_arr[dist_idx].id1);
            int c_id2 = find_flow_id(cluster_arr, dist_arr[dist_idx].id2);
            if(c_id1 != c_id2) {
            cluster_arr_merge(cluster_arr, c_id1, c_id2);
            }
            dist_idx++;
        }

        free(dist_arr);
    }

    // Print result
    print_clusters(cluster_arr);

    // Cleanup
    flow_arr_dtor(flow_arr);
    cluster_array_dtor(cluster_arr);

    return EXIT_SUCCESS;
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

double calculate_flow_dist(Flow *flow1, Flow *flow2, double *weights) {
    double dist = 0;
    for(int i = 0; i < FLOW_DIMENSIONS; i++) {
        double value_dist = flow1->values[i] - flow2->values[i];
        value_dist *= weights[i] * value_dist; // value_dist = weight*(val1 - val2)^2
        dist += value_dist;
    }
    return dist;
}

FlowDist *get_flow_distances(Flow *flow_arr, int flow_count, double *weights) {
    int dist_count = triangular_number(flow_count-1);
    FlowDist *dist_arr = calloc(dist_count, sizeof(FlowDist));
    int idx = 0;
    // For each pair of flows
    for(int i = 0; i < (flow_count - 1); i++) {
        for(int j = (i + 1); j < flow_count; j++) {
            // Calculate and store distance
            dist_arr[idx].id1 = flow_arr[i].id;
            dist_arr[idx].id2 = flow_arr[j].id;
            dist_arr[idx].dist = calculate_flow_dist(&flow_arr[i], &flow_arr[j], weights);
            idx++;
        }
    }
    return dist_arr;
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

int read_line(FILE *file, Flow *flow) {
    char line[LINE_SIZE] = "";
    if(fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "fgets retunred NULL\n");
        return EXIT_FAILURE;
    }
    // We need avg. packet size, need to calculate that
    double packet_count = 0;
    int result = sscanf(line, LINE_FORMAT,
        &flow->id,
        &flow->values[0],
        &flow->values[1],
        &packet_count,
        &flow->values[2]
    );
    if(result != 5) {
        fprintf(stderr, "sscanf read less tha it should!\n");
        return EXIT_FAILURE;
    }
    // 
    if(packet_count == 0) {
        packet_count = 1;
    }
    flow->values[3] = flow->values[0] / packet_count;
    return EXIT_SUCCESS;
}

int read_data(FILE *file, int flow_count, Flow *flow_arr) {
    int succesful_reads = 0;
    for(int i = 0; i < flow_count; i++) {
        if(read_line(file, &flow_arr[i]) == EXIT_FAILURE) {
            fprintf(stderr, "Readline failed on line %d\n", i + 2); // i + 2 because of header
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
    cluster_array->size = 0;
    for(int i = 0; i < flow_count; i++) {
        cluster_array->arr[i] = flow_to_cluster(&flow_arr[i]);
        if(cluster_array->arr[i] == NULL) {
            fprintf(stderr, "Failed to create cluster from flow at line %d\n", i + 2);
            cluster_array_dtor(cluster_array);
            return NULL;
        }
        cluster_array->size++;
    }
    return cluster_array;
}

void cluster_array_dtor(ClustArr *cluster_arr) {
    for(int i = 0; i < cluster_arr->size; i++) {
        cluster_dtor(cluster_arr->arr[i]);
    }
    free(cluster_arr->arr);
    free(cluster_arr);
}

void print_clusters(ClustArr *clusters) {
    fprintf(stdout, "Clusters:\n");
    for(int i = 0; i < clusters->size; i++) {
        fprintf(stdout, "cluster %d:", i);

        Cluster *cluster = clusters->arr[i];
        for(int j = 0; j < cluster->size; j++) {
            fprintf(stdout, " %d", cluster->id_arr[j]);
        }
        putchar('\n');
    }
}

Cluster *cluster_merge(Cluster *clust1, Cluster *clust2) {
    int size = clust1->size + clust2->size;
    Cluster *result = cluster_ctor(size);
    if(result == NULL) {
        fprintf(stderr, "Cluster merge failed, nowhere to merge into\n");
        return NULL;
    }
    result->size = size;
    
    // Read indexes
    int idx1 = 0;
    int idx2 = 0;
    // Merge clusters so that they stay ordered
    while((idx1 + idx2) < size) {
        int id1;
        int id2;
        // Prevent unwanted memmory access
        if(idx1 < clust1->size) {
            id1 = clust1->id_arr[idx1];
        }
        if(idx2 < clust2->size) {
            id2 = clust2->id_arr[idx2];
        }
        // If one array is allready read
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
        // Write lower of the values
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

void cluster_arr_merge(ClustArr *cluster_arr, int id1, int id2) {
    // Make sure cluster_arr stays ordered
    if(id1 > id2) {
        int tmp = id1;
        id1 = id2;
        id2 = tmp;
    }
    cluster_arr->arr[id1] = cluster_merge(cluster_arr->arr[id1], cluster_arr->arr[id2]);
    // move clusters to fill empty space after id2
    for(int i = id2; i < cluster_arr->size - 1; i++) {
        cluster_arr->arr[i] = cluster_arr->arr[i+1];
    }
    cluster_arr->size--;
}

int cmp_flow_id(const void *flow1, const void *flow2) {
    const Flow *flow1_p = (const Flow *)flow1;
    const Flow *flow2_p = (const Flow *)flow2;
    return flow1_p->id - flow2_p->id;
}

int cmp_flow_dist(const void *dist1, const void *dist2) {
    const FlowDist *dist1_p = (const FlowDist *)dist1;
    const FlowDist *dist2_p = (const FlowDist *)dist2;
    if(dist1_p->dist < dist2_p->dist) {
        return -1;
    }
    return 1;
}
