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

#include "DAG.h"
#include <libconfig/GlobalConfigure.h>
using namespace std;
using namespace dev;
using namespace dev::blockverifier;


Array* to_delete;
Array* deleted;
Array* pile;

//fold
char is_folding(int u, Graph *graph)
{
    int degree_u = get_degree(u, graph);

    if (degree_u == 2)
    {
        return 1;
    }
    else if (degree_u == 3 || degree_u == 4)
    {
        int* adj_u = graph->adjacency[u];
        for (int i = 0; i < graph->adj_size[u]; i++)
        {
            int x = adj_u[i];

            if (OUT & graph->mark[x])
                continue;

            for (int j = i + 1; j < graph->adj_size[u]; j++)
            {
                int y = adj_u[j];

                if (OUT & graph->mark[y])
                    continue;

                if (are_connected(x, y, graph))
                    continue;

                for (int k = j + 1; k < graph->adj_size[u]; k++)
                {
                    int z = adj_u[k];

                    if (OUT & graph->mark[z])
                        continue;

                    if (are_connected(x, z, graph)
                        || are_connected(y, z, graph))
                        continue;

                    return 0;
                }
            }
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

void
fold(int v, Graph *graph)
{
    int old_size = graph->size;

    int* adj_v = graph->adjacency[v];

    for (int i = 0; i < graph->adj_size[v]; i++)
    {
        int x = adj_v[i];

        if (OUT & graph->mark[x])
            continue;

        graph->mark[x] |= OUT;
        graph->mark[x] |= MARKED;
        push(x, deleted);
    }
    graph->mark[v] |= OUT;
    graph->mark[v] |= MARKED;
    push(v, deleted);

    for (int i = 0; i < graph->adj_size[v]; i++)
    {
        int x = adj_v[i];

        if (!(MARKED & graph->mark[x]))
            continue;

        for (int j = i + 1; j < graph->adj_size[v]; j++)
        {
            int y = adj_v[j];

            if (!(MARKED & graph->mark[y]))
                continue;

            if (!are_connected(x, y, graph))
                add_tild(v, x, y, graph);
        }
    }

    for (int i = old_size; i < graph->size; i++)
    {
        for (int j = i + 1; j < graph->size; j++)
        {
            add_edge(i, j, graph);
        }
    }

    for (int i = 0; i < graph->adj_size[v]; i++)
    {
        int x = adj_v[i];
        graph->mark[x] |= MARKED;
        graph->mark[x] ^= MARKED;
    }
    graph->mark[v] ^= MARKED;
}

void
add_tild(int v, int x, int y, Graph *graph)
{
    int xy_tild = add_vertex(graph);
    graph->v[xy_tild] = v;
    graph->x[xy_tild] = x;
    graph->y[xy_tild] = y;

    int* adj_x = graph->adjacency[x];
    int* adj_y = graph->adjacency[y];

    for (int i = 0; i < graph->adj_size[x]; i++)
    {
        int z = adj_x[i];

        if (OUT & graph->mark[z])
            continue;

        add_edge(z, xy_tild, graph);
        graph->mark[z] |= MARKED;
    }

    for (int j = 0; j < graph->adj_size[y]; j++)
    {
        int z = adj_y[j];

        if ((OUT & graph->mark[z]) || (MARKED & graph->mark[z]))
            continue;

        add_edge(z, xy_tild, graph);
    }

    int* adj_xy_tild = graph->adjacency[xy_tild];

    for (int k = 0; k < graph->adj_size[xy_tild]; k++)
    {
        int z = adj_xy_tild[k];
        graph->mark[z] |= MARKED;
        graph->mark[z] ^= MARKED;
        if (TO_CHECK & graph->mark[z])
            continue;
        graph->mark[z] |= TO_CHECK;
        push(z, pile);
    }
    graph->mark[xy_tild] |= TO_CHECK;
    push(xy_tild, pile);
}

void
unfold(int v, int mem, Graph *graph)
{
    while (deleted->size != mem)
        graph->mark[pop(deleted)] ^= OUT;

    int new_size = graph->size;
    while (new_size - 1 >= 0 && graph->v[new_size - 1] == v)
    {
        new_size--;
        int* adj = graph->adjacency[new_size];
        for (int i = 0; i < graph->adj_size[new_size]; i++)
            graph->adj_size[adj[i]]--;
    }

    graph->size = new_size;
}

// mirrors
void
get_mirrors(int v, Graph *graph)
{
    push(v, pile);
    graph->mark[v] |= VISITED;

    int* adj_v = graph->adjacency[v];

    for (int j = 0; j < graph->adj_size[v]; j++)
    {
        int w = adj_v[j];

        if (OUT & graph->mark[w])
            continue;

        graph->mark[w] |= VISITED;
        push(w, pile);
    }

    for (int j = 0; j < graph->adj_size[v]; j++)
    {
        int w = adj_v[j];

        if (OUT & graph->mark[w])
            continue;

        int* adj_w = graph->adjacency[w];
        for (int k = 0; k < graph->adj_size[w]; k++)
        {
            int u = adj_w[k];

            if ((OUT | VISITED) & graph->mark[u])
                continue;

            if (check_delta(v, u, graph))
                push(u, to_delete);

            graph->mark[u] |= VISITED;
            push(u, pile);
        }
    }

    while (pile->size != 0)
    {
        int u = pop(pile);
        graph->mark[u] |= VISITED;
        graph->mark[u] ^= VISITED;
    }
}

char
check_delta(int v, int u, Graph *graph)
{
    int* adj_v = graph->adjacency[v];
    int* adj_u = graph->adjacency[u];

    for (int i = 0; i < graph->adj_size[u]; i++)
    {
        int x = adj_u[i];
        if (OUT & graph->mark[x])
            continue;

        graph->mark[x] |= MARKED;
    }

    char r = 1;
    for (int i = 0; i < graph->adj_size[v] && r; i++)
    {
        int x = adj_v[i];

        if ((OUT | MARKED) & graph->mark[x])
            continue;

        for (int j = i + 1; j < graph->adj_size[v] && r; j++)
        {
            int y = adj_v[j];
            if ((OUT | MARKED) & graph->mark[y])
                continue;

            r &= are_connected(x, y, graph);
        }
    }

    for (int i = 0; i < graph->adj_size[u]; i++)
    {
        int x = adj_u[i];
        graph->mark[x] |= MARKED;
        graph->mark[x] ^= MARKED;
    }

    return r;
}

//array
Array*
new_array()
{
    Array* array = (struct Array*) malloc(sizeof(struct Array));
    array->t = (int*)malloc(sizeof(int));
    array->size = 0;
    array->capacity = 1;
    return array;
}

void
push(int x, Array * array)
{
    if (array->size == array->capacity)
    {
        int* t = array->t;
        int* new_t = (int*)malloc((array->capacity << 1) * sizeof(int));
        for (int i = 0; i < array->size; i++)
            new_t[i] = t[i];
        free(t);
        array->t = new_t;
        array->capacity <<= 1;
    }
    array->t[array->size] = x;
    array->size++;
}

int
pop(Array * array)
{
    array->size--;
    return array->t[array->size];
}

void
free_array(Array * array)
{
    free(array->t);
    free(array);
}

void
conc(Array * a, Array * b)
{
    int new_capacity = b->capacity;
    while (new_capacity < a->size + b->size)
        new_capacity <<= 1;

    if (new_capacity != b->capacity)
    {
        int* t = b->t;
        int* new_t = (int*)malloc(new_capacity * sizeof(int));
        for (int i = 0; i < b->size; i++)
        {
            new_t[i] = t[i];
        }
        free(t);
        b->t = new_t;
        b->capacity = new_capacity;
    }

    while (a->size != 0)
        push(pop(a), b);

    free_array(a);
}

int
compare_int(const void* a, const void* b)
{
    const int* c = (const int*)a;
    const int* d = (const int*)b;
    return (*c > * d) - (*c < *d);
}

void
sort_array(Array * array)
{
    int* t = array->t;
    int size = array->size;

    qsort(t, size, sizeof(int), compare_int);
}

void
write_array(Array * l, char* filename)
{
    FILE* file = fopen(filename, "w+");
    if (file == NULL)
    {
        printf("Error: unable to write to output file\n");
        exit(1);
    }

    sort_array(l);
    for (int i = 0; i < l->size; i++)
        fprintf(file, "%d\n", l->t[i]);

    fclose(file);
}

// graph
Graph*
init_graph(int size)
{
    Graph* graph = (struct Graph*) malloc(sizeof(struct Graph));

    graph->size = size;
    graph->capacity = 1;

    while (graph->capacity < graph->size)
        graph->capacity <<= 1;

    graph->adjacency = (int**)malloc(graph->capacity * sizeof(int*));
    graph->adj_size = (int*)malloc(5 * graph->capacity * sizeof(int));
    graph->mark = (char*)malloc(graph->capacity * sizeof(char));

    graph->dele = (int*)malloc(graph->capacity * sizeof(int));

    for(int i = 0; i < graph->capacity; ++i) {
        graph->dele[i] = 0;
    }

    graph->adj_capacity = graph->adj_size + graph->capacity;
    graph->v = graph->adj_capacity + graph->capacity;
    graph->x = graph->v + graph->capacity;
    graph->y = graph->x + graph->capacity;

    for (int u = 0; u < graph->capacity; u++)
    {
        graph->adjacency[u] = (int*)malloc(sizeof(int));
        graph->adj_capacity[u] = 1;
    }
    for (int u = 0; u < graph->size; u++)
    {
        graph->adj_size[u] = 0;
        graph->mark[u] = 0;
        graph->v[u] = -1;
    }

    return graph;
}

int
add_vertex(Graph * graph)
{
    if (graph->size == graph->capacity)
        extend_graph(graph);

    int u = graph->size;
    graph->size++;
    graph->mark[u] = 0;
    graph->v[u] = -1;
    graph->adj_size[u] = 0;

    return u;
}

int
remove_vertex(Graph *graph, int u) {
    if(u >= graph->size) {
        return 0;
    }
    graph->dele[u] = 1;
    for(int i = 0; i < graph->adj_size[u]; ++i) {
        int v = graph->adjacency[u][i];
        int n = 0;
        for(int j = 0; j < graph->adj_size[v]; ++j) {
            if(graph->adjacency[v][j] != u) {
                graph->adjacency[v][n] = graph->adjacency[v][j];
                ++n;
            }
        }
        graph->adj_size[v] = n;
    }
    graph->adj_size[u] = 0;
    return 1;
}

void
add_edge(int u, int v, Graph * graph)
{
    int adj_capacity_u = graph->adj_capacity[u];
    int* adj_u = graph->adjacency[u];
    if (graph->adj_size[u] == adj_capacity_u)
    {
        int* new_adj_u = (int*)malloc((adj_capacity_u << 1) * sizeof(int));
        for (int i = 0; i < adj_capacity_u; i++)
        {
            new_adj_u[i] = adj_u[i];
        }
        free(adj_u);
        graph->adjacency[u] = new_adj_u;
        graph->adj_capacity[u] <<= 1;
    }
    graph->adjacency[u][graph->adj_size[u]++] = v;

    int adj_capacity_v = graph->adj_capacity[v];
    int* adj_v = graph->adjacency[v];
    if (graph->adj_size[v] == adj_capacity_v)
    {
        int* new_adj_v = (int*)malloc((adj_capacity_v << 1) * sizeof(int));
        for (int j = 0; j < adj_capacity_v; j++)
        {
            new_adj_v[j] = adj_v[j];
        }
        free(adj_v);
        graph->adjacency[v] = new_adj_v;
        graph->adj_capacity[v] <<= 1;
    }
    graph->adjacency[v][graph->adj_size[v]++] = u;
}

void
extend_graph(Graph * graph)
{
    int new_capacity = graph->capacity << 1;

    int** new_adjacency = (int**)malloc(new_capacity * sizeof(int*));
    int* new_adj_size = (int*)malloc(5 * new_capacity * sizeof(int));
    char* new_mark = (char*)malloc(new_capacity * sizeof(char));

    int *new_dele = (int*)malloc(new_capacity * sizeof(int));
    for(int i = 0; i < new_capacity; ++i) {
        new_dele[i] = 0;
    }

    int* new_adj_capacity = new_adj_size + new_capacity;
    int* new_v = new_adj_capacity + new_capacity;
    int* new_x = new_v + new_capacity;
    int* new_y = new_x + new_capacity;

    for (int u = 0; u < graph->capacity; u++)
    {
        new_dele[u] = graph->dele[u];
        new_adjacency[u] = graph->adjacency[u];
        new_mark[u] = graph->mark[u];
        new_v[u] = graph->v[u];
        new_x[u] = graph->x[u];
        new_y[u] = graph->y[u];
        new_adj_size[u] = graph->adj_size[u];
        new_adj_capacity[u] = graph->adj_capacity[u];
    }
    for (int u = graph->capacity; u < new_capacity; u++)
    {
        new_adjacency[u] = (int*)malloc(sizeof(int));
        new_adj_capacity[u] = 1;
    }

    free(graph->adjacency);
    free(graph->mark);
    free(graph->adj_size);

    graph->adjacency = new_adjacency;
    graph->adj_size = new_adj_size;
    graph->adj_capacity = new_adj_capacity;
    graph->mark = new_mark;
    graph->v = new_v;
    graph->x = new_x;
    graph->y = new_y;
    graph->dele = new_dele;

    graph->capacity = new_capacity;
}

void
free_graph(Graph * graph)
{
    for (int u = 0; u < graph->capacity; u++)
        free(graph->adjacency[u]);

    free(graph->adjacency);
    free(graph->adj_size);
    free(graph->mark);
    free(graph->dele);

    free(graph);
}

int
get_degree(int u, Graph * graph)
{
    int degree_u = 0;
    int* adj_u = graph->adjacency[u];
    for (int i = 0; i < graph->adj_size[u]; i++)
    {
        if (!(OUT & graph->mark[adj_u[i]]))
            degree_u++;
    }
    return degree_u;
}

char
are_connected(int u, int v, Graph * graph)
{
    int* adj_v = graph->adjacency[v];
    for (int i = 0; i < graph->adj_size[v]; i++)
    {
        if (adj_v[i] == u)
            return 1;
    }
    return 0;
}



Array*
mis(Graph * graph)
{
    // graph = input;

    to_delete = new_array();
    deleted = new_array();
    pile = new_array();

    for (int u = 0; u < graph->size; u++)
    {
        graph->mark[u] |= TO_CHECK;
        push(u, pile);
    }

    Array* l = check(graph);

    while (deleted->size != 0)
        graph->mark[pop(deleted)] ^= OUT;

    free_array(to_delete);
    free_array(deleted);
    free_array(pile);

    int n = 0;
    for(int i = 0; i < l->size; ++i) {
        if(!(graph->dele[l->t[i]])) {
            l->t[n] = l->t[i];
            ++n;
        }
    }
    l->size = n;

    return l;
}

Array*
core(Graph *graph)
{
    int d_max = -1, v;
    for (int w = 0; w < graph->size; w++)
    {
        if (OUT & graph->mark[w])
            continue;

        int degree_w = get_degree(w, graph);
        if (d_max < degree_w)
        {
            d_max = degree_w;
            v = w;
        }
    }

    int mem = deleted->size;

    get_mirrors(v, graph);
    push(v, to_delete);
    vertex_out(graph);

    Array* l1 = check(graph);

    while (deleted->size != mem)
        graph->mark[pop(deleted)] ^= OUT;

    int* adj_v = graph->adjacency[v];
    push(v, to_delete);
    for (int i = 0; i < graph->adj_size[v]; i++)
        push(adj_v[i], to_delete);
    vertex_out(graph);

    Array * l2 = check(graph);
    push(v, l2);

    while (deleted->size != mem)
        graph->mark[pop(deleted)] ^= OUT;

    if (l1->size < l2->size)
    {
        free_array(l1);
        return l2;
    }
    else
    {
        free_array(l2);
        return l1;
    }
}

Array*
check(Graph *graph)
{
    while (pile->size != 0)
    {
        int v = pop(pile);

        if (OUT & graph->mark[v])
        {
            graph->mark[v] ^= TO_CHECK;
            continue;
        }

        if (is_folding(v, graph))
        {
            graph->mark[v] ^= TO_CHECK;
            int mem = deleted->size;
            fold(v, graph);
            Array* l = check(graph);
            unfold(v, mem, graph);

            for (int i = 0; i < l->size; i++)
            {
                int u = l->t[i];

                if (graph->v[u] == v)
                {
                    l->t[i] = graph->x[u];
                    push(graph->y[u], l);
                    return l;
                }
            }
            push(v, l);
            return l;
        }

        int* adj_v = graph->adjacency[v];
        int degree_v = 0;
        for (int j = 0; j < graph->adj_size[v]; j++)
        {
            int u = adj_v[j];

            if (OUT & graph->mark[u])
                continue;

            degree_v++;
            graph->mark[u] |= MARKED;
        }
        graph->mark[v] |= MARKED;

        for (int j = 0; j < graph->adj_size[v]; j++)
        {
            int u = adj_v[j];

            if (OUT & graph->mark[u])
                continue;

            int* adj_u = graph->adjacency[u];

            int degree_uv = 0;
            for (int i = 0; i < graph->adj_size[u]; i++)
            {
                if (MARKED & graph->mark[adj_u[i]])
                    degree_uv++;
            }

            if (degree_v == degree_uv)
            {
                graph->mark[u] |= OUT;
                graph->mark[u] |= MARKED;
                graph->mark[u] ^= MARKED;
                push(u, deleted);

                for (int i = 0; i < graph->adj_size[u]; i++)
                {
                    int x = adj_u[i];
                    if ((OUT | TO_CHECK) & graph->mark[x])
                        continue;
                    graph->mark[x] |= TO_CHECK;
                    push(x, pile);
                }

                degree_v--;
                j = -1;
            }
        }

        for (int j = 0; j < graph->adj_size[v]; j++)
        {
            int u = adj_v[j];
            graph->mark[u] |= MARKED;
            graph->mark[u] ^= MARKED;
        }
        graph->mark[v] ^= MARKED;
        graph->mark[v] ^= TO_CHECK;
    }

    return apply_to_components(graph);
}

Array*
apply_to_components(Graph *graph)
{
    int mem = deleted->size;

    for (int u0 = 0; u0 < graph->size; u0++)
    {
        if (OUT & graph->mark[u0])
            continue;

        push(-1, deleted);

        graph->mark[u0] |= OUT;
        push(u0, deleted);
        push(u0, pile);
        while (pile->size != 0)
        {
            int u = pop(pile);

            int* adj_u = graph->adjacency[u];
            for (int i = 0; i < graph->adj_size[u]; i++)
            {
                int v = adj_u[i];

                if (OUT & graph->mark[v])
                    continue;

                graph->mark[v] |= OUT;
                push(v, deleted);
                push(v, pile);
            }
        }
    }

    Array* l = new_array();

    int i = deleted->size - 1;
    while (mem <= i)
    {
        for (int j = i; deleted->t[j] != -1; j--)
        {
            graph->mark[deleted->t[j]] ^= OUT;
        }

        conc(core(graph), l);

        while (deleted->t[i] != -1)
        {
            graph->mark[deleted->t[i]] |= OUT;
            i--;
        }
        i--;
    }

    while (deleted->size != mem)
    {
        int u = pop(deleted);
        if (u != -1)
            graph->mark[u] ^= OUT;
    }

    return l;
}

void
vertex_out(Graph *graph)
{
    while (to_delete->size != 0)
    {
        int u = pop(to_delete);

        if (OUT & graph->mark[u])
            continue;

        graph->mark[u] |= OUT;
        push(u, deleted);

        int* adj_u = graph->adjacency[u];
        for (int i = 0; i < graph->adj_size[u]; i++)
        {
            int v = adj_u[i];
            if ((OUT | TO_CHECK) & graph->mark[v])
                continue;
            graph->mark[v] |= TO_CHECK;
            push(v, pile);
        }
    }
}



DAG::~DAG()
{
    clear();
}

void DAG::init(ID _maxSize)
{
    clear();
    for (ID i = 0; i < _maxSize; ++i)
        m_vtxs.emplace_back(make_shared<Vertex>());
    m_totalVtxs = _maxSize;
    m_totalConsume = 0;
    graph = init_graph(_maxSize);
}

void DAG::verifyInit(ID _size)
{
    for (ID i = 0; i < _size; i ++)
    {
        m_topLevel.push(i);
    }
}

void DAG::addEdge(ID _f, ID _t)
{
    if (_f >= m_vtxs.size() && _t >= m_vtxs.size())
        return;
    if (are_connected((int)_f,(int)_t, graph) == 0)
    {
        std::cout << "addEdge From " << _f << " to " << _t << std::endl;
        add_edge((int)_f,(int)_t, graph);
    }

    PARA_LOG(TRACE) << LOG_BADGE("DAG") << LOG_DESC("Add edge") << LOG_KV("from", _f)
                    << LOG_KV("to", _t);
}

void DAG::generate()
{
//    for (ID id = 0; id < m_vtxs.size(); ++id)
//    {
//        if (m_vtxs[id]->inDegree == 0)
//            m_topLevel.push(id);
//    }
    while (1)
    {
        Array *l = mis(graph);
        if ((uint32_t)l->size != 0)
        {
            for (ID id = 0; id < (uint32_t)l->size; ++id)
            {
                std::cout << " push " << (int)l->t[id];
                m_topLevel.push((uint32_t)(l->t[id]));
                remove_vertex(graph, (int)(l->t[id]));
            }
        }
        else {
            std::cout<< " " << std::endl;
            break;
        }

    }
    // PARA_LOG(TRACE) << LOG_BADGE("DAG") << LOG_DESC("generate")
    //                << LOG_KV("queueSize", m_topLevel.size());
    // for (ID id = 0; id < m_vtxs.size(); id++)
    // printVtx(id);
}

/*
ID DAG::waitPop(bool _needWait)
{
    std::unique_lock<std::mutex> ul(x_topLevel);
    ID top;
    while (m_totalConsume < m_totalVtxs)
    {
        auto has = m_topLevel.try_pop(top);
        if (has)
        {
            return top;
        }
        else
        {
            if (_needWait)
            {
                cv_topLevel.wait_for(ul, std::chrono::milliseconds(10),
                    [&]() { return m_totalConsume >= m_totalVtxs || m_topLevel.size() > 0; });
                ul.unlock();
            }
            else
                break;
        }
    }
    return INVALID_ID;
}
*/

ID DAG::waitPop(bool _needWait)
{
    std::unique_lock<std::mutex> ul(x_topLevel);
    ID top = INVALID_ID;
    cv_topLevel.wait_for(ul, std::chrono::milliseconds(10), [&]() {
        auto has = m_topLevel.try_pop(top);
        if (has)
        {
            return true;
        }
        else if (m_totalConsume >= m_totalVtxs)
        {
            return true;
        }
        else if (!_needWait)
        {
            return true;
        }
            // process-exit related:
            // if the g_BCOSConfig.shouldExit is true (may be the storage has exceptioned)
            // return true, will pop INVALID_ID
        else if (g_BCOSConfig.shouldExit.load())
        {
            return true;
        }

        return false;
    });
    return top;
}

ID DAG::consume(ID _id)
{
    ID producedNum = 0;
    ID nextId = INVALID_ID;
    ID lastDegree = INVALID_ID;

    if (m_totalConsume.fetch_add(1) + 1 == m_totalVtxs)
    {
        cv_topLevel.notify_all();
    }
    // PARA_LOG(TRACE) << LOG_BADGE("TbbCqDAG") << LOG_DESC("consumed")
    //                << LOG_KV("queueSize", m_topLevel.size());
    // for (ID id = 0; id < m_vtxs.size(); id++)
    // printVtx(id);
    return nextId;
}

void DAG::clear()
{
    m_vtxs = std::vector<std::shared_ptr<Vertex>>();
    // XXXX m_topLevel.clear();
}

void DAG::printVtx(ID _id)
{
    for (ID id : m_vtxs[_id]->outEdge)
    {
        PARA_LOG(TRACE) << LOG_BADGE("DAG") << LOG_DESC("VertexEdge") << LOG_KV("ID", _id)
                        << LOG_KV("inDegree", m_vtxs[_id]->inDegree) << LOG_KV("edge", id);
    }
}


