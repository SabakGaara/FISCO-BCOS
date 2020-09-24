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
 * @brief : DAG(Directed Acyclic Graph) basic implement
 * @author: jimmyshi
 * @date: 2019-1-8
 */

#include "DAG_V.h"
#include <libconfig/GlobalConfigure.h>
using namespace std;
using namespace dev;

DAG_V::~DAG_V()
{
    clear();
}

void DAG_V::init(ID _maxSize)
{
    clear();
    for (ID i = 0; i < _maxSize; ++i)
        m_vtxs.emplace_back(make_shared<Vertex>());
    m_totalVtxs = _maxSize;
    m_totalConsume = 0;
}

void DAG_V::addEdge(ID _f, ID _t)
{
    if (_f >= m_vtxs.size() && _t >= m_vtxs.size())
        return;
    m_vtxs[_f]->outEdge.emplace_back(_t);
    m_vtxs[_t]->inDegree += 1;
    std::cout << "addEdge From " << _f << " to " << _t << std::endl;
    // PARA_LOG(TRACE) << LOG_BADGE("DAG") << LOG_DESC("Add edge") << LOG_KV("from", _f)
    //                << LOG_KV("to", _t);
}

void DAG_V::generate()
{
    for (ID id = 0; id < m_vtxs.size(); ++id)
    {
        if (m_vtxs[id]->inDegree == 0)
            m_topLevel.push(id);
    }

    // PARA_LOG(TRACE) << LOG_BADGE("DAG") << LOG_DESC("generate")
    //                << LOG_KV("queueSize", m_topLevel.size());
    // for (ID id = 0; id < m_vtxs.size(); id++)
    // printVtx(id);
}

bool DAG_V::waitPop(ID _id)
{
    // Note: concurrent_queue of TBB can't be used with boost::conditional_variable
    //       the try_pop will already be false
    std::unique_lock<std::mutex> ul(x_topLevel);
    cv_topLevel.wait_for(ul, std::chrono::milliseconds(10), [&]() {
        if (m_vtxs[_id]->inDegree == 0)
        {
            return true;
        }
    });
    return false;
}

ID DAG_V::consume(ID _id)
{
    ID producedNum = 0;
    ID nextId = INVALID_ID;
    ID lastDegree = INVALID_ID;
    for (ID id : m_vtxs[_id]->outEdge)
    {
        auto vtx = m_vtxs[id];
        {
            lastDegree = vtx->inDegree.fetch_sub(1);
        }
        if (lastDegree == 1)
        {
//            m_topLevel.push(id);
            cv_topLevel.notify_one();
        }
    }

    // PARA_LOG(TRACE) << LOG_BADGE("TbbCqDAG") << LOG_DESC("consumed")
    //                << LOG_KV("queueSize", m_topLevel.size());
    // for (ID id = 0; id < m_vtxs.size(); id++)
    // printVtx(id);
    return nextId;
}

void DAG_V::clear()
{
    m_vtxs = std::vector<std::shared_ptr<Vertex>>();
    // XXXX m_topLevel.clear();
}

void DAG_V::printVtx(ID _id)
{
    for (ID id : m_vtxs[_id]->outEdge)
    {
        PARA_LOG(TRACE) << LOG_BADGE("DAG") << LOG_DESC("VertexEdge") << LOG_KV("ID", _id)
                        << LOG_KV("inDegree", m_vtxs[_id]->inDegree) << LOG_KV("edge", id);
    }
}