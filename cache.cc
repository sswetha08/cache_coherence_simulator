/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(Cache** cacheA, int num_p, int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writebacks = currentCycle = 0;

   cacheArray = cacheA;
   num_processors = num_p;
   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
      tagMask <<= 1;
      tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
         cache[i][j].invalidate();
      }
   }      
   
}

void Cache::Snoop(Signal sig, ulong addr)
{
   // Implement in specialized caches
}

bool Cache::CopyCheck(ulong addr)
{
    for(ulong i=0;i<num_processors;i++)
    {
        if(cacheArray[i]->findLine(addr))
         return true;
    }
    return false;
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
Signal Cache::Access(ulong addr,uchar op)
{
   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/
         
   if(op == 'w') writes++;
   else          reads++;
   
   cacheLine * line = findLine(addr);
   if(line == NULL)/*miss*/
   {
      if(op == 'w') writeMisses++;
      else readMisses++;

      cacheLine *newline = fillLine(addr);
      if(op == 'w') newline->setFlags(DIRTY);    
      
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w') line->setFlags(DIRTY);
   }
   return NONE;
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
   if(cache[i][j].isValid()) {
      if(cache[i][j].getTag() == tag)
      {
         pos = j; 
         break; 
      }
   }
   if(pos == assoc) {
      return NULL;
   }
   else {
      return &(cache[i][pos]); 
   }
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
   line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) { 
         return &(cache[i][j]); 
      }   
   }

   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].getSeq() <= min) { 
         victim = j; 
         min = cache[i][j].getSeq();}
   } 

   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   
   if(victim->getFlags() == DIRTY) {
      writeBack(addr);
      mem_trans++;
   }
      
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::setLineState(ulong addr, State state)
{ 

   cacheLine *line = findLine(addr);
   //assert(line != 0);
  
   line->setState(state);    
}

State Cache::getLineState(ulong addr)
{ 

   cacheLine *line= findLine(addr);
   if(line == NULL)
      return I;
   return line->getState();    
}

void Cache::printStats()
{ 
   printf("===== Simulation results      =====\n");
   /****print out the rest of statistics here.****/
   /****follow the ouput file format**************/
}

Signal MSICache::Access(ulong addr,uchar op)
{
   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/
         
   if(op == 'w') writes++;
   else          reads++;
   
   cacheLine * line = findLine(addr);

   if(line == NULL)/*miss*/
   {
      cacheLine *newLine = fillLine(addr);
      if(op == 'w') {
         busRdxs++;
         mem_trans++;
         writeMisses++;  
         setLineState(addr, M);
         newLine->setFlags(DIRTY);  
         return BUSRDX; 
      }
      else 
      {
         readMisses++;
         mem_trans++;
         setLineState(addr, S);
         return BUSRD;
      }
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w' && getLineState(addr) == S){
            mem_trans++;
            busRdxs++;
            setLineState(addr, M);
            line->setFlags(DIRTY);
            return BUSRDX;

      }
      return NONE;
   }
}

void MSICache::Snoop(Signal sig, ulong addr)
{
   State state = getLineState(addr);
   cacheLine * line = findLine(addr);

   if(state == S)
   {
      switch(sig)
      {
         case BUSRDX:
            setLineState(addr, I);
            line->invalidate();
            invalidations++;
            break;
         default: break;

      }
   }
   else if (state == M)
   {
      switch(sig)
      {
         case BUSRD:
            setLineState(addr, S);
            line->setFlags(VALID);
            interventions++;
            flushes++;
            writebacks++;
            mem_trans++;
            break;
         case BUSRDX:
            setLineState(addr, I);
            line->invalidate();
            invalidations++;
            writebacks++;
            mem_trans++;
            flushes++;
            break;
         default: break;
      }
   }
      
}

Signal MSIXCache::Access(ulong addr,uchar op)
{
   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/
         
   if(op == 'w') writes++;
   else          reads++;
   
   cacheLine * line = findLine(addr);
   if(line == NULL)/*miss*/
   {
      cacheLine *newLine = fillLine(addr);
      if(op == 'w') {
         busRdxs++;
         mem_trans++;
         writeMisses++;  
         setLineState(addr, M);
         newLine->setFlags(DIRTY);  
         return BUSRDX; 
      }
      else 
      {
         readMisses++;
         mem_trans++;
         setLineState(addr, S);
         return BUSRD;
      }
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w' && getLineState(addr) == S){
      setLineState(addr, M);
      line->setFlags(DIRTY);
      busUpgrs++;
      return BUSUPGR;
      }
      return NONE;
   }
}

void MSIXCache::Snoop(Signal sig, ulong addr)
{
   State state = getLineState(addr);
   cacheLine * line = findLine(addr);
   if(state == S)
   {
      switch(sig)
      {
         case BUSRDX:
            setLineState(addr, I);
            line->invalidate();
            invalidations++;
            break;
         case BUSUPGR:
            setLineState(addr, I);
            line->invalidate();
            invalidations++;
            break;
         default: break;

      }
   }
   else if (state == M)
   {
      switch(sig)
      {
         case BUSRD:
            setLineState(addr, S);
            line->setFlags(VALID);
            interventions++;
            flushes++;
            writebacks++;
            mem_trans++;
            break;
         case BUSRDX:
            setLineState(addr, I);
            line->invalidate();
            invalidations++;
            writebacks++;
            mem_trans++;
            flushes++;
            break;
         default: break;
      }
   }
      
}

Signal MESICache::Access(ulong addr,uchar op)
{
   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/
         
   if(op == 'w') writes++;
   else          reads++;
   
   cacheLine * line = findLine(addr);
   if(line == NULL)/*miss*/
   {
      cacheLine *lhf = historyFilter->findLine(addr);
      if (lhf != NULL)
         lhf->invalidate();   // Remove this entry from history as it will be fetched now

      // Does any cache have a copy?
      int cc = CopyCheck(addr);
      if(cc)
         cache_trans++;
      else
         mem_trans++;

      cacheLine *newLine = fillLine(addr);
      if(op == 'w') {
         busRdxs++;
         writeMisses++;  
         setLineState(addr, M);
         newLine->setFlags(DIRTY);  
         return BUSRDX; 
      }
      else 
      {
         readMisses++;
         if (cc)
            setLineState(addr, S);
         else
            setLineState(addr, E);
         return BUSRD;
      }
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w' ){
      if (getLineState(addr) == S) // No BUSUPGR if E or M
      {
         busUpgrs++;
         setLineState(addr, M);
         line->setFlags(DIRTY);
         return BUSUPGR;
      }
         setLineState(addr, M);
         line->setFlags(DIRTY);
      }
      return NONE;
   }
}

void MESICache::Snoop(Signal sig, ulong addr)
{
   if(sig == NONE)
      return;

   // Check with history filter first
   cacheLine * lhf = historyFilter->findLine(addr);
   if (lhf != NULL)
   {
      filteredSnoops++;
      return;
   }
   
   State state = getLineState(addr);
   cacheLine * line = findLine(addr);
   if (line == NULL)
   {
      wastedSnoops++;
      historyFilter->fillLine(addr);
   }
   else
      usefulSnoops++;

   if(state == S)
   {
      switch(sig)
      {
         case BUSRDX:
            setLineState(addr, I);
            line->invalidate();
            invalidations++;
            historyFilter->fillLine(addr);
            break;
         case BUSUPGR:
            setLineState(addr,I);
            line->invalidate();
            invalidations++;
            historyFilter->fillLine(addr);
            break;
         default: break;

      }
   }
   if(state == E)
   {
      switch(sig)
      {
         case BUSRDX:
            setLineState(addr, I);
            line->invalidate();
            invalidations++;
            historyFilter->fillLine(addr);
            break;
         case BUSRD:
            setLineState(addr, S); 
            interventions++;
            break;
         default: break;

      }
   }
   else if (state == M)
   {
      switch(sig)
      {
         case BUSRD:
            setLineState(addr, S);
            line->setFlags(VALID);
            interventions++;
            flushes++;
            writebacks++;
            mem_trans++;
            break;
         case BUSRDX:
            setLineState(addr, I);
            line->invalidate();
            invalidations++;
            writebacks++;
            mem_trans++;
            flushes++;
            historyFilter->fillLine(addr);
            break;
         default: break;
      }
   }
}
