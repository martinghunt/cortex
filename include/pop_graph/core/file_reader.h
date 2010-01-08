#ifndef FILE_READER_H_
#define FILE_READER_H_

#include <seq.h>
#include <dB_graph.h>
#include <pop_globals.h>



extern int MAX_FILENAME_LENGTH;
extern int MAX_READ_LENGTH;

typedef enum
 {
    full_flank_format   = 0,
    glf  = 1,
 } Variant_File_Format ;

typedef struct{
  dBNode** flank5p;
  Orientation* flank5p_or;
  int len_flank5p;
  dBNode** ref_allele; 
  Orientation* ref_allele_or; 
  int len_ref_allele;
  dBNode** alt_allele; 
  Orientation* alt_allele_or;
  int len_alt_allele;
  dBNode** flank3p;    
  Orientation* flank3p_or;
  int len_flank3p;
} VariantBranchesAndFlanks;






long long load_fasta_data_from_filename_into_graph_of_specific_person_or_pop(char* filename, long long * bad_reads, long long* dup_reads, int max_chunk_length,
                                                                             boolean remove_duplicates_single_endedly, boolean break_homopolymers, int homopolymer_cutoff,
                                                                             dBGraph* db_graph, EdgeArrayType type, int index);
long long load_fastq_data_from_filename_into_graph_of_specific_person_or_pop(char* filename, long long * bad_reads,  char quality_cut_off, long long* dup_reads, int max_read_length,
                                                                             boolean remove_duplicates_single_endedly, boolean break_homopolymers, int homopolymer_cutoff,
                                                                             dBGraph* db_graph, EdgeArrayType type, int index);



long long load_paired_fastq_from_filenames_into_graph_of_specific_person_or_pop(char* filename1, char* filename2, 
										long long * bad_reads,  char quality_cut_off, int max_read_length, 
										long long* dup_reads, boolean remove_duplicates, boolean break_homopolymers, int homopolymer_cutoff, 
										dBGraph* db_graph, EdgeArrayType type, int index );

long long load_list_of_paired_end_fastq_into_graph_of_specific_person_or_pop(char* list_of_left_mates, char* list_of_right_mates, char quality_cut_off, int max_read_length,
                                                                             long long* bad_reads, long long* num_dups, int* count_file_pairs, boolean remove_dups,
                                                                             boolean break_homopolymers, int homopolymer_cutoff,dBGraph* db_graph, EdgeArrayType type, int index);

long long load_population_as_fasta(char* filename, long long* bad_reads, dBGraph* db_graph);
int load_all_fasta_for_given_person_given_filename_of_file_listing_their_fasta_files(char* f_name, long long* bad_reads, dBGraph* db_graph, int index);

long long load_population_as_fastq(char* filename, long long* bad_reads, char quality_cutoff, dBGraph* db_graph);
int load_all_fastq_for_given_person_given_filename_of_file_listing_their_fastq_files(char* f_name, long long* bad_reads, char quality_cutoff, dBGraph* db_graph, int index);

int load_chromosome_overlap_data(char* f_name,  dBGraph* db_graph, int which_chromosome);

//functions for loading graphs from sv_trio
int load_multicolour_binary_data_from_filename_into_graph(char* filename,  dBGraph* db_graph);

//functions for loading binaries for graph/ target
int load_single_colour_binary_data_from_filename_into_graph(char* filename,  dBGraph* db_graph, EdgeArrayType type, int index);
long long load_all_binaries_for_given_person_given_filename_of_file_listing_their_binaries(char* filename,  dBGraph* db_graph, EdgeArrayType type, int index);
long long load_population_as_binaries_from_graph(char* filename, dBGraph* db_graph);


//functions for comparing graph with reference, or comparing reads with the graph
void read_ref_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference(FILE* fp, int (* file_reader)(FILE * fp, Sequence * seq, int max_read_length, boolean new_entry, boolean * full_entry),
                                                                            int max_read_length, dBGraph * db_graph);
void read_fastq_and_print_reads_that_lie_in_graph(FILE* fp, FILE* fout, int (* file_reader)(FILE * fp, Sequence * seq, int max_read_length, boolean new_entry, boolean * full_entry),
                                                  long long * bad_reads, int max_read_length, dBGraph * db_graph,
                                                  boolean is_for_testing, char** for_test_array_of_clean_reads, int* for_test_index);
void read_fastq_and_print_subreads_that_lie_in_graph_breaking_at_edges_or_kmers_not_in_graph(FILE* fp, FILE* fout,
											     int (* file_reader)(FILE * fp, Sequence * seq, int max_read_length, boolean new_entry, 
														 boolean * full_entry), 
											     long long * bad_reads, int max_read_length, dBGraph * db_graph, 
											     EdgeArrayType type, int index,
											     boolean is_for_testing, char** for_test_array_of_clean_reads, int* for_test_index);

void read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference(char* f_name, dBGraph* db_graph);
void read_all_ref_chromosomes_and_mark_graph(dBGraph* db_graph);

int get_sliding_windows_from_sequence_breaking_windows_when_sequence_not_in_graph(char * seq,  char * qualities, int length, char quality_cut_off, 
										  KmerSlidingWindowSet * windows, int max_windows, int max_kmers, dBGraph* db_graph);

int get_sliding_windows_from_sequence_requiring_entire_seq_and_edges_to_lie_in_graph(char * seq,  char * qualities, int length, char quality_cut_off, 
										     KmerSlidingWindowSet * windows, int max_windows, int max_kmers, dBGraph* db_graph,
										     EdgeArrayType type, int index); 

void read_fastq_and_print_reads_that_lie_in_graph(FILE* fp, FILE* fout, int (* file_reader)(FILE * fp, Sequence * seq, int max_read_length, boolean new_entry, boolean * full_entry), 
						  long long * bad_reads, int max_read_length, dBGraph * db_graph,
						  boolean is_for_testing, char** for_test_array_of_clean_reads, int* for_test_index);


void read_fastq_and_print_subreads_that_lie_in_graph_breaking_at_edges_or_kmers_not_in_graph(FILE* fp, FILE* fout,
                                                                                             int (* file_reader)(FILE * fp, Sequence * seq, int max_read_length, boolean new_entry,
                                                                                                                 boolean * full_entry),
                                                                                             long long * bad_reads, int max_read_length, dBGraph * db_graph,
                                                                                             EdgeArrayType type, int index,
                                                                                             boolean is_for_testing, char** for_test_array_of_clean_reads, int* for_test_index);

void read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference(char* f_name, dBGraph* db_graph);
void read_all_ref_chromosomes_and_mark_graph(dBGraph* db_graph);





//gets number_of_bases_to_load's worth of kmers, and returns the corresponding nodes, orientations etc in he array passed in.

int load_seq_into_array(FILE* chrom_fptr, int number_of_bases_to_load, int length_of_arrays,
			    dBNode * * path_nodes, Orientation * path_orientations, Nucleotide * path_labels, char* path_string,
			    Sequence* seq, KmerSlidingWindow* kmer_window, boolean expecting_new_fasta_entry,  dBGraph * db_graph);


int align_next_read_to_graph_and_return_node_array(FILE* fp, int max_read_length, dBNode** array_nodes, Orientation* array_orientations, 
						   boolean require_nodes_to_lie_in_given_colour,
						   int (* file_reader)(FILE * fp, Sequence * seq, int max_read_length,boolean new_entry, boolean * full_entry), 
						   Sequence* seq, KmerSlidingWindow* kmer_window,dBGraph * db_graph, int colour);

int read_next_variant_from_full_flank_file(FILE* fptr, int max_read_length,
                                           dBNode** flank5p,    Orientation* flank5p_or,    int* len_flank5p,
                                           dBNode** ref_allele, Orientation* ref_allele_or, int* len_ref_allele,
                                           dBNode** alt_allele, Orientation* alt_allele_or, int* len_alt_allele,
                                           dBNode** flank3p,    Orientation* flank3p_or,    int* len_flank3p,
                                           dBGraph* db_graph, int colour);


#endif /* FILE_READER_H_ */
