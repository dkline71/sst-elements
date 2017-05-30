// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   cacheArray.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef CACHEARRAY_H
#define CACHEARRAY_H

#include <vector>
#include <cstdlib>
#include <boost/dynamic_bitset.hpp>
#include "hash.h"
#include "memEvent.h"
#include "sst/core/output.h"
#include "util.h"
#include "replacementManager.h"

using namespace std;

namespace SST { namespace MemHierarchy {

class ReplacementMgr;

class CacheArray {
public:

    class CacheLine; // Forward declaration so DataLine/CacheLine can point to each other

    /* Data line used for split coherence/data (i.e., directories ) */
    class DataLine {
    private:
        const uint32_t size_;
        const int index_;
        Output * dbg_;

        vector<uint8_t> data_;
        CacheArray::CacheLine * dirLine_;
        
    public:
        DataLine(unsigned int size, int index, Output * dbg) : size_(size), index_(index), dbg_(dbg), dirLine_(nullptr) {
            data_.resize(size_/sizeof(uint8_t));
        }


        // Data getter/setter
        vector<uint8_t>* getData() { return &data_; }

        void setData(vector<uint8_t> data, MemEvent* ev) {
            if (ev->getPayloadSize() == size_) data_ = data;
            else {
                Addr offset = ev->getAddr() - ev->getBaseAddr();
                for(uint32_t i = 0; i < ev->getPayloadSize() ; i++ ) {
                    data_[offset + i] = ev->getPayload()[i];
                }
            }
        }

        // dirIndex getter/setter
        void setDirLine(CacheArray::CacheLine * dirLine) { dirLine_ = dirLine; }
        CacheArray::CacheLine* getDirLine() { return dirLine_; }
        
        // index getter
        int getIndex() { return index_; }
    };


    /* Cache line type - didn't bother splitting into different types (L1/lower-level/dir) because space overhead is small */
    class CacheLine {
    protected:
        const uint32_t          size_;              // Cache line size in Bytes
        const int               index_;
        Output *                dbg_;
        
        Addr                    baseAddr_;
        State                   state_;
        set<std::string>        sharers_;
        std::string             owner_;

        boost::dynamic_bitset<> txWriteBits;
        boost::dynamic_bitset<> txReadBits;
        
        uint64_t                lastSendTimestamp_; // Use to force sequential timing for subsequent accesses to the line

        /* L1 specific */
        unsigned int userLock_;
        bool LLSCAtomic_;
        bool eventsWaitingForLock_;

        /* Dir specific - data stored separately */
        DataLine * dataLine_;

        /* Cache specific */
        vector<uint8_t> data_;

    public:
        CacheLine (unsigned int size, int index, Output * dbg, bool cache) : size_(size), index_(index), dbg_(dbg), baseAddr_(0), state_(I) {
            reset();
            if(cache)
            {
               data_.resize(size_/sizeof(uint8_t));

               txWriteBits.resize(size_/4);
               txReadBits.resize(size_/4);
            }

        }
        
        virtual ~CacheLine() {}

        void reset()
        {
            state_ = I;
            sharers_.clear();
            owner_.clear();
            
            lastSendTimestamp_      = 0;

            txWriteBits.reset();
            txReadBits.reset();

            /* Dir specific */
            dataLine_ = NULL;

            /* L1 specific */
            userLock_               = 0;
            eventsWaitingForLock_   = false;
            LLSCAtomic_             = false;
        }


        /** Getter for size. Constant field - no setter */
        unsigned int getSize() { return size_; }

        /** Getter for index. Constant field - no setter */
        int getIndex() { return index_; }

        /** Setter for line address */
        void setBaseAddr(Addr addr) { baseAddr_ = addr; }

        /** Getter for line address */
        Addr getBaseAddr() { return baseAddr_; }

        /** Setter for line state */
        virtual void setState(State state)
        {
            state_ = state; 
            if(state == I)
            {
                clearAtomics();
                sharers_.clear();
                owner_.clear();

                clearAll_TxWriteBits();
                clearAll_TxReadBits();
            }
        }

        /** Getter for line state */
        State getState() { return state_; }

        /** Getter for line state - return whether state is stable (true) or not (false) */
        bool inTransition() { return (state_ != I) && (state_ != S) && (state_ != E) && (state_ != M) && (state_ != NP); }

        /** Getter for line state - return whether state is valid (!I) */
        bool valid() { return state_ != I; }

        /** Getter for sharer field - return whether sharer field is empty */
        bool isShareless() { return sharers_.empty(); }

        /** Getter for sharer field */
        set<std::string>* getSharers() { return &sharers_; }

        /** Getter for sharer field - return number of sharers in set*/
        unsigned int numSharers() { return sharers_.size(); }
        
        /** Getter for sharer field - return whether a particular sharer exists in the set*/
        bool isSharer(std::string name) { 
            if (name.empty()) return false; 
            return sharers_.find(name) != sharers_.end(); 
        }
        
        /** Setter for sharer field - remove a specific sharer */
        void removeSharer(std::string name) {
            if(name.empty()) return;
            if (sharers_.find(name) == sharers_.end()) 
                dbg_->fatal(CALL_INFO, -1, "Error: cannot remove sharer '%s', not a current sharer. Addr = 0x%" PRIx64 "\n", name.c_str(), baseAddr_);
            sharers_.erase(name);        
        }
    
        /** Setter for sharer field - add a specific sharer */
        void addSharer(std::string name) {
            if (name.empty()) return;
            sharers_.insert(name);
        }

        /** Setter for owner field */
        void setOwner(std::string owner) { owner_ = owner; }

        /** Getter for owner field */
        std::string getOwner() { return owner_; }

        /** Setter for owner field - clear field */
        void clearOwner() { owner_.clear(); }

        /** Getter for owner field - return whether field is set */
        bool ownerExists() { return !owner_.empty(); }

        /** Setter for timestamp field */
        void setTimestamp(uint64_t timestamp) { lastSendTimestamp_ = timestamp; }

        /** Getter for timestamp field */
        uint64_t getTimestamp() { return lastSendTimestamp_; }

        /** HTM Fields **/
        void set_TxWriteBits(Addr addressIn) {
           uint32_t offset = (addressIn - baseAddr_) >> 2;
           txWriteBits.set(offset,1);
        }

        void set_TxReadBits(Addr addressIn) {
           uint32_t offset = (addressIn - baseAddr_) >> 2;
           txReadBits.set(offset,1);
        }

        boost::dynamic_bitset<>  get_TxWritebits(void) { return txWriteBits; };
        boost::dynamic_bitset<>  get_TxReadbits(void) { return txReadBits; };

        bool test_TxWriteBits(void) { return txWriteBits.any(); };
        bool test_TxReadBits(void) { return txReadBits.any(); };

        void clearAll_TxWriteBits(void) { txWriteBits.reset(); }
        void clearAll_TxReadBits(void) { txReadBits.reset(); }


        /****** L1 specific fields ******/
        
        /** Bulk setter for atomic fields - clear fields
         *  Fields: userLock_, eventsWaitingForLock_, LLSCAtomic_
         */
        void clearAtomics() {
            if (userLock_ != 0) dbg_->fatal(CALL_INFO, -1, "Error: clearing cacheline but userLock is not 0. Address = 0x%" PRIx64 "\n", baseAddr_);
            if (eventsWaitingForLock_) dbg_->fatal(CALL_INFO, -1, "Error: clearing cacheline but eventsWaitingForLock is true. Address = 0x%" PRIx64 "\n", baseAddr_);
            atomicEnd();
        }
        
        /** Setter for LLSCAtomic - set true */
        void atomicStart() { LLSCAtomic_ = true; }

        /** Setter for LLSCAtomic - set false */
        void atomicEnd() { LLSCAtomic_ = false; }

        /** Getter for LLSCAtomic */
        bool isAtomic() { return LLSCAtomic_; }

        /** Getter for userLock - return whether userLock > 0 */
        bool isLocked() { return (userLock_ > 0) ? true : false; }

        /** Setter for userLock - increment */
        void incLock() { userLock_++; }

        /** Setter for userLock - decrement */
        void decLock() { userLock_--; }

        /** Getter for eventsWaitingForLock */
        bool getEventsWaitingForLock() { return eventsWaitingForLock_; }

        /** Setter for eventsWaitingForLock */
        void setEventsWaitingForLock(bool eventsWaiting) { eventsWaitingForLock_ = eventsWaiting; }
        
        /***** Cache specific fields *****/
        /** Getter for cache line data */
        vector<uint8_t>* getData() { return &data_; }

        /** Setter for cache line data - write only specified bits*/
        void setData(vector<uint8_t> data, MemEvent* memEvent) {
            if (memEvent->getPayloadSize() == size_) data_ = data;
            else {
                Addr offset = memEvent->getAddr() - baseAddr_;
                for(uint32_t i = 0; i < memEvent->getPayloadSize() ; i++ ) {
                    data_[offset + i] = memEvent->getPayload()[i];
                }
            }
        }

        /***** Dir specific fields *****/
        /** Setter for directory dataline */
        void setDataLine(DataLine * line) { dataLine_ = line; }

        /** Getter for directory dataline */
        DataLine * getDataLine() { return dataLine_; }

    };

    typedef CacheArray::CacheLine CacheLine;
    typedef CacheArray::DataLine DataLine;

    /** Function returns the cacheline tag's ID if its valid (-1 if unvalid).
        If updateReplacement is set, the replacement stats are updated */
    virtual CacheLine * lookup(Addr baseAddr, bool updateReplacement) = 0;

    /** Determine a replacement candidate using the replacement manager */
    virtual CacheLine * findReplacementCandidate(Addr baseAddr, bool cache) = 0;
    
    /** Replace cache line */
    virtual void replace(Addr baseAddr, CacheArray::CacheLine * candidate, CacheArray::DataLine * dataCandidate=nullptr) = 0;

    /** Get line size.  Should not change at runtime */
    Addr getLineSize() { return lineSize_; }
    
    /** Drop block offset bits (ie. log2(lineSize) */
    Addr toLineAddr(Addr addr) { return (Addr) ((addr >> lineOffset_) / slices_); }
    
    /** Destructor - Delete all cache line objects */
    virtual ~CacheArray()
    {
        for (unsigned int i = 0; i < lines_.size(); i++)
            delete lines_[i];
        delete replacementMgr_;
        delete hash_;
    }

    vector<CacheLine *> lines_;
    void setSliceAware(unsigned int numSlices)
    {
        slices_ = numSlices;
    }

private:
    void printConfiguration();
    void errorChecking();


protected:
    Output*         dbg_;
    unsigned int    numSets_;
    unsigned int    numLines_;
    unsigned int    associativity_;
    unsigned int    lineSize_;
    unsigned int    setMask_;
    unsigned int    lineOffset_;
    ReplacementMgr* replacementMgr_;
    HashFunction*   hash_;
    bool            sharersAware_;
    unsigned int    slices_;

    CacheArray(Output* dbg, unsigned int numLines, unsigned int associativity, unsigned int lineSize,
               ReplacementMgr* replacementMgr, HashFunction* hash, bool sharersAware, bool cache) : dbg_(dbg), 
               numLines_(numLines), associativity_(associativity), lineSize_(lineSize),
               replacementMgr_(replacementMgr), hash_(hash) {
        dbg_->debug(_INFO_,"--------------------------- Initializing [Set Associative Cache Array]... \n");
        numSets_    = numLines_ / associativity_;
        setMask_    = numSets_ - 1;
        lineOffset_ = log2Of(lineSize_);
        lines_.resize(numLines_);
        slices_ = 1;

        for (unsigned int i = 0; i < numLines_; i++) {
            lines_[i] = new CacheLine(lineSize_, i, dbg_, cache);
        }

        printConfiguration();
        errorChecking();
        sharersAware_ = sharersAware;
    }
};

/* 
 * Set-associative cache array 
 * Used for arrays where data/coherence state are always held together (e.g., inclusive caches)
 */
class SetAssociativeArray : public CacheArray {
public:

    SetAssociativeArray(Output* dbg, unsigned int numLines, unsigned int lineSize, unsigned int associativity,
                        ReplacementMgr* rp, HashFunction* hf, bool sharersAware);
    

    ~SetAssociativeArray();

    CacheLine * lookup(Addr baseAddr, bool updateReplacement);
    CacheLine * findReplacementCandidate(Addr baseAddr, bool cache);
    void replace(Addr baseAddr, CacheLine * candidate_id, DataLine * dataCandidate);
    unsigned int preReplace(Addr baseAddr);
    void deallocate(unsigned int index);
    State * setStates;
    unsigned int * setSharers;
    bool * setOwned;
};

/*
 *  Dual set-associative cache array
 *  Implements an array for coherence state and an array for data
 *  Data array is a strict subset of the coherence array
 *  Used for arrays where data/coherence state are not neccessarily both cached at the same time (e.g., exclusive caches)
 *
 */
class DualSetAssociativeArray : public CacheArray {
public:
    DualSetAssociativeArray(Output * dbg, unsigned int lineSize, HashFunction * hf, bool sharersAware, unsigned int dirNumLines, unsigned int dirAssociativity, ReplacementMgr* dirRp, 
            unsigned int cacheNumLines, unsigned int cacheAssociativity, ReplacementMgr * cacheRp);

    CacheLine * lookup(Addr baseAddr, bool updateReplacement);
    CacheLine * findReplacementCandidate(Addr baseAddr, bool cache);
    void replace(Addr baseAddr, CacheLine * candidate, DataLine * dataCandidate);
    unsigned int preReplaceDir(Addr baseAddr);
    unsigned int preReplaceCache(Addr baseAddr);
    void deallocateCache(unsigned int index);
    
    vector<DataLine*> dataLines_;
    State * dirSetStates;
    unsigned int * dirSetSharers;
    bool * dirSetOwned;
    State * cacheSetStates;
    unsigned int * cacheSetSharers;
    bool * cacheSetOwned;

private:
    /* For our separate data cache */
    ReplacementMgr* cacheReplacementMgr_;
    unsigned int    cacheNumSets_;
    unsigned int    cacheNumLines_;
    unsigned int    cacheAssociativity_;
    unsigned int    cacheSetMask_;

};

}}
#endif	/* CACHEARRAY_H */
