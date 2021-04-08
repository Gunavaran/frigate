# Frigate
Frigate is a tool for counting and querying k-mers. Its emphasis is on small k values, typically k < 20;
### Installation 
Current version of Frigate can run only on Linux operating system. support for other operating systems will incorporated in the future versions. To install Frigate:
1. Download the source files<br/>
`git clone https://github.com/Gunavaran/frigate.git` 

2. To compile<br/>
`cd frigate`<br/>
`cmake .`<br/>
`make`
### Usage
Frigate can perform FOUR operations:
 1. count:      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; counting k-mers
 2. histogram:  &nbsp;&nbsp; generating histogram for k-mer counting results
 3. dump:       &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; output k-mer counting results in readable format
 4. query:     &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  query k-mers

##### 1. K-mer counting
`./frigate count [options] <input_file> <output_file>`
|Parameter|Description|Default|
|--|---|---|
|<input_file>| single FASTQ file|
| <output_file>|preferred name for the output file(s)|
|-k <size>|k-mer length|10|
|-t <value>|number of threads|# CPU cores|
|-m <size>|max amount of RAM in GB|size of RAM|
|-l <value>|exclude k-mers with count less than <value> |2|
|-d|disable canonical counting||
|-v|verbose mode; displays the parameter settings|false|

##### 2. Histogram
`./frigate histogram <output_file> <histo_file>`
|||
|--|---|
|<output_file>| same file name used when counting|
|<histo_file>|preferred file name to generate the histogram|

##### 3. K-mer counting results in ASCII format
`./frigate dump <output_file> <dump_file>`
|||
|--|---|
|<output_file>| same file name used when counting|
|<dump_file> |preferred file name to generate the readable outputs|

##### 4. Querying k-mers
`./frigate query <output_file> <query_file>`
|||
|--|---|
|<output_file>| same file name used when counting|
|<query_file>|file containing the k-mers to be queried (k-mers should be separated using \\n)|



## License

MIT
