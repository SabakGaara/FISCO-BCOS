/*
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 */
/**
 * @brief : Generate transaction DAG for parallel execution
 * @author: jimmyshi
 * @date: 2019-1-8
 */

#include "TxDAG_V.h"
#include <tbb/parallel_for.h>
#include <map>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::blockverifier;
using namespace dev::executive;

#define DAG_LOG(LEVEL) LOG(LEVEL) << LOG_BADGE("DAG")

// Generate DAG according with given transactions
void TxDAG_V::init(
        ExecutiveContext::Ptr _ctx, std::shared_ptr<dev::eth::Transactions> _txs, int64_t _blockHeight)
{
    DAG_LOG(TRACE) << LOG_DESC("Begin init transaction DAG") << LOG_KV("blockHeight", _blockHeight)
                   << LOG_KV("transactionNum", _txs->size());

    m_txs = _txs;
    m_dag.init(_txs->size());

    // get criticals
    std::vector<std::shared_ptr<std::vector<std::string>>> txsCriticals;
    auto txsSize = _txs->size();
    txsCriticals.resize(txsSize);
    tbb::parallel_for(
        tbb::blocked_range<uint64_t>(0, txsSize), [&](const tbb::blocked_range<uint64_t>& range) {
            for (uint64_t i = range.begin(); i < range.end(); i++)
            {
                auto& tx = (*_txs)[i];
                txsCriticals[i] = _ctx->getTxCriticals(*tx);
            }
        });

    CriticalField_V<string> latestCriticals;

    for (ID id = 0; id < txsSize; ++id)
    {
#if 0
        auto& tx = _txs[id];
        // Is para transaction?
        auto criticals = _ctx->getTxCriticals(tx);
#endif
        auto criticals = txsCriticals[id];
        if (criticals)
        {
            // DAG transaction: Conflict with certain critical fields
            // Get critical field

            // Add edge between critical transaction
            for (string const& c : *criticals)
            {
                ID pId = latestCriticals.get(c);
                if (pId != INVALID_ID_2)
                {
                    m_dag.addEdge(pId, id);  // add DAG edge
                }
            }

            for (string const& c : *criticals)
            {
                latestCriticals.update(c, id);
            }
        }
    }

    // Generate DAG
    m_dag.generate();

    m_totalParaTxs = _txs->size();

    DAG_LOG(TRACE) << LOG_DESC("End init transaction DAG") << LOG_KV("blockHeight", _blockHeight);
}

bool TxDAG_V::isReady(ID _id)
{
    while(1)
    {
        bool id =  m_dag.waitPop(true,_id);
        if (id)
        {
            return true;
        }
    }

}

void TxDAG_V::consume(ID _id)
{
    m_dag.consume(_id);
}


void TxDAG_V::clear()
{
    m_dag.clear();
}



