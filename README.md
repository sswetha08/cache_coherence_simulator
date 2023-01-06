## Cache Coherence Simulator

### Objective
To study and compare the performance of different kinds of bus-based coherence protocols.

This project simulates cache coherence for 3 different protocols: 
* MSI
* MSI (With BusUpgr signal)
* MESI 

Output of the simulator contains metrics like cache hits, misses, memory transactions,number of interventions, invalidations and snoops which can be used to to compare performance for the sample trace files.

#### History Filter
A history filter has been implemented to reduce the number of wasteful snoops on the receiving core side and improve performance.
It has been implemented by default as a direct mapped tag array with 16 entries and block size the same as in the cache.

### Execution

```./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file>```

