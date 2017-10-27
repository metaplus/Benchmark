## DiskIO
Assign different buffer size per socket-like file stream and 2 dedicated
thread separately handle its own ssd in a style that reading behaviours are
interleaved by a atomic indexer.
## Result
Fact can be observered that performance reaches plenty good level
after the buffer size per IO thread gains 4MB while too small buffer
entails a approximate reading speed to the 1 single thread 
cirucumstance.  
The underlying explanation should be the cache block in the disk 
whose size affect the optimal buffer size in system memory.
![8W5a6.png](https://s1.ax1x.com/2017/10/13/8W5a6.png "io performance")