#include <stdlib.h>
#include <stdint.h> // uint8_t etc types
#include <stdio.h> 
#include <glib.h> // gnew
#include <endian.h> // bits swap (be64toh etc) functions
#include <assert.h>

/* Masks for getting the information stored in L1 and L2 tables.
 * This is how they are in the Qemu code, which differs from 
 * qcow2 documentation. */
#define L1_OFFSET_MASK 0x00fffffffffffe00ULL
#define L1_REFCOUNT_MASK (1ULL<<63)
#define L2_OFFSET_MASK 0x00fffffffffffe00ULL
#define L2_ISZEROS_MASK (1ULL)
#define L2_COMPRESSED_MASK (1ULL<<62)
#define L2_REFCOUNT_MASK (1ULL<<63)

#define DEFAULT_OUTPUT_NAME "output.bin"

/* This structure is mostly taken from qemu code */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t backing_file_offset;
    uint32_t backing_file_size;
    uint32_t cluster_bits;
    uint64_t size;
    uint32_t crypt_method;
    uint32_t l1_size;
    uint64_t l1_table_offset;
    uint64_t refcount_table_offset;
    uint32_t refcount_table_clusters;
    uint32_t nb_snapshots;
    uint64_t snapshots_offset;

    /* The following fields are only valid for version >= 3 */
    uint64_t incompatible_features;
    uint64_t compatible_features;
    uint64_t autoclear_features;

    uint32_t refcount_order;
    uint32_t header_length;

    /* Additional fields */
    uint8_t compression_type;

    /* header must be a multiple of 8 */
    uint8_t padding[7];
} QCowHeader;

/* Use an extra structure to store only useful or transformed header 
 * information that we will need */
typedef struct {

    uint64_t nb_virt_clusters;
    uint32_t l1_size;
    uint64_t l1_offset;
    uint32_t cluster_size;
    uint32_t nb_l2_entries;
    uint64_t backing_file_offset;
    uint32_t backing_file_size;

} UsefulHeader;

/* This is to collect the information stored in a L1 entry */
typedef struct {    
    uint64_t offset;
    uint8_t refcount;
} L1Entry ;

/* Same for a L2 entry */
typedef struct {
    uint8_t is_zeros;
    uint64_t offset;
    uint8_t is_compressed;
    uint8_t refcount;
} L2Entry ;

/* Get the length of a string */
uint32_t str_len(char *str){

    uint32_t result = 0;
    while(str[result] != '\0')
        ++ result;
    return result;
}

/* Take a full path to a file, split it and collect the path (everything until
 * the last '/') and the actual name (what is after). 
 * For instance, on "/home/username/code.c, this will store "/home/username/" 
 * in *path and "code.c" in *name . */
void split_path(char* full_path, char** path, char** name){
    
    uint32_t last_bs = 0;
    uint32_t length = 0;
    /* Get the position of the last backslash and the total length of full_path */
    while(full_path[length] != '\0'){
        if(full_path[length] == '/')
            last_bs = length;
        ++ length;
    }
    
    *path = (char *) malloc((last_bs + 2) * sizeof(char));
    *name = (char *) malloc((length - last_bs) * sizeof(char));
    
    uint32_t i = 0;
    while(i <= last_bs){
        (*path)[i] = full_path[i];
        ++ i;
    }
    (*path)[i] = '\0';
    while(i < length){
        (*name)[i - last_bs - 1] = full_path[i];
        ++ i;
    }
    (*name)[i - last_bs - 1] = '\0';

    return ;
}

/* Merge two strings. Used to build paths of manipulated files.
 * result should be already malloc'ed */
void merge_strings(char *first, char* second, char *result){
    
    uint32_t first_length = str_len(first);
    uint32_t second_length = str_len(second);

    for(uint32_t i = 0 ; i < first_length ; ++ i)
        result[i] = first[i];
    for(uint32_t i = 0 ; i < second_length ; ++ i)
        result[i + first_length] = second[i];
    result[first_length + second_length] = '\0';

    return ;
}


/* This reads the beginning of the header and collects 
 * all the information we need. There are plenty of verification
 * that could be done here to make sure that the file is not
 * corrupted. 
 * See the qcow2 format specification for details about what composes
 * this header, and what are the information that are in the header but
 * we don't read here.
 * Note that everything in the qcow2 file is in big-endian, thus we use
 * <endian.h> functions to do all the swapping. */
void read_header(FILE *f, UsefulHeader *target_header){
    /* Reset the file stream at the beginning of the file */
    fseek(f, 0, SEEK_SET);

    QCowHeader *header = g_new(QCowHeader, 1);
    /* First, store in header the first 104 bytes of the file. The information
     * is correct for qcow2 version 2 and 3 for bytes 0-71, and only for version 3
     * for bytes 72-103 */
    int result = fread(header, 1, 104, f);
    if(!result){
        printf("Failed to fetch header data. Exiting...\n");
        return ;
    }

    /* Header values are stored in big endian. Change that to what is used
     * by the cpu */
    header->magic = be32toh(header->magic);
    header->version = be32toh(header->version);
    header->backing_file_offset = be64toh(header->backing_file_offset);
    header->backing_file_size = be32toh(header->backing_file_size);
    header->cluster_bits = be32toh(header->cluster_bits);
    header->size = be64toh(header->size);
    header->crypt_method = be32toh(header->crypt_method);
    header->l1_size = be32toh(header->l1_size);
    header->l1_table_offset = be64toh(header->l1_table_offset);
    header->refcount_table_offset = be64toh(header->refcount_table_offset);
    header->refcount_table_clusters =
        be32toh(header->refcount_table_clusters);
    header->nb_snapshots = be32toh(header->nb_snapshots);
    header->snapshots_offset = be64toh(header->snapshots_offset);

    /* If we use version 3, swap the remaining bytes as well.
     * Otherwise, fill these fields with default values. */
    if(header->version == 3){
        header->incompatible_features = be64toh(header->incompatible_features);
        header->compatible_features = be64toh(header->compatible_features);
        header->autoclear_features = be64toh(header->autoclear_features);
        header->refcount_order = be32toh(header->refcount_order);
        header->header_length = be32toh(header->header_length);
    }
    else{
        header->incompatible_features = 0;
        header->compatible_features = 0;
        header->autoclear_features = 0;
        header->refcount_order = 4;
        header->header_length = 72;
        
    }

    
    /* Now we fill our UsefulHeader structure with the information from
     * QCowHeader */
    
    target_header->cluster_size = 1 << header->cluster_bits;
    target_header->nb_virt_clusters = 
        header->size / (uint64_t) target_header->cluster_size;
    target_header->l1_size = header->l1_size;
    target_header->l1_offset = header->l1_table_offset;
    target_header->nb_l2_entries = 
        target_header->cluster_size / (uint32_t) (sizeof(uint64_t)); // XXX beware of extended l2 entries, see the documentation to get what I am talking about
    target_header->backing_file_offset = header->backing_file_offset;
    target_header->backing_file_size = header->backing_file_size;


    /* Free the QCowHeader as we don't need it anymore */
    free(header);

    return ;
}

/* Read in the file the name of its backing file, and write it
 * in *name. Space for the name should already be allocated.
 * Returns 1 if the current file actually has a backing file, 0 if not
 * or if an error happened.
 * header is the header of the current file. */
int get_backing_file_name(FILE *f, UsefulHeader *header, char *name){
    
    uint64_t offset = header->backing_file_offset;

    /* If there is no backing file, offset is 0 */
    if(!offset){
        *name = '\0'; 
        return 0 ;
    }

    /* Place the cursor at the offset were we will find the backing file name */
    if(fseek(f, offset, SEEK_SET)){
        printf("Could not set file pointer to backing file name offset.\n");
        return 0 ;
    }

    if(!fread(name, 1, header->backing_file_size, f)){
        printf("Could not read backing file name\n");
        return 0 ;
    }
    else
        name[header->backing_file_size] = '\0';

    return 1 ;
}

/* Load the l1 table in memory at the adress given by target.
 * target should be already malloc'ed to the correct size. */
void load_l1_table(FILE *f, UsefulHeader *header, uint64_t *target){
    
    fseek(f, header->l1_offset, SEEK_SET);
    fread(target, sizeof(uint64_t), header->l1_size, f);
    return ;
}

/* Load the entry of the L1 table stored at position index, get the information
 * thanks to masks defined at beginning of this file, and store the information 
 * in the L1Entry object pointed by l1_entry.
 * XXX Warning: there is an inconsistence between the qcow2 format specification
 * and what the Qemu code actually does. For this reason, while the 
 * documentation says the refcount bit is bit 63, we read bit 0. */
void load_l1_entry(uint64_t *l1_table, uint32_t index, L1Entry *l1_entry){
    
    uint64_t raw_entry = be64toh(l1_table[index]) ;

    l1_entry->offset = raw_entry & L1_OFFSET_MASK ;
    l1_entry->refcount = raw_entry & L1_REFCOUNT_MASK ? 1 : 0 ;

    return ;
}

/* Load a whole L2 table cluster in the allocated space pointed by target. */
void load_l2_table(FILE *f, uint64_t offset, UsefulHeader *header, uint64_t *target){
    
    fseek(f, offset, SEEK_SET);
    fread(target, 1, header->cluster_size, f);

    return ;
}

/* Load the entry of the L2 table stored at position index and store it in 
 * the L2Entry object pointed by l2_entry.
 * XXX Warning: same than the one for L1 entries. */
void load_l2_entry(uint64_t *l2_table, uint32_t index, L2Entry *l2_entry){
    
    uint64_t raw_entry = be64toh(l2_table[index]);
    l2_entry->is_zeros = raw_entry & L2_ISZEROS_MASK ? 1 : 0 ;
    l2_entry->offset = raw_entry & L2_OFFSET_MASK ;
    l2_entry->is_compressed = raw_entry & L2_COMPRESSED_MASK ? 1 : 0 ;
    l2_entry->refcount = raw_entry & L2_REFCOUNT_MASK ? 1 : 0 ;

    printf("%ld\n", l2_entry->offset);
    return ;
}


/* Read the whole L1 table and all the L2 tables to see which clusters are
 * used and note their depth. Loop on the whole chain so we get the depth
 * of all active clusters. */
uint16_t *build_map(char *input_file, uint64_t *nb_virt_clusters, uint16_t *chain_length){
    
    char *path_to_input_file, *input_file_name; 
    split_path(input_file, &path_to_input_file, &input_file_name);
    
    /* We know from qcow2 format that the name won't exceed 1023 bytes */
    char *current_file_name = malloc(sizeof(char)*1024);
    /* Set current_file_name to the input file name */
    for(int i = 0 ; i < 1024 ; ++i){
        current_file_name[i] = input_file_name[i];
        if(current_file_name[i] == '\0')
            break;
    }
    
    char *full_path_to_current_file = malloc((str_len(path_to_input_file) + 1024)*sizeof(char));
    /* Set full_path_to_current_file to the input file path */
    int i = 0;
    while(input_file[i] != '\0'){
        full_path_to_current_file[i] = input_file[i];
        ++ i;
    }
    full_path_to_current_file[i] = '\0';


    /* Open the first datafile of the chain */
    FILE *current_file = fopen(full_path_to_current_file, "r");

    /* Read the header of the input file */
    UsefulHeader *header = g_new(UsefulHeader, 1);
    read_header(current_file, header);

    /* Store the size of the used_clusters table for outside this function */
    *nb_virt_clusters = header->nb_virt_clusters;

    *chain_length = 0;

    /* This is the table that will be filled and returned. */
    uint16_t *clusters_depth = calloc(header->nb_virt_clusters, sizeof(unsigned int));

        
    /* This will be for loading in memory the full tables.
     * current_l1_table should be realloc'ed when loading a new backing file
     * as the size of the disk could change between two snapshots.
     * current_l2_table can be reused without realloc as we assume the cluster
     * size does not change.
     * current_l1_entry and current_l2_entry can be reused.
     */
    uint64_t *current_l1_table = malloc(header->l1_size * sizeof(uint64_t));
    uint64_t *current_l2_table = malloc(header->cluster_size * sizeof(char));

    L1Entry *current_l1_entry = malloc(sizeof(L1Entry));
    L2Entry *current_l2_entry = malloc(sizeof(L2Entry));

    uint64_t guest_cluster_number;

    /* Depth is the position of the datafile in the chain.
     * We consider that the first file has depth 1 */
    unsigned int current_depth = 1;

    /* Iterate on the chain.
     * get_backing_file_name will return 0 when we reach the end of the chain, 
     * so the loop ends when no backing file is found. */
    while(1){      

        *chain_length += 1 ;

        load_l1_table(current_file, header, current_l1_table);

        /* Iterate on the l1 table to reach all the l2 tables */
        for(uint32_t l1_index = 0 ; l1_index < header->l1_size ; l1_index++){
            load_l1_entry(current_l1_table, l1_index, current_l1_entry);
            /* Quickly check that the offset is coherent, i.e. it is indeed
             * the beginning of a cluster */
            assert(!(current_l1_entry->offset % header->cluster_size));

            /* If current_l1_entry refcount is 0, the corresponding
             * L2 table is either unused or requires COW, i.e. not used anymore. 
             * It can then be ignored. */
            if(!current_l1_entry->refcount && !current_l1_entry->offset)
                continue ;
            
            load_l2_table(current_file, current_l1_entry->offset, header, current_l2_table);
            
            /* Iterate on the L2 table to reach all guest clusters */
            for(uint32_t l2_index = 0 ; l2_index < header->nb_l2_entries ; l2_index ++){
                load_l2_entry(current_l2_table, l2_index, current_l2_entry);
                
                /* Quickly check that the offset is coherent, i.e. it is indeed
                * the beginning of a cluster */
                assert(!(current_l2_entry->offset % header->cluster_size));
                
                if(!current_l2_entry->refcount && !current_l2_entry->offset)
                    continue;
                /* Get the position in the virtual disk of the cluster 
                 * referenced by this L2 entry */
                guest_cluster_number = l1_index*header->nb_l2_entries + l2_index;
                /* When we see a cluster that is actually used, note its depth.
                 * We note the depth only the first time as we are interested
                 * only in active clusters. */
                if(!clusters_depth[guest_cluster_number])
                    clusters_depth[guest_cluster_number] = current_depth;
            }

        }

        /* Try getting the backing file name. If there is one, prepare next
         * iteration, otherwise exit the main while loop. */
        if(!get_backing_file_name(current_file, header, current_file_name)){
            fclose(current_file);
            break;
        }
        fclose(current_file);
        merge_strings(path_to_input_file, current_file_name, full_path_to_current_file);
        current_file = fopen(full_path_to_current_file, "r");
        read_header(current_file, header);
        current_l1_table = realloc(current_l1_table, header->l1_size * sizeof(uint64_t));
        ++current_depth;
    }

    free(header);
    free(current_file_name);
    free(current_l1_table);
    free(current_l2_table);
    free(current_l1_entry);
    free(current_l2_entry);

    return clusters_depth;
}


/* Arguments of the main function:
 *  + The path to the first file of the qcow2 chain. Relative or absolute path
 *    should both be fine.
 *  + The name of the output file. This is optional and defaults to 
 *    "output.bin" (how original, I know).
 *    this output file will be a strict copy of the clusters_depth table. */
int main(int argc, char* argv[]){
        
    if(argc < 2){
        printf(
"Arguments missing. How to use:\n\
    ./clusters_map qcow2_file [output_name]\n\
Exiting...\n");
        return 1;
    }
    
    /* Get number of virtual clusters and chain length while building the map
     * in case we need it */
    uint64_t nb_clusters;
    uint64_t *nb_clusters_p = &nb_clusters;
    uint16_t chain_length;
    uint16_t *chain_length_p = &chain_length;
    uint16_t *clusters_depth = build_map(argv[1], nb_clusters_p, chain_length_p);

    FILE *log_file = fopen("clusters_depth.log", "w");
    for (int i = 0; i < nb_clusters; ++i)
    {
        fprintf(log_file, "%d", clusters_depth[i]);
    }
    fclose(log_file);

    FILE *output_file = fopen(argc >= 3 ? argv[2] : DEFAULT_OUTPUT_NAME, "w");
    fwrite(clusters_depth, sizeof(uint16_t), nb_clusters, output_file);
    fclose(output_file);

    return 0;    
}
