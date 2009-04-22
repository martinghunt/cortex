/*
  routines to load files into dB Graph
 */


#include <binary_kmer.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <dB_graph.h>
#include <seq.h>
#include <file_reader.h>
#include <element.h>

long long load_seq_data_into_graph(FILE* fp, int (* file_reader)(FILE * fp, Sequence * seq, int max_read_length), long long * bad_reads, char qualiy_cut_off, int max_read_length, dBGraph * db_graph);


long long load_fasta_data_from_filename_into_graph(char* filename, long long * bad_reads, int max_read_length, dBGraph* db_graph)
{
  FILE* fp = fopen(filename, "r");
  if (fp == NULL){
    fprintf(stderr,"cannot open file:%s\n",filename);
    exit(1); //TODO - prefer to print warning and skip file and return an error code?
  }

  long long ret = load_seq_data_into_graph(fp,&read_sequence_from_fasta,bad_reads,0,max_read_length,db_graph);

  fclose(fp);

  return ret;
}

long long load_fastq_data_from_filename_into_graph(char* filename, long long * bad_reads,  char quality_cut_off, int max_read_length, dBGraph* db_graph)
{
  FILE* fp = fopen(filename, "r");
  if (fp == NULL){
    fprintf(stderr,"cannot open file:%s\n",filename);
    exit(1); //TODO - prefer to print warning and skip file and return an error code?
  }

  long long ret =  load_seq_data_into_graph(fp,&read_sequence_from_fastq,bad_reads,quality_cut_off,max_read_length,db_graph);
  fclose(fp);

  return ret;
}




//returns length of sequence loaded
long long load_seq_data_into_graph(FILE* fp, int (* file_reader)(FILE * fp, Sequence * seq, int max_read_length), long long * bad_reads, char quality_cut_off, int max_read_length, dBGraph * db_graph){

  //----------------------------------
  // preallocate the memory used to read the sequences
  //----------------------------------
  Sequence * seq = malloc(sizeof(Sequence));
  if (seq == NULL){
    fputs("Out of memory trying to allocate Sequence\n",stderr);
    exit(1);
  }
  alloc_sequence(seq,max_read_length,LINE_MAX);
  
  
  long long seq_length=0;
  short kmer_size = db_graph->kmer_size;

  //max_read_length/(kmer_size+1) is the worst case for the number of sliding windows, ie a kmer follow by a low-quality/bad base
  int max_windows = max_read_length/(kmer_size+1);
 
  //number of possible kmers in a 'perfect' read
  int max_kmers   = max_read_length-kmer_size+1;

  

  //----------------------------------
  //preallocate the space of memory used to keep the sliding_windows. NB: this space of memory is reused for every call -- with the view 
  //to avoid memory fragmentation
  //NB: this space needs to preallocate memory for orthogonal situations: 
  //    * a good read -> few windows, many kmers per window
  //    * a bad read  -> many windows, few kmers per window    
  //----------------------------------
  KmerSlidingWindowSet * windows = malloc(sizeof(KmerSlidingWindowSet));  
  if (windows == NULL){
    fputs("Out of memory trying to allocate a KmerArraySet",stderr);
    exit(1);
  }  
  //allocate memory for the sliding windows 
  binary_kmer_alloc_kmers_set(windows, max_windows, max_kmers);
  
  int entry_length;

  //hash_table_print_stats(db_graph);
  while ((entry_length = file_reader(fp,seq,max_read_length))){
    
    if (DEBUG){
      printf ("\nsequence %s - kmer size: %i - entry length: %i - max kmers:%i\n",seq->seq,db_graph->kmer_size, entry_length, max_kmers);
    }
    
    int i,j;
    seq_length += (long long) entry_length;
    
    int nkmers = get_sliding_windows_from_sequence(seq->seq,seq->qual,entry_length,quality_cut_off,db_graph->kmer_size,windows,max_windows, max_kmers);

    if (nkmers == 0) {
      (*bad_reads)++;
    }
    else {
      Element * current_node  = NULL;
      Element * previous_node = NULL;
      
      Orientation current_orientation,previous_orientation;
     
      for(i=0;i<windows->nwindows;i++){ //for each window
	KmerSlidingWindow * current_window = &(windows->window[i]);
	
	for(j=0;j<current_window->nkmers;j++){ //for each kmer in window
	  boolean found = false;
	  current_node = hash_table_find_or_insert(element_get_key(current_window->kmer[j],db_graph->kmer_size),&found,db_graph);	  
	  if (current_node == NULL){
	    fputs("file_reader: problem - current kmer not found\n",stderr);
	    exit(1);
	  }
	  
	  element_update_coverage(current_node,1);

	  current_orientation = db_node_get_orientation(current_window->kmer[j],current_node, db_graph->kmer_size);
	  
	  if (DEBUG){
	    char kmer_seq[db_graph->kmer_size];
	    printf("kmer %i:  %s\n",i,binary_kmer_to_seq(current_window->kmer[j],db_graph->kmer_size,kmer_seq));
	  }
	  
	  if (j>0){
	    
	    if (previous_node == NULL){
	      fputs("file_reader: problem - prev kmer not found\n",stderr);
	      exit(1);
	    }
	    else{
	      db_node_add_edge(previous_node,current_node,previous_orientation,current_orientation, db_graph->kmer_size);	  	  
	    }
	  }
	  previous_node = current_node;
	  previous_orientation = current_orientation;
	  
	}
      }
      
    }
  }
  
 
  free_sequence(&seq);
  binary_kmer_free_kmers_set(&windows);
  return seq_length;    
}


//returns number of sequence loaded (ie all the kmers concatenated)
//count_kmers returns the number of new kmers
long long load_binary_data_from_filename_into_graph(char* filename,  dBGraph* db_graph){
  FILE* fp_bin = fopen(filename, "r");
 
  dBNode node_from_file;
  boolean found;
  long long count=0;

  if (fp_bin == NULL){
    printf("cannot open file:%s\n",filename);
    exit(1); //TODO - prefer to print warning and skip file and return an error code?
  }
  
  //Go through all the entries in the binary file
  while (db_node_read_binary(fp_bin,db_graph->kmer_size,&node_from_file)){
    count++;
    
    if (count % 10000000 == 0 ){
     printf("loaded %lld\n",count);
    }
   
    dBNode * current_node  = NULL;
    current_node = hash_table_find_or_insert(element_get_key(element_get_kmer(&node_from_file),db_graph->kmer_size),&found,db_graph);
       
   
    db_node_set_edges(current_node,db_node_get_edges(&node_from_file));
    element_update_coverage(current_node, element_get_coverage(&node_from_file));
  }
 
  fclose(fp_bin);

  return (long long) db_graph->kmer_size * count;

}


void read_ref_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference(FILE* fp, int (* file_reader)(FILE * fp, Sequence * seq, int max_read_length), 
									    long long * bad_reads, char quality_cut_off, int max_read_length, dBGraph * db_graph)
{
    //----------------------------------
  // preallocate the memory used to read the sequences
  //----------------------------------
  Sequence * seq = malloc(sizeof(Sequence));
  if (seq == NULL){
    fputs("Out of memory trying to allocate Sequence\n",stderr);
    exit(1);
  }
  alloc_sequence(seq,max_read_length,LINE_MAX);
  
  
  int seq_length=0;
  short kmer_size = db_graph->kmer_size;

  //max_read_length/(kmer_size+1) is the worst case for the number of sliding windows, ie a kmer follow by a low-quality/bad base
  int max_windows = max_read_length/(kmer_size+1);
 
  //number of possible kmers in a 'perfect' read
  int max_kmers   = max_read_length-kmer_size+1;

  

  //----------------------------------
  //preallocate the space of memory used to keep the sliding_windows. NB: this space of memory is reused for every call -- with the view 
  //to avoid memory fragmentation
  //NB: this space needs to preallocate memory for orthogonal situations: 
  //    * a good read -> few windows, many kmers per window
  //    * a bad read  -> many windows, few kmers per window    
  //----------------------------------
  KmerSlidingWindowSet * windows = malloc(sizeof(KmerSlidingWindowSet));  
  if (windows == NULL){
    fputs("Out of memory trying to allocate a KmerArraySet",stderr);
    exit(1);
  }  
  //allocate memory for the sliding windows 
  binary_kmer_alloc_kmers_set(windows, max_windows, max_kmers);
  
  int entry_length;

  while ((entry_length = file_reader(fp,seq,max_read_length))){

    if (DEBUG){
      printf ("\nsequence %s\n",seq->seq);
    }
    
    int i,j;
    seq_length += entry_length;
    
    int nkmers = get_sliding_windows_from_sequence(seq->seq,seq->qual,entry_length,quality_cut_off,db_graph->kmer_size,windows,max_windows, max_kmers);
    
    if (nkmers == 0) {
      (*bad_reads)++;
    }
    else {
      Element * current_node  = NULL;

      
      Orientation current_orientation;
      
      for(i=0;i<windows->nwindows;i++){ //for each window
	KmerSlidingWindow * current_window = &(windows->window[i]);
	
	for(j=0;j<current_window->nkmers;j++){ //for each kmer in window

	  current_node = hash_table_find(element_get_key(current_window->kmer[j],db_graph->kmer_size),db_graph);	  	 

	  if (current_node){
	    db_node_set_status(current_node, exists_in_reference);
	  }
	  
	}
	
      }
    }
    
  }
  
  free_sequence(&seq);
  binary_kmer_free_kmers_set(&windows);

}

void read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference(char* f_name, dBGraph* db_graph)
{

  int max_read_length = 3000;

  FILE* fptr = fopen(f_name, "r");
  if (fptr == NULL)
    {
      printf("cannot open chromosome fasta file:%s\n",f_name);
      exit(1);
    }

  long long bad_reads;
  read_ref_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference(fptr, &read_sequence_from_fasta, &bad_reads,  0 , max_read_length, db_graph);


}


void read_all_ref_chromosomes_and_mark_graph(dBGraph* db_graph)
{

  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.1.fa.short_reads" , db_graph);
  printf("Loaded chromosome 1\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.2.fa.short_reads" , db_graph);
  printf("Loaded chromosome 2\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.3.fa.short_reads" , db_graph);
  printf("Loaded chromosome 3\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.4.fa.short_reads" , db_graph);
  printf("Loaded chromosome 4\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.5.fa.short_reads" , db_graph);
  printf("Loaded chromosome 5\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.6.fa.short_reads" , db_graph);
  printf("Loaded chromosome 6\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.7.fa.short_reads" , db_graph);
  printf("Loaded chromosome 7\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.8.fa.short_reads" , db_graph);
  printf("Loaded chromosome 8\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.9.fa.short_reads" , db_graph);
  printf("Loaded chromosome 9\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.10.fa.short_reads" , db_graph);
  printf("Loaded chromosome 10\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.11.fa.short_reads" , db_graph);
  printf("Loaded chromosome 11\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.12.fa.short_reads" , db_graph);
  printf("Loaded chromosome 12\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.13.fa.short_reads" , db_graph);
  printf("Loaded chromosome 13\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.14.fa.short_reads" , db_graph);
  printf("Loaded chromosome 14\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.15.fa.short_reads" , db_graph);
  printf("Loaded chromosome 15\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.16.fa.short_reads" , db_graph);
  printf("Loaded chromosome 16\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.17.fa.short_reads" , db_graph);
  printf("Loaded chromosome 17\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.18.fa.short_reads" , db_graph);
  printf("Loaded chromosome 18\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.19.fa.short_reads" , db_graph);
  printf("Loaded chromosome 19\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.20.fa.short_reads" , db_graph);
  printf("Loaded chromosome 20\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.21.fa.short_reads" , db_graph);
  printf("Loaded chromosome 21\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.22.fa.short_reads" , db_graph);
  printf("Loaded chromosome 22\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.23.fa.short_reads" , db_graph);
  printf("Loaded chromosome 23\n");
  read_chromosome_fasta_and_mark_status_of_graph_nodes_as_existing_in_reference("/nfs/1000g-work/G1K/work/zi/projects/marzam/humref/split/clean/Homo_sapiens.NCBI36.52.dna.chromosome.24.fa.short_reads" , db_graph);
  printf("Loaded chromosome 24\n");

  

}

