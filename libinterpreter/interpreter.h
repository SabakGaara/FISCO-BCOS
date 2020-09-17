/* Aleth: Ethereum C++ client, tools and libraries.
 * Copyright 2018 Aleth Autors.
 * Licensed under the GNU General Public License, Version 3. See the LICENSE file.
 */
/** @file interpreter.h
 * @author wheatli
 * @date 2018.8.27
 * @record copy from aleth, this is a default VM
 */

#pragma once

#include <evmc/evmc.h>
#include <evmc/utils.h>

#include <tbb/spin_mutex.h>
#include <tbb/concurrent_vector.h>
#include <libdevcore/Guards.h>
#include <tbb/concurrent_queue.h>
#include <condition_variable>
#include <cstdint>
#include <queue>
#include <thread>
#include <vector>

using my_mutex_t = tbb::speculative_spin_mutex;
//using my_mutex_t = tbb::spin_mutex;  
extern my_mutex_t my_mutex_10;
#if __cplusplus
extern "C" {
#endif

EVMC_EXPORT struct evmc_instance* evmc_create_interpreter() EVMC_NOEXCEPT;

#if __cplusplus
}
#endif



class TBBMUTEX 
{
   public:
        static my_mutex_t my_mutex_2;
        static tbb::concurrent_vector<uint32_t> tempEntries;
//        static std::vector<uint32_t> tempEntries;
	void hello();
};

using ID = uint32_t;
using IDs = std::vector<ID>;
static const ID INVALID_ID_2 = (ID(0) - 1);

struct Vertex_2 {
    std::atomic <ID> inDegree;
    std::vector <ID> outEdge;
};

class DAG_V {
    // Just algorithm, not thread safe
public:
    DAG_V() {};

    ~DAG_V();

    // Init DAG basic memory, should call before other function
    // _maxSize is max ID + 1
    void init(ID _maxSize);

    // Add edge between vertex
    void addEdge(ID _f, ID _t);

    // Generate DAG
    void generate();
    
    bool findID(ID id);

    // Wait until topLevel is not empty, return INVALID_ID if DAG reach the end
    ID waitPop(bool _needWait, ID _t);

    // Consume the top and add new top in top queue (thread safe)
    ID consume(ID _id);

    // Clear all data of this class (thread safe)
    void clear();
    ID m_totalVtxs = 0;

    std::vector <std::shared_ptr<Vertex_2>> m_vtxs;
    tbb::concurrent_queue <ID> m_topLevel;
    tbb::concurrent_vector <ID> m_list;
    std::atomic <ID> m_totalConsume;
    void printVtx(ID _id);

    mutable std::mutex x_topLevel;
    std::condition_variable cv_topLevel;
};




class forDAG
{
public:
    static DAG_V lockDAG;
    void hello();
};
