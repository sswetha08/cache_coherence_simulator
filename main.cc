/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

const char* Protocol[] =
{
    "MSI",
    "MSI BusUpgr",
    "MESI",
    "MESI Filter BusNOP"
};

int main(int argc, char *argv[])
{
    
    ifstream fin;
    FILE * pFile;

    if(argv[1] == NULL){
         printf("input format: ");
         printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
         exit(0);
        }

    ulong cache_size     = atoi(argv[1]);
    ulong cache_assoc    = atoi(argv[2]);
    ulong blk_size       = atoi(argv[3]);
    ulong num_processors = atoi(argv[4]);
    ulong protocol       = atoi(argv[5]); /* 0:MSI 1:MSI BusUpgr 2:MESI 3:MESI Snoop FIlter */
    char *fname        = (char *) malloc(20);
    fname              = argv[6];

    printf("===== 506 Coherence Simulator configuration =====");
    printf("\nL1_SIZE: %lu", cache_size);
    printf("\nL1_ASSOC: %lu", cache_assoc);
    printf("\nL1_BLOCKSIZE: %lu", blk_size);
    printf("\nNUMBER OF PROCESSORS: %lu", num_processors);
    printf("\nCOHERENCE PROTOCOL: %s", Protocol[protocol]);
    printf("\nTRACE FILE: %s", fname);
    
    // Using pointers so that we can use inheritance */
    Cache** cacheArray = (Cache **) malloc(num_processors * sizeof(Cache));
    Cache* historyFilter = (Cache *) malloc(sizeof(Cache));
    for(ulong i = 0; i < num_processors; i++) {
        if(protocol == 0) {
            cacheArray[i] = new MSICache(cacheArray, num_processors, cache_size, cache_assoc, blk_size);
        }
        else if (protocol == 1){
           cacheArray[i] = new MSIXCache(cacheArray, num_processors, cache_size, cache_assoc, blk_size); 
        }
        else if (protocol == 2 || protocol == 3){
           historyFilter = new Cache(cacheArray, num_processors, 16*blk_size, 1, blk_size);
           cacheArray[i] = new MESICache(cacheArray, num_processors, cache_size, cache_assoc, blk_size, historyFilter); 
        }
    }

    pFile = fopen (fname,"r");
    if(pFile == 0)
    {   
        printf("Trace file problem\n");
        exit(0);
    }
    
    ulong proc;
    char op;
    ulong addr;
    Signal sig;

    int line = 1;
    while(fscanf(pFile, "%lu %c %lx", &proc, &op, &addr) != EOF)
    {
        sig = cacheArray[proc]->Access(addr,op);
        for(ulong i=0;i<num_processors;i++)
        {
            if(i==proc)
                continue;
            else
                cacheArray[i]->Snoop(sig, addr);
        }

        line++;
    }

    fclose(pFile);
    
    for(ulong i=0;i<num_processors;i++)
    {
        printf("\n============ Simulation results (Cache %lu) ============", i);
        printf("\n01. number of reads: %lu", cacheArray[i]->reads);
        printf("\n02. number of read misses: %lu", cacheArray[i]->readMisses);
        printf("\n03. number of writes %lu", cacheArray[i]->writes);
        printf("\n04. number of write misses: %lu", cacheArray[i]->writeMisses);
        float missR = (float) (cacheArray[i]->writeMisses + cacheArray[i]->readMisses)/(cacheArray[i]->reads + cacheArray[i]->writes) * 100;
        printf("\n05. total miss rate: %.2f%%", missR);
        printf("\n06. number of writebacks: %lu", cacheArray[i]->writebacks);
        printf("\n07. number of cache-to-cache transfers: %lu", cacheArray[i]->cache_trans);
        printf("\n08. number of memory transactions: %lu", cacheArray[i]->mem_trans);
        printf("\n09. number of interventions: %lu", cacheArray[i]->interventions);
        printf("\n10. number of invalidations: %lu", cacheArray[i]->invalidations);
        printf("\n11. number of flushes: %lu", cacheArray[i]->flushes);
        printf("\n12. number of BusRdX: %lu", cacheArray[i]->busRdxs);
        printf("\n13. number of BusUpgr: %lu", cacheArray[i]->busUpgrs);
        if(protocol == 3)
        {
        printf("\n14. number of useful snoops: %lu", static_cast<MESICache*>(cacheArray[i])->usefulSnoops);
        printf("\n15. number of wasted snoops: %lu", static_cast<MESICache*>(cacheArray[i])->wastedSnoops);
        printf("\n16. number of filtered snoops: %lu", static_cast<MESICache*>(cacheArray[i])->filteredSnoops);
        }
    }
    
}
