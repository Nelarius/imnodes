#pragma once

#include <algorithm>
#include <cassert>
#include <stack>
#include <stddef.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace example
{
// a very simple directional graph
template<typename NodeType>
class Graph
{
public:
    Graph()
        : current_id_(0), nodes_(), edges_from_node_(), edges_to_node_(),
          edges_()
    {
    }

    struct Edge
    {
        int from, to;

        Edge() = default;
        Edge(const int f, const int t) : from(f), to(t) {}

        inline int opposite(const int n) const { return n == from ? to : from; }
    };

    using edge_iterator =
        typename std::unordered_map<int, Edge>::const_iterator;

    struct EdgeSpan
    {
        explicit EdgeSpan(edge_iterator begin, edge_iterator end)
            : begin_(begin), end_(end)
        {
        }

        edge_iterator begin() const { return begin_; }
        edge_iterator end() const { return end_; }

    private:
        edge_iterator begin_;
        edge_iterator end_;
    };

    // Element access

    NodeType& node(int node_id);
    const NodeType& node(int node_id) const;

    EdgeSpan edges() const;

    // Capacity

    size_t num_adjacencies_from_node(int node_id) const;
    size_t num_adjacencies_to_node(int node_id) const;

    // Modifiers

    int insert_node(const NodeType& node);
    void erase_node(int node_id);

    int insert_edge(int from, int to);
    void erase_edge(int edge_id);

private:
    template<typename T, typename Visitor>
    void dfs_traverse(const Graph<T>& graph, int start_node, Visitor visitor);

    int current_id_;
    std::unordered_map<int, NodeType> nodes_;
    std::unordered_map<int, std::vector<int>> edges_from_node_;
    std::unordered_map<int, std::vector<int>> edges_to_node_;
    std::unordered_map<int, Edge> edges_;
};

template<typename NodeType>
NodeType& Graph<NodeType>::node(const int id)
{
    return const_cast<NodeType&>(static_cast<const Graph*>(this)->node(id));
}

template<typename NodeType>
const NodeType& Graph<NodeType>::node(const int id) const
{
    assert(nodes_.find(id) != nodes_.end());
    return nodes_.at(id);
}

template<typename NodeType>
typename Graph<NodeType>::EdgeSpan Graph<NodeType>::edges() const
{
    return EdgeSpan(edges_.begin(), edges_.end());
}

template<typename NodeType>
size_t Graph<NodeType>::num_adjacencies_from_node(const int id) const
{
    auto iter = edges_from_node_.find(id);
    assert(iter != edges_from_node_.end());
    return iter->second.size();
}

template<typename NodeType>
size_t Graph<NodeType>::num_adjacencies_to_node(const int id) const
{
    auto iter = edges_to_node_.find(id);
    assert(iter != edges_to_node_.end());
    return iter->second.size();
}

template<typename NodeType>
int Graph<NodeType>::insert_node(const NodeType& node)
{
    const int id = current_id_++;
    nodes_.insert(std::make_pair(id, node));
    edges_from_node_.insert(std::make_pair(id, std::vector<int>()));
    edges_to_node_.insert(std::make_pair(id, std::vector<int>()));
    return id;
}

template<typename NodeType>
void Graph<NodeType>::erase_node(const int id)
{
    // first, collect all the edges from the adjacency lists
    // since erasing an edge invalidates the adjacency list iterators
    static std::vector<int> edges_to_erase;
    for (const int edge : edges_from_node_[id])
    {
        edges_to_erase.push_back(edge);
    }
    for (const int edge : edges_to_node_[id])
    {
        edges_to_erase.push_back(edge);
    }
    for (const int edge : edges_to_erase)
    {
        erase_edge(edge);
    }
    edges_to_erase.clear();
    nodes_.erase(id);
    edges_from_node_.erase(id);
    edges_to_node_.erase(id);
}

template<typename NodeType>
int Graph<NodeType>::insert_edge(const int from, const int to)
{
    const int id = current_id_++;
    edges_.insert(std::make_pair(id, Edge(from, to)));
    edges_from_node_[static_cast<size_t>(from)].push_back(id);
    edges_to_node_[static_cast<size_t>(to)].push_back(id);
    return id;
}

template<typename NodeType>
void Graph<NodeType>::erase_edge(const int edge_id)
{
    auto edge = edges_.find(edge_id);
    assert(edge != edges_.end());

    {
        auto& edges_from = edges_from_node_[edge->second.from];
        auto iter = std::find(edges_from.begin(), edges_from.end(), edge_id);
        assert(iter != edges_from.end());
        edges_from.erase(iter);
    }

    {
        auto& edges_to = edges_to_node_[edge->second.to];
        auto iter = std::find(edges_to.begin(), edges_to.end(), edge_id);
        assert(iter != edges_to.end());
        edges_to.erase(iter);
    }

    edges_.erase(edge);
}

template<typename NodeType, typename Visitor>
void dfs_traverse(
    const Graph<NodeType>& /*graph*/,
    const int /*start_node*/,
    Visitor /*visitor*/)
{
    // TODO
}
} // namespace example
