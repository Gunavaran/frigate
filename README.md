# frigate
A fast k-mer counter for small k values, typically k < 20.

Frigate can perform FOUR operations: 
    1. count:       counting k-mers 
    2. histogram:   generating histogram for k-mer counting results 
    3. dump:        output k-mer counting results in readable format 
    4. query:       query k-mers 
-----------------------------------------------------------------------------------------

1. COUNTING K-MERS ======================================================================
    Usage: ./frigate count [options] <input_file> <output_file> 
    <input_file>        single FASTQ file
    <output_file>       preferred name for the output file(s)
    [options] : 
        -k <size>           size of k-mers (default: 10) 
        -t <value>          number of threads (default: no. of CPU cores) 
        -m <size>           max amount of RAM in GB (default: size of RAM) 
        -l <value>          exclude k-mers with count less than <value> (default: 2)
        -d                  disable canonical counting 
        -v                  verbose mode; displays the parameter settings (default: 0) 

2. HISTOGRAM =============================================================================
    Usage: ./frigate histogram <output_file> <histo_file> 
    <output_file>           same file name used when counting
    <histo_file>            preferred file name to generate the histogram

3. READABLE K-MER COUNTING OUTPUT ========================================================
    Usage: ./frigate dump <output_file> <dump_file>
    <output_file>           same file name used when counting
    <dump_file>             preferred file name to generate the readable outputs

4. QUERYING K-MERS =======================================================================
    Usage: ./frigate query <output_file> <query_file>
    <output_file>           same file name used when counting
    <query_file>            file containing the k-mers to be queried (k-mers should be separated using \n) 

