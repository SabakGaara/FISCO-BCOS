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

#pragma once
#include <libinterpreter/interpreter.h>
#include "ExecutiveContext.h"
#include <libconfig/GlobalConfigure.h>
#include <libethcore/Block.h>
#include <libethcore/Transaction.h>
#include <libexecutive/Executive.h>
#include <map>
#include <memory>
#include <queue>
#include <vector>

namespace dev
{
namespace blockverifier
{
using ExecuteTxFunc =
    std::function<bool(dev::eth::Transaction::Ptr, ID, dev::executive::Executive::Ptr)>;

class TxDAGFace_V
{
public:
    virtual bool hasFinished() = 0;
    virtual ~TxDAGFace_V() {}

    // Called by thread
    // Execute a unit in DAG
    // This function can be parallel

    virtual void getHaventRun(){};
};

class TxDAG_V : public TxDAGFace_V
{
public:
    DAG_V m_dag;
    TxDAG_V() : m_dag() {}
    virtual ~TxDAG_V() {}
    DAG_V* getDag() { return &m_dag;}


    // Generate DAG according with given transactions
    void init(ExecutiveContext::Ptr _ctx, std::shared_ptr<dev::eth::Transactions> _txs,
        int64_t _blockHeight);

    // Set transaction execution function
    void setTxExecuteFunc(ExecuteTxFunc const& _f);
    // Called by VMCase.cpp
    void consume(ID _id);

    bool isReady(ID _id);

    // Called by thread
    // Has the DAG reach the end?
    // process-exit related:
    // if the g_BCOSConfig.shouldExit is true(may be the storage has exceptioned), return true
    // directly
    bool hasFinished() override
    {
        return (m_exeCnt >= m_totalParaTxs) || (g_BCOSConfig.shouldExit.load());
    }

    // Called by thread
    // Execute a unit in DAG
    // This function can be parallel
//    int executeUnit(dev::executive::Executive::Ptr _executive) override;

    ID paraTxsNumber() { return m_totalParaTxs; }

    ID haveExecuteNumber() { return m_exeCnt; }

    void clear();


private:
    ExecuteTxFunc f_executeTx;
    std::shared_ptr<dev::eth::Transactions const> m_txs;


    ID m_exeCnt = 0;
    ID m_totalParaTxs = 0;

    mutable std::mutex x_exeCnt;
};


template <typename T>
class CriticalField_V
{
public:
    ID get(T const& _c)
    {
        auto it = m_criticals.find(_c);
        if (it == m_criticals.end())
        {
            if (m_criticalAll != INVALID_ID_2)
                return m_criticalAll;
            return INVALID_ID_2;
        }
        return it->second;
    }

    void update(T const& _c, ID _txId) { m_criticals[_c] = _txId; }
private:
    std::map<T, ID> m_criticals;
//    std::map<T, std::vector<ID>> m_criticals;
    ID m_criticalAll = INVALID_ID_2;
};

}  // namespace blockverifier
}  // namespace dev
