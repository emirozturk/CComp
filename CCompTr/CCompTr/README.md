# CComp: A Parallel Compression Algorithm for Compressed Word Search
The goal of CComp is to achieve better compressed search times while achieving the same compression-decompression speed as other parallel compression algorithms. CComp achieves this by splitting both the word dictionaries and the input stream, processing them in parallel.

run script could be used to compile. 

To use compression
```c++
./CComp -c FileName ThreadCount CompressionRate
```

Thread count could be given at any value but at some point, increasing the thread count will decrease performance. 
Rate could be given in range of 0-100. As the range increases, CComp will be slower but compression ratio will increase.

To use decompression
```c++
./CComp -d FileName ThreadCount
```

Compressor creates a folder in the name of text file (foo directory for foo.txt) and puts stream and dict files inside. To decompress, FileName should be given as foo.txt. Decompressor will split input text and obtain "foo" keyword and reads stream and dict files in the directory. 
Thread count could be given at any value

To use compressed search
```c++
./CComp -s FileName ThreadCount word
```

Again, the file name should be given with its extension (like foo.txt). CComp will search the word in stream and returns the count of the given word 
