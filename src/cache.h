/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum {
   INVALID = 0,
   VALID,
   DIRTY
};

enum Signal{
   NONE = 0,
   BUSRD,
   BUSRDX,
   BUSUPGR
};

enum State{
   I= 0,
   S,
   M,
   E
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq; 
   State state; // 0:I, 1:S, 2:M, 3:E
 
public:
   cacheLine()                { tag = 0; Flags = 0; }
   ulong getTag()             { return tag; }
   ulong getFlags()           { return Flags;}
   ulong getSeq()             { return seq; }
   void setSeq(ulong Seq)     { seq = Seq;}
   void setFlags(ulong flags) {  Flags = flags;}
   void setTag(ulong a)       { tag = a; }
   void invalidate()          { tag = 0; Flags = INVALID; } //useful function
   bool isValid()             { return ((Flags) != INVALID); }
   State getState()           { return state;}
   void setState(State st)    { state = st;}
};

class Cache
{
protected:
   Cache** cacheArray; // Reference to the Cache System
   ulong num_processors; // Number of Caches in the System

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)   { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag) { return (tag << (log2Blk));}
   
public:

   ulong currentCycle, size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;
   ulong writebacks, mem_trans, interventions, invalidations, flushes, busRdxs, busUpgrs, cache_trans; //cache coherence counters
     
    Cache(Cache**,int,int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   void setLineState(ulong addr, State state); 
   State getLineState(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   bool CopyCheck(ulong addr);
   
   ulong getRM()     {return readMisses;} 
   ulong getWM()     {return writeMisses;} 
   ulong getReads()  {return reads;}       
   ulong getWrites() {return writes;}
   ulong getWB()     {return writebacks;}
   
   void writeBack(ulong) {writebacks++;}
   virtual Signal Access(ulong,uchar);
   virtual void Snoop(Signal sig, ulong addr);
   void printStats();
   void updateLRU(cacheLine *);

};

class MSICache: public Cache
{
public:
   MSICache(Cache** c, int n, int s,int a,int b): Cache(c,n,s,a,b) {}
   ~MSICache() {}

   Signal Access(ulong,uchar);
   void Snoop(Signal sig, ulong addr);
};

class MSIXCache: public Cache
{
public:
   MSIXCache(Cache** c, int n, int s,int a,int b): Cache(c,n,s,a,b) {}
   ~MSIXCache() {}
   Signal Access(ulong,uchar);
   void Snoop(Signal sig, ulong addr);
};

class MESICache: public Cache
{
public:
   Cache* historyFilter;
   ulong filteredSnoops, usefulSnoops, wastedSnoops, totalsnoops;
   MESICache(Cache** c, int n, int s,int a,int b, Cache* hf): Cache(c,n,s,a,b) {
      historyFilter = hf;
      filteredSnoops = usefulSnoops = wastedSnoops = totalsnoops = 0;
   }
   ~MESICache() {}
   Signal Access(ulong,uchar);
   void Snoop(Signal sig, ulong addr);
};

#endif
