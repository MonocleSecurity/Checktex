#ifdef _WIN32
  #pragma warning(disable:26110)
#endif

#include "checktex.hpp"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace checktex
{

std::atomic<uint64_t> mutex_id_counter(1);

class Node
{
public:
  Node(const uint64_t mutex_id, const char* mutex_name)
    : mutex_id_(mutex_id)
    , mutex_name_(mutex_name)
  {
  }

  Node(const Node& rhs)
    : mutex_id_(rhs.mutex_id_)
    , mutex_name_(rhs.mutex_name_)
    , children_(rhs.children_)
  {
  }

  ~Node()
  {
  }

  Node& operator=(const Node& rhs)
  {
    mutex_id_ = rhs.mutex_id_;
    mutex_name_ = rhs.mutex_name_;
    children_ = rhs.children_;
    return *this;
  }

  bool operator==(const Node& rhs) const
  {
    return (mutex_id_ == rhs.mutex_id_);
  }

  uint64_t mutex_id_;
  const char* mutex_name_;
  std::vector<Node> children_;
};

class Graph
{
public:
  Graph()
    : tree_(0, nullptr)
  {
  }

  Graph(const Graph&) = delete;

  ~Graph()
  {
    if (tree_.children_.size())
    {
      std::lock_guard<std::mutex> lock(mutex_);
      trees_.push_back(tree_);
    }
  }

  void AddNode(const uint64_t mutex_id, const char* mutex_name)
  {
    // Build tree
    if (current_vector_.empty())
    {
      // Insert if it doesn't already exist
      if (std::find_if(tree_.children_.cbegin(), tree_.children_.cend(), [mutex_id](const Node& node) { return (node.mutex_id_ == mutex_id); }) == tree_.children_.cend())
      {
        tree_.children_.push_back(Node(mutex_id, mutex_name));
      }
    }
    else
    {
      std::vector<Node>* current_node = &tree_.children_;
      for (const uint64_t mutex_id : current_vector_)
      {
        std::vector<Node>::iterator n = std::find_if(current_node->begin(), current_node->end(), [mutex_id](const Node& node) { return (node.mutex_id_ == mutex_id); });
        if (n == current_node->end())
        {
          std::cout << "Could not find node vector" << std::endl;
          abort();
          return;
        }
        current_node = &n->children_;
      }
      // Insert if it doesn't already exist
      if (std::find_if(current_node->cbegin(), current_node->cend(), [mutex_id](const Node& node) { return (node.mutex_id_ == mutex_id); }) == current_node->cend())
      {
        current_node->push_back(Node(mutex_id, mutex_name));
      }
    }
    // Current route
    current_vector_.push_back(mutex_id);
  }

  void RemoveNode(const uint64_t mutex_id)
  {
    std::vector<uint64_t>::const_reverse_iterator i = std::find(current_vector_.crbegin(), current_vector_.crend(), mutex_id);
    if (i == current_vector_.crend())
    {
      std::cout << "Could not find node" << std::endl;
      abort();
      return;
    }
    current_vector_.erase(std::next(i).base());
  }

  Node tree_;
  std::vector<uint64_t> current_vector_;

  static std::mutex mutex_;
  static std::vector<Node> trees_;
};

std::mutex Graph::mutex_;
std::vector<Node> Graph::trees_;
thread_local Graph graph;

Mutex::Mutex()
  : Mutex(nullptr)
{
}

Mutex::Mutex(const char* name)
#ifndef NDEBUG
  : id_(mutex_id_counter.fetch_add(1, std::memory_order_relaxed))
  , name_(name)
#endif
{
}

Mutex::~Mutex()
{
}

void Mutex::lock()
{
#ifndef NDEBUG
  graph.AddNode(id_, name_);
#endif
  mutex_.lock();
}

void Mutex::unlock()
{
#ifndef NDEBUG
  graph.RemoveNode(id_);
#endif
  mutex_.unlock();
}

int NumDigits(uint64_t number)
{
  int digits = 0;
  while (number)
  {
    number /= 10;
    ++digits;
  }
  return digits;
}

void PrintNode(const Node& node, const std::string spaces, const bool first)
{
  if (first == false)
  {
    std::cout << spaces;
  }
  std::cout << node.mutex_id_ << " ";
  if (node.children_.size())
  {
    std::cout << "-> ";
  }
  else // Newlines only for leafs
  {
    std::cout << '\n';
  }
  bool f = true;
  for (const Node& n : node.children_)
  {
    const int num_digits = NumDigits(node.mutex_id_);
    PrintNode(n, spaces + std::string(num_digits + 1 + 3, ' '), f);
    f = false;
  }
}

void PruneNode(Node& node, std::vector<Node>& current_nodes)
{
  current_nodes.push_back(node);
  for (std::vector<Node>::iterator n = node.children_.begin(); n != node.children_.end();)
  {
    if (std::find(current_nodes.cbegin(), current_nodes.cend(), *n) != current_nodes.cend())
    {
      n = node.children_.erase(n);
    }
    else
    {
      PruneNode(*n, current_nodes);
      ++n;
    }
  }
  current_nodes.pop_back();
}

void PruneTree()
{
  // Prune the tree of duplicates downstream. This is easier than trying to do it within the printing loop and recursion
  std::lock_guard<std::mutex> lock(Graph::mutex_);
  for (Node& node : Graph::trees_)
  {
    std::vector<Node> current_nodes;
    for (Node& child : node.children_)
    {
      PruneNode(child, current_nodes);
    }
  }
}

void PrintTree()
{
  std::lock_guard<std::mutex> lock(Graph::mutex_);
  int thread_num = 0;
  for (const Node& node : Graph::trees_)
  {
    std::cout << "Thread " << ++thread_num << '\n';
    for (const Node& child : node.children_)
    {
      PrintNode(child, std::string(), true);
    }
    std::cout << '\n';
  }
  std::cout << std::flush;
}

void GatherMutexNames(const Node& node, std::vector<std::pair<uint64_t, std::string>>& mutex_names)
{
  if (node.mutex_name_)
  {
    if (std::find_if(mutex_names.cbegin(), mutex_names.cend(), [&node](const std::pair<uint64_t, std::string>& n) { return (n.first == node.mutex_id_); }) == mutex_names.cend())
    {
      mutex_names.push_back(std::make_pair(node.mutex_id_, node.mutex_name_));
    }
  }
  for (const Node& child : node.children_)
  {
    GatherMutexNames(child, mutex_names);
  }
}

void PrintMutexes()
{
  // Gather mutex names
  std::vector<std::pair<uint64_t, std::string>> mutex_names;
  {
    std::lock_guard<std::mutex> lock(Graph::mutex_);
    for (const Node& node : Graph::trees_)
    {
      for (const Node& child : node.children_)
      {
        GatherMutexNames(child, mutex_names);
      }
    }
  }
  // Print
  std::cout << "Mutex Names\n";
  for (const std::pair<uint64_t, std::string>& mutex_name : mutex_names)
  {
    std::cout << mutex_name.first << " " << mutex_name.second << '\n';
  }
  std::cout << std::flush;
}

// Deadlock
class GraphNode
{
public:
  GraphNode(const uint64_t mutex_id)
   : mutex_id_(mutex_id)
  {
  }

  uint64_t mutex_id_;
  std::vector<GraphNode*> children_;
};

GraphNode* GetGraphNode(const uint64_t mutex_id, std::vector<std::unique_ptr<GraphNode>>& unique_nodes)
{
  std::vector<std::unique_ptr<GraphNode>>::const_iterator n = std::find_if(unique_nodes.cbegin(), unique_nodes.cend(), [mutex_id](const std::unique_ptr<GraphNode>& n) { return (n->mutex_id_ == mutex_id); });
  if (n == unique_nodes.cend())
  {
    unique_nodes.emplace_back(std::make_unique<GraphNode>(mutex_id));
    return unique_nodes.back().get();
  }
  else
  {
    return n->get();
  }
}

void BuildGraph(const Node& node, std::vector<std::unique_ptr<GraphNode>>& unique_nodes)
{
  // Insert unique nodes
  GraphNode* graph_node = GetGraphNode(node.mutex_id_, unique_nodes);
  for (const Node& child : node.children_)
  {
    // Add children
    std::vector<GraphNode*>::const_iterator n = std::find_if(graph_node->children_.cbegin(), graph_node->children_.cend(), [&child](const GraphNode* n) { return (n->mutex_id_ == child.mutex_id_); });
    if (n == graph_node->children_.cend())
    {
      graph_node->children_.push_back(GetGraphNode(child.mutex_id_, unique_nodes));
    }
    // Recurse
    BuildGraph(child, unique_nodes);
  }
}

void FindDeadlocks(const GraphNode* node, std::vector<uint64_t> current_nodes, std::vector<std::vector<uint64_t>>& potential_deadlocks)
{
  current_nodes.push_back(node->mutex_id_);
  for (const GraphNode* child : node->children_)
  {
    std::vector<uint64_t>::const_iterator end = current_nodes.cend() - 1;
    if (std::find(current_nodes.cbegin(), end, child->mutex_id_) == end)
    {
      FindDeadlocks(child, current_nodes, potential_deadlocks);
    }
    else
    {
      potential_deadlocks.push_back(current_nodes);
    }
  }
  current_nodes.pop_back();
}

void PrintPotentialDeadlocks()
{
  std::lock_guard<std::mutex> lock(Graph::mutex_);
  // Merge all the data into a single graph
  std::vector<std::unique_ptr<GraphNode>> unique_nodes;
  for (const Node& node : Graph::trees_)
  {
    for (const Node& child : node.children_)
    {
      BuildGraph(child, unique_nodes);
    }
  }
  // Search for potential deadlocks by looking for loops
  std::vector<std::vector<uint64_t>> potential_deadlocks;
  for (const std::unique_ptr<GraphNode>& node : unique_nodes)
  {
    FindDeadlocks(node.get(), std::vector<uint64_t>(), potential_deadlocks);
  }
  // Print potential deadlock sequences
  std::cout << "Potential Deadlocks\n";
  for (const std::vector<uint64_t> potential_deadlock : potential_deadlocks)
  {
    for (const uint64_t mutex_id : potential_deadlock)
    {
      std::cout << mutex_id << " ";
    }
    std::cout << '\n';
  }
  std::cout << std::endl;
}

}
