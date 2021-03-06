/*This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.*/
/* Michael Welsch (c) 2018 */

#include "metric_search.hpp" // back reference for header only use
#include <functional>
#include <algorithm>
#include <sstream>
#if defined BOOST_SERIALIZATION_NVP
#define SERIALIZATION_NVP2(name,variable) boost::serialization::make_nvp(name, variable)
#define SERIALIZATION_NVP(variable) BOOST_SERIALIZATION_NVP(variable)
#else
#define SERIALIZATION_NVP2(name, variable) (variable)
#define SERIALIZATION_NVP(variable) (variable)
#endif

#ifdef BOOST_SERIALIZATION_SPLIT_MEMBER
#define SERIALIZE_SPLIT_MEMBERS BOOST_SERIALIZATION_SPLIT_MEMBER
#else
#ifndef SERIALIZE_SPLIT_MEMBERS
#define SERIALIZE_SPLIT_MEMBERS()
#endif
#endif

namespace metric_search
{

/*
  __ \          _|                |  |         \  |        |         _)           
  |   |   _ \  |     _` |  |   |  |  __|      |\/ |   _ \  __|   __|  |   __|     
  |   |   __/  __|  (   |  |   |  |  |        |   |   __/  |    |     |  (        
 ____/  \___| _|   \__,_| \__,_| _| \__|     _|  _| \___| \__| _|    _| \___|     
                                                                                                                                                    
*/
template <typename Container>
struct L2_Metric_STL
{
    typedef typename Container::value_type result_type;
    static_assert(
        std::is_floating_point<result_type>::value, "T must be a float type");

    result_type operator()(const Container &a, const Container &b) const
    {
        result_type sum = 0;
        for (auto it1 = a.begin(), it2 = b.begin(); it1 != a.end() || it2 != b.end(); ++it1, ++it2)
        {
            sum += (*it1 - *it2) * (*it1 - *it2);
        }
        return std::sqrt(sum);
    }
};

/*
   \  |             |                   
    \ |   _ \    _` |   _ \             
  |\  |  (   |  (   |   __/             
 _| \_| \___/  \__,_| \___|             
                                                          
nodes of the tree                          
*/
template <class recType, class Metric>
class Node
{
  public:
  using Distance = typename Tree<recType,Metric>::Distance;

    friend Tree<recType, Metric>;
    Node(Distance base = Tree<recType, Metric>().base) : base(base) { }
    ~Node() {
        for(auto p : children) {
            delete p;
        }
    }
  //    typedef Tree<recType, Metric>::Node NodeType;
    //typedef std::shared_ptr<Tree<recType, Metric>::Node> Node_ptr;
  typedef Node<recType,Metric>* Node_ptr;
    //  typedef typename std::result_of<Metric(recType, recType)>::type Distance;

  Distance base;
  recType data;    // data record associated with the node
  Node_ptr parent = nullptr; // parent of current node
  std::vector<Node_ptr> children; // list of children
  int level = 0;                      // current level of the node
  Distance parent_dist = 0;           // upper bound of distance to any of descendants
  unsigned ID = 0;                    // unique ID of current node

    //    mutable std::shared_timed_mutex mut; // lock for current node

    Distance covdist(); // covering distance of subtree at current node
    Distance sepdist(); // separating distance between nodes at current level

    Distance dist(const recType &pp) const; // distance between this node and point pp
    Distance dist(Node_ptr n) const;        // distance between this node and node n

    Node_ptr setChild(const recType &p, int new_id = -1); // insert a new child of current node with point p
    Node_ptr setChild(Node_ptr p, int new_id = -1);       // // insert the subtree p as child of current node (erase or reordering)

    // setting iterators for children access in loops
    typename std::vector<Node_ptr>::const_iterator begin() const
    {
        return children.cbegin();
    }
    typename std::vector<Node_ptr>::const_iterator end() const
    {
        return children.cend();
    }
    typename std::vector<Node_ptr>::iterator begin()
    {
        return children.begin();
    }
    typename std::vector<Node_ptr>::iterator end()
    {
        return children.end();
    }

  template<typename Archive>
  void serialize(Archive & ar, const unsigned int) {
    ar & SERIALIZATION_NVP(base)
        & SERIALIZATION_NVP(level)
        & SERIALIZATION_NVP(parent_dist)
        & SERIALIZATION_NVP(ID)
        & SERIALIZATION_NVP(data);
  }
};

template<typename recType, typename Metric>
struct SerializedNode {
  using NodeType = Node<recType, Metric>;
  NodeType * node = nullptr;
  bool is_null = true;
  bool has_children = false;

  SerializedNode():node(nullptr), is_null(true), has_children(false) { }
  ~SerializedNode() {  }
  SerializedNode(NodeType* n):node(n),is_null((node == nullptr)? true : false), has_children(!is_null && node->children.empty() ? false : true) {}

  template<typename Archive>
  void save(Archive & ar, const unsigned int ) const {
    ar << SERIALIZATION_NVP(is_null);
    if(!is_null) {
      // ar << *node << has_children;
      ar  << SERIALIZATION_NVP2("node",*node)
          << SERIALIZATION_NVP(has_children);
    }
  }
  template<typename Archive>
  void load(Archive & ar, const unsigned int ) {
    ar >> SERIALIZATION_NVP(is_null);
    if(!is_null) {
      try {
        node = new NodeType();
        ar >> SERIALIZATION_NVP2("node",*node);

        ar >> SERIALIZATION_NVP(has_children);
      } catch(...) {
        delete node;
        node = nullptr;
        throw;
      }
    }
  }
  SERIALIZE_SPLIT_MEMBERS();
  //BOOST_SERIALIZATION_SPLIT_MEMBER()
  
};
/*
   \  |             |           _ _|                    |                                |           |   _)                   
    \ |   _ \    _` |   _ \       |   __ `__ \   __ \   |   _ \  __ `__ \    _ \  __ \   __|   _` |  __|  |   _ \   __ \      
  |\  |  (   |  (   |   __/       |   |   |   |  |   |  |   __/  |   |   |   __/  |   |  |    (   |  |    |  (   |  |   |     
 _| \_| \___/  \__,_| \___|     ___| _|  _|  _|  .__/  _| \___| _|  _|  _| \___| _|  _| \__| \__,_| \__| _| \___/  _|  _|     
                                                _|                                                                                                                                                                                                                                                       
*/

/*** covering distance of subtree at current node ***/
template <class recType, class Metric>
typename Node<recType, Metric>::Distance
Node<recType, Metric>::covdist()
{
    return std::pow(base, level);
}

/*** separating distance between nodes at current level ***/
template <class recType, class Metric>
typename Node<recType, Metric>::Distance
Node<recType, Metric>::sepdist()
{
    return 2 * std::pow(base, level - 1);
}

/*** distance between current node and point pp ***/
template <class recType, class Metric>
typename Node<recType, Metric>::Distance
Node<recType, Metric>::dist(const recType &pp) const
{
    return Metric()(data, pp);
}

/*** distance between current node and node n ***/
template <class recType, class Metric>
typename Node<recType, Metric>::Distance
Node<recType, Metric>::dist(Node_ptr n) const
{
    return Metric()(data, n->data);
}

/*** insert a new child of current node with point p ***/
template <class recType, class Metric>
Node<recType, Metric>*
Node<recType, Metric>::setChild(const recType &p, int new_id)
{
  Node_ptr temp(new Node<recType, Metric>());
    temp->data = p;
    temp->level = level - 1;
    temp->parent_dist = 0;
    temp->ID = new_id;
    temp->parent = this;
    children.push_back(temp);
    return temp;
}

/*** insert the subtree p as child of current node ***/
template <class recType, class Metric>
Node<recType, Metric>*
Node<recType, Metric>::setChild(Node_ptr p, int /*new_id*/)
{
    if (p->level != level - 1)
    {
        Node_ptr current = p;
        std::stack<Node_ptr> travel;
        current->level = level - 1;
        travel.push(current);

        while (travel.size() > 0)
        {
            current = travel.top();
            travel.pop();

            for (const auto &child : *current)
            {
                child->level = current->level - 1;
                travel.push(child);
            }
        }
    }

    p->parent  = this;
    children.push_back(p);
    return p;
}

/*
 __ __|                    _ _|                    |                                |           |   _)               
    |   __|  _ \   _ \       |   __ `__ \   __ \   |   _ \  __ `__ \    _ \  __ \   __|   _` |  __|  |   _ \   __ \  
    |  |     __/   __/       |   |   |   |  |   |  |   __/  |   |   |   __/  |   |  |    (   |  |    |  (   |  |   | 
   _| _|   \___| \___|     ___| _|  _|  _|  .__/  _| \___| _|  _|  _| \___| _|  _| \__| \__,_| \__| _| \___/  _|  _| 
                                           _|                                                                        
*/

/*
   __|                   |                   |                  
  (      _ \    \  (_-<   _|   _| |  |   _|   _|   _ \   _|     
 \___| \___/ _| _| ___/ \__| _|  \_,_| \__| \__| \___/ _|       
                                                                                                                                                                                                           
*/

/*** constructor: empty tree **/
template <class recType, class Metric>
Tree<recType, Metric>::Tree(int truncate /*=-1*/, Metric d)
    : metric_(d)
{
    root = NULL;
    min_scale = 1000;
    max_scale = 0;
    truncate_level = truncate;
    N = 0;
}

/*** constructor: with a signal data record **/
template <class recType, class Metric>
Tree<recType, Metric>::Tree(const recType &p, int truncateArg /*=-1*/, Metric d)
    : metric_(d)
{
    min_scale = 1000;
    max_scale = 0;
    truncate_level = truncateArg;
    N = 1;

    root = std::make_unique<NodeType>();
    root->data = p;
    root->level = 0;
    root->parent_dist = 0;
    root->ID = 0;
}

/*** constructor: with a vector data records **/
template <class recType, class Metric>
Tree<recType, Metric>::Tree(const std::vector<recType> &p, int truncateArg /*=-1*/, Metric d)
    : metric_(d)
{
    min_scale = 1000;
    max_scale = 0;
    truncate_level = truncateArg;
    N = 1;

    root = new NodeType; //std::make_unique<NodeType>();
    root->data = p[0];
    root->level = 0;
    root->parent_dist = 0;
    root->ID = 0;

    for (std::size_t i = 1; i < p.size(); ++i)
    {
        insert(p[i]);
    }
}

/*** default deconstructor **/
template <class recType, class Metric>
Tree<recType, Metric>::~Tree() {
    delete root;
}

/*
                |     __|  |    _)  |      |                 _ )        _ \ _)       |                           
 (_-<   _ \   _| _|  (       \   |  |   _` |   _| -_)    \   _ \  |  |  |  | | (_-<   _|   _` |    \    _|   -_) 
 ___/ \___/ _| \__| \___| _| _| _| _| \__,_| _| \___| _| _| ___/ \_, | ___/ _| ___/ \__| \__,_| _| _| \__| \___| 
                                                                 ___/                                            
*/
template <class recType, class Metric>
template <typename pointOrNodeType>
std::tuple<std::vector<int>, std::vector<typename Tree<recType, Metric>::Distance>>
Tree<recType, Metric>::sortChildrenByDistance(Node_ptr p, pointOrNodeType x) const
{
    auto num_children = p->children.size();
    std::vector<int> idx(num_children);
    std::iota(std::begin(idx), std::end(idx), 0);
    std::vector<Distance> dists(num_children);
    for (unsigned i = 0; i < num_children; ++i)
    {
        dists[i] = p->children[i]->dist(x);
    }
    auto comp_x = [&dists](int a, int b) { return dists[a] < dists[b]; };
    std::sort(std::begin(idx), std::end(idx), comp_x);
    return std::make_tuple(idx, dists);
}

/*
 _ _|                      |   
   |     \  (_-<   -_)   _| _| 
 ___| _| _| ___/ \___| _| \__|                            
*/
template <class recType, class Metric>
std::size_t Tree<recType, Metric>::insert_if(const std::vector<recType> &p, Distance treshold){
    std::size_t inserted = 0;
    for (const auto & rec : p){
        if( root->dist(rec) > treshold) {
            insert(rec);
            inserted++;
        }
    }
    return inserted;
}
template <class recType, class Metric>
bool Tree<recType, Metric>::insert_if(const recType &p, Distance treshold){
    if( root->dist(p) > treshold) {
        insert(p);
        return true;
    }
    return false;
}

/*** vector of data record insertion  **/
template <class recType, class Metric>
bool Tree<recType, Metric>::insert(const std::vector<recType> &p){
    bool result;
    for (const auto & rec : p){
       result = insert(rec);
    }
    return result;
}

/*** data record insertion **/
template <class recType, class Metric>
bool Tree<recType, Metric>::insert(const recType &p)
{
    //    std::cout << "insert("  << p << ")"<< std::endl;
    bool result = false;
    // global_mut.lock_shared();

    // root insertion
    if (root == NULL)
    {
        // global_mut.unlock_shared(); // FIXME: this is not atomic
        // global_mut.lock();          //
        min_scale = 1000;
        max_scale = 0;
        truncate_level = -1;
        N = 1;

        root = new NodeType(); //std::make_unique<NodeType>();
        root->data = p;
        root->level = 0;
        root->parent_dist = 0;
        root->ID = 0;
        root->parent = NULL;

        // global_mut.unlock();      // FIXME: this is not atomic
        // global_mut.lock_shared(); //
    }

    // normal insertion
    else if (root->dist(p) > root->covdist())
    {
        // global_mut.unlock_shared(); // FIXME: this is not atomic
        // global_mut.lock();          //
        while (root->dist(p) > base * root->covdist() / (base - 1))
        {
            Node_ptr current = root;
            Node_ptr parent = NULL;
            while (current->children.size() > 0)
            {
                parent = current;
                current = current->children.back();
            }
            if (parent != NULL)
            {
                parent->children.pop_back();
                current->level = root->level + 1;
                current->children.push_back(root);
                root->parent = current;
                root->parent_dist = root->dist(current);
                root = current;
                root->parent = nullptr;
                root->parent_dist = 0;
            }
            else
            {
                root->level += 1;
            }
        }
        Node_ptr temp(new NodeType());
        temp->data = p;
        temp->level = root->level + 1;
        temp->parent = NULL;
        temp->children.push_back(root);
        temp->ID = N;
        root->parent_dist = root->dist(temp);
        N++;
        root->parent = temp;
        root = temp;
        max_scale = root->level;
        result = true;
        // global_mut.unlock();         // FIXME: this is not atomic
        // global_mut.lock_shared();    //
    }
    else
    {
        result = insert_(root, p);
    }
    // global_mut.unlock_shared();
    return result;
}

/*** recursive insertion part ***/
template <class recType, class Metric>
template <typename pointOrNodeType>
bool Tree<recType, Metric>::insert_(Node_ptr p, pointOrNodeType x, int new_id)
{
  //  std::cout << "insert_("  << x << ")"<< std::endl;
    bool result = false;
    bool flag = true;

    // p->mut.lock_shared();
    unsigned num_children = p->children.size(); // for later check, if there is a change during the next steps

    //auto[idx, dists] = sortChildrenByDistance(p, x); // can't wait C++17 ^^
    auto idx__dists = sortChildrenByDistance(p, x);
    auto idx = std::get<0>(idx__dists);
    auto dists = std::get<1>(idx__dists);
    for (const auto &q_idx : idx)
    {
        Node_ptr q = p->children[q_idx];

        Distance d = dists[q_idx];

        if (d <= q->covdist())
        {
            if (q->parent_dist < d)
            {
                q->parent_dist = d;
            }
            // p->mut.unlock_shared();
            result = insert_(q, x);
            flag = false;
            break;
        }
    }

    if (flag)
    {
        // p->mut.unlock_shared(); // FIXME: this is not atomic operation
        // p->mut.lock();          //

        // if insert is not valid try again
        if (num_children == p->children.size())
        {
            if (new_id == -2)
            {
                p->setChild(x, N); // this line behaves different by inserting a node or a record.
            }
            else
            {
                p->setChild(x, new_id);
            }
            N++;

            result = true;
            // p->mut.unlock();

            int local_min = min_scale.load();
            while (local_min > p->level - 1)
            {
                min_scale.compare_exchange_weak(local_min, p->level - 1, std::memory_order_relaxed, std::memory_order_relaxed);
                local_min = min_scale.load();
            }
        }
        else
        {
            // p->mut.unlock();
            result = insert_(p, x);
        }
    }
    return result;
}
// template <class recType, class Metric>
// auto Tree<recType, Metric>::merge(Node_ptr p, Node_ptr q)  -> Node_ptr {
//     Node_ptr p1 = p;
//     Node_ptr q1 = q;
//     if(q->level > p->level) {
//         p1 = q;
//         q1 = p;
//     }
//     while(q1->level < p1->level) {
//         Node_ptr leaf = findAnyLeaf(q1);
//         extractNode(leaf);
//         leaf->children.push_back(root);
//         leaf->level = root->level + 1;
//         root->parent = leaf;
//         root = leaf;
//     }
//     std::pair<Node_ptr,std::vector<Node_ptr>> lf = mergeHelper(p1,q1);
//     for(auto r : lf.second ) {
//         insert_(p1,r);
//     }
//     return p1;
// }

// template <class recType, class Metric>
// auto Tree<recType, Metric>::mergeHelper(Node_ptr p, Node_ptr q) -> std::pair<Node_ptr,std::vector<Node_ptr>> {
//     std::vector<Node_ptr> children1;
//     std::vector<Node_ptr> sepcov;
//     std::vector<Node_ptr> leftovers;
//     std::vector<Node_ptr> uncovered;
//     std::copy(p->children.begin(),p->children.end(), std::back_inserter(children1));
//     for(auto r : q->children) {
//         if(p->dist(r) < p->covdist()) {
//             bool foundMatch = false;
//             for(auto s : children1) {
//                 if(s->dist(r) <= p->sepdist()) {
//                     std::pair<Node_ptr,std::vector<Node_ptr>> lf = mergeHelper(s,r);
//                     children1.push_back(lf.first);
//                     for(auto t = children1.begin(); t != children1.end(); ++t) {
//                         if(*t == s) {
//                             children1.erase(t);
//                             break;
//                         }
//                     }
//                     leftovers.isert(leftovers.end(),lf.second.begin(),lf.second.end());
//                     foundMatch = true;
//                     break;
//                 }
//             }
//             if(!foundMatch) {
//                 sepcov.push_back(r);
//             }
//         } else {
//                 uncovered.push_back(r);
//         }
//     }
//     children1.insert(children1.end(),sepcov.begin(),sepcov.end());
//     p->children.assign(children1);
//     for(auto t : p->children) {
//         t->parent = p;
//     }
//     insert_(p,q);
//     std::vector<Node_ptr> leftovers1;
//     for(auto r : leftovers) {
//         if(r->dist(p) < p->covdist) {
//             insert_(p,r);
//         } else {
//             leftovers1.push_back(r);
//         }
//     }
//     leftovers1.insert(leftovers1.end(),uncovered.begin(),uncovered.end());
//     return std::make_pair(p,leftovers1);
// }

template <class recType, class Metric>
inline
auto Tree<recType, Metric>::findAnyLeaf() -> Node_ptr {
    Node_ptr current = root;
    while(!current->children.empty()) {
      current = current->children.back();
    }
    return current;
}

template <class recType, class Metric>
void Tree<recType, Metric>::extractNode(Node_ptr node) {
    Node_ptr parent = node->parent;
    if(parent == nullptr)
        return;
    unsigned num_children = parent->children.size();
    for (unsigned i = 0; i < num_children; ++i)
        {
            if (parent->children[i] == node)
                {
                    parent->children[i] = parent->children.back();
                    parent->children.pop_back();
                    break;
                }
        }
    node->parent = nullptr;
}

/*
   -_)   _| _` | (_-<   -_)
 \___| _| \__,_| ___/ \___|
*/
template <class recType, class Metric>
bool Tree<recType, Metric>::erase(const recType &p)
{
    bool ret_val = false;
    //find the best node to insert
    std::pair<Node_ptr, Distance> result(root, root->dist(p));
    nn_(root, result.second, p, result);

    if (result.second <= 0.0)
    {
        Node_ptr node_p = result.first;
        Node_ptr parent_p = node_p->parent;

        if (node_p == root)
        {
            if(node_p->children.empty()) {
                delete root;
                root = nullptr;
                N--;
                return true;
            }
            auto leaf = findAnyLeaf();
            extractNode(leaf);
            leaf->level = root->level;
            root = leaf;
            leaf->children.assign(node_p->children.begin(), node_p->children.end());
            for(auto l : leaf->children) {
                l->parent = leaf;
            }
            // std::cout << "erasing the root!" << std::endl;
            // // find the nearest neighbour node to switch with root node.
            // std::vector<std::pair<Node_ptr, Distance>> result2 = knn(node_p->data, 2);

            // Node_ptr node_s = result2[1].first; // nn to root
            // //            int level = node_s->level;
            // Node_ptr parent_s = node_s->parent;

            // std::vector<Node_ptr> temp_childs(node_s->children);

            // unsigned num_children = parent_s->children.size();
            // for (unsigned i = 0; i < num_children; ++i)
            // {
            //     if (parent_s->children[i] == node_s)
            //     {
            //         parent_s->children[i] = parent_s->children.back();
            //         parent_s->children.pop_back();
            //         break;
            //     }
            // }

            // root = node_s; // switch
            // // std::vector<std::pair<Node_ptr, Distance>> result3 = knn(node_p->data, 3);
            // // Node_ptr node_insert = result3[1].first;
            // // if (node_insert->parent == NULL)
            // // {
            // //     node_insert = result3[2].first;
            // // }

            // // //for each child q of p:
            // // for (Node_ptr q : *node_p)
            // // {
            // //     Tree<recType, Metric>::insert_(node_insert, q, q->ID);
            // // }

            ret_val = true;
            N--;
            node_p->children.clear();
            delete node_p;
        }

        else
        {
            //erase node from parent's list of child
            unsigned num_children = parent_p->children.size();
            for (unsigned i = 0; i < num_children; ++i)
            {
                if (parent_p->children[i] == node_p)
                {
                    parent_p->children[i] = parent_p->children.back();
                    parent_p->children.pop_back();
                    break;
                }
            }
            // insert each child of the node in new root again.
            for (Node_ptr q : node_p->children)
            {
                Tree<recType, Metric>::insert_(root, q, q->ID);
            }
            node_p->children.clear();
            delete node_p;
            N--;
            ret_val = true;
        }
    }
    return ret_val;
}

/* 
             
    \     \  
 _| _| _| _| 
             
Nearest Neighbour 
*/
template <class recType, class Metric>
typename Tree<recType, Metric>::Node_ptr
Tree<recType, Metric>::nn(const recType &p) const
{
    std::pair<Node_ptr, Distance> result(root, root->dist(p));
    nn_(root, result.second, p, result);
    return result.first;
}

template <class recType, class Metric>
void Tree<recType, Metric>::nn_(Node_ptr current, Distance dist_current, const recType &p, std::pair<Node_ptr, Distance> &nn) const
{

    if (dist_current < nn.second) // If the current node is the nearest neighbour
    {
        nn.first = current;
        nn.second = dist_current;
    }

    auto idx__dists = sortChildrenByDistance(current, p);
    auto idx = std::get<0>(idx__dists);
    auto dists = std::get<1>(idx__dists);
    // Distance max_dist = 0;
    // for(std::size_t i = 0; i < idx.size();i++) {
    //     if(p->children[i])
    // }
    for (const auto &child_idx : idx)
    {
        Node_ptr child = current->children[child_idx];
        Distance dist_child = dists[child_idx];

        if (nn.second > dist_child - child->parent_dist)
            nn_(child, dist_child, p, nn);
    }
}

/*
  |               
  | /    \     \  
 _\_\ _| _| _| _| 
                  
 k-Nearest Neighbours */
template <class recType, class Metric>
std::vector<std::pair<typename Tree<recType, Metric>::Node_ptr, typename Tree<recType, Metric>::Distance>>
Tree<recType, Metric>::knn(const recType &queryPt, unsigned numNbrs) const
{
    using NodePtr = typename Tree<recType, Metric>::Node_ptr;
    // Do the worst initialization
    std::pair<NodePtr, Distance> dummy(nullptr, std::numeric_limits<Distance>::max());
    // List of k-nearest points till now
    std::vector<std::pair<NodePtr, Distance>> nnList(numNbrs, dummy);

    // Call with root
    Distance dist_root = root->dist(queryPt);
    std::size_t nnSize = 0;
    nnSize = knn_(root, dist_root, queryPt, nnList, nnSize);
    if(nnSize < nnList.size()) {
        nnList.resize(nnSize);
    }
    return nnList;
}
template <class recType, class Metric>
std::size_t Tree<recType, Metric>::knn_(Node_ptr current, Distance dist_current, const recType &p, std::vector<std::pair<Node_ptr, Distance>> &nnList, std::size_t nnSize) const
{
    if (dist_current < nnList.back().second) // If the current node is eligible to get into the list
    {
        auto comp_x = [](std::pair<Node_ptr, Distance> a, std::pair<Node_ptr, Distance> b) { return a.second < b.second; };
        std::pair<Node_ptr, Distance> temp(current, dist_current);
        nnList.insert(
            std::upper_bound(nnList.begin(), nnList.end(), temp, comp_x),
            temp);
        nnList.pop_back();
        nnSize++;
    }

    auto idx__dists = sortChildrenByDistance(current, p);
    auto idx = std::get<0>(idx__dists);
    auto dists = std::get<1>(idx__dists);

    for (const auto &child_idx : idx)
    {
        Node_ptr child = current->children[child_idx];
        Distance dist_child = dists[child_idx];
        if (nnList.back().second > dist_child - child->parent_dist)
            nnSize = knn_(child, dist_child, p, nnList, nnSize);
    }
    return nnSize;
}

/*
                              
   _| _` |    \    _` |   -_) 
 _| \__,_| _| _| \__, | \___| 
                 ____/        
Range Neighbours Search 
*/
template <class recType, class Metric>
std::vector<std::pair<typename Tree<recType, Metric>::Node_ptr, typename Tree<recType, Metric>::Distance>>
Tree<recType, Metric>::rnn(const recType &queryPt, Distance distance) const
{

    std::vector<std::pair<Node_ptr, Distance>> nnList; // List of nearest neighbors in the rnn

    Distance dist_root = root->dist(queryPt);
    rnn_(root, dist_root, queryPt, distance, nnList); // Call with root

    return nnList;
}
template <class recType, class Metric>
void Tree<recType, Metric>::rnn_(Node_ptr current, Distance dist_current, const recType &p, Distance distance, std::vector<std::pair<Node_ptr, Distance>> &nnList) const
{

    if (dist_current < distance) // If the current node is eligible to get into the list
    {
        std::pair<Node_ptr, Distance> temp(current, dist_current);
        nnList.push_back(temp);
    }

    //auto[idx, dists] = sortChildrenByDistance(current, p);
    auto idx__dists = sortChildrenByDistance(current, p);
    auto idx = std::get<0>(idx__dists);
    auto dists = std::get<1>(idx__dists);

    for (const auto &child_idx : idx)
    {
        Node_ptr child = current->children[child_idx];
        Distance dist_child = dists[child_idx];
        if (distance > dist_child - child->parent_dist)
            rnn_(child, dist_child, p, distance, nnList);
    }
}

/*
      _)            
 (_-<  | _  /   -_) 
 ___/ _| ___| \___| 
 tree size                   
*/
template <class recType, class Metric>
size_t Tree<recType, Metric>::size()
{
    return size_t(N);
}

/*
  |        \ \   /           |              
   _|   _ \ \ \ /  -_)   _|   _|   _ \   _| 
 \__| \___/  \_/ \___| \__| \__| \___/ _|   
export to vector
*/
template <class recType, class Metric>
std::vector<recType> Tree<recType, Metric>::toVector()
{

    std::vector<std::pair<recType, int>> zipped;

    std::stack<Node_ptr> stack;
    Node_ptr current;
    stack.push(root);
    while (stack.size() > 0)
    {
        current = stack.top();
        stack.pop();
        zipped.push_back(std::make_pair(current->data, current->ID)); // Add to dataset
        for (const auto &child : *current)
            stack.push(child);
    }

    // Sort the vector by index
    std::sort(std::begin(zipped), std::end(zipped),
              [&](const auto &a, const auto &b) {
                  return a.second < b.second;
              });

    std::vector<recType> data(zipped.size());
    for (int i = 0; i < zipped.size(); ++i)
    {
        data[i] = zipped[i].first;
    }
    return data;
}

template <class recType, class Metric>
recType Tree<recType, Metric>::operator[](size_t id)
{
    // iterate through tree with stack
    std::stack<Node_ptr> stack;
    Node_ptr current;
    stack.push(root);
    while (stack.size() > 0)
    {
        current = stack.top();
        stack.pop();
        if (current->ID == id)
            break;
        for (const auto &child : *current)
            stack.push(child);
    }
    return current->data;
}

/* 
  _ \               __ __|            |        
  |  |  -_) \ \ /      |   _ \   _ \  | (_-<   
 ___/ \___|  \_/      _| \___/ \___/ _| ___/   
debugging functions
*/

//get root level == max_level
template <class recType, class Metric>
int Tree<recType, Metric>::levelSize()
{
    return root->level;
}
template <class recType, class Metric>
std::map<int, unsigned> Tree<recType, Metric>::print_levels()
{
    std::map<int, unsigned> level_count;
    std::stack<Node_ptr> stack;
    Node_ptr curNode;
    stack.push(root);
    while (stack.size() > 0)
    {
        curNode = stack.top();
        stack.pop();
        std::cout << "level: " << curNode->level << ",  node ID: " << curNode->ID << ",  child ids: ";
        for (std::size_t i = 0; i < curNode->children.size(); ++i)
        {
            std::cout << curNode->children[i]->ID << ", ";
        }
        std::cout << std::endl;

        level_count[curNode->level]++;

        // Now push the children
        for (const auto &child : *curNode)
            stack.push(child);
    }
    return level_count;
}

template <class recType, class Metric>
bool Tree<recType, Metric>::check_covering() const
{
    bool result = true;
    std::stack<Node_ptr> stack;
    Node_ptr curNode;
    if(root == nullptr)
        return true;
    stack.push(root);
    while (stack.size() > 0)
    {
        curNode = stack.top();
        stack.pop();

        // Check covering for the current -> children pair
        for (const auto &child : *curNode)
        {
            stack.push(child);
            if (curNode->dist(child) > curNode->covdist())
            {
                std::cout << "covering ill here (" << curNode->ID << ") --> (" << child->ID << ") dist < covdist " << curNode->dist(child) << " < " << curNode->covdist() << " level:" << curNode->level << std::endl;
                result = false;
            }
        }
    }

    return result;
}


std::string depth2;

char depth[2056];
int di = 0;

template <class recType, class Metric>
void Tree<recType, Metric>::print() const
{
    if(root != nullptr)
      print_(root, std::cout);
    else {
        std::cout << "Empty tree" << std::endl;
    }
}
template <class recType, class Metric>
void Tree<recType, Metric>::print(std::ostream &ostr) const
{
  if(root != nullptr)
    print_(root, ostr);
  else {
    ostr << "Empty tree" << std::endl;
  }
}

template <class recType, class Metric>
void Tree<recType, Metric>::print_(NodeType *node_p, std::ostream & ostr) const
{
    auto push = [&](char c) {
        depth[di++] = ' ';
        depth[di++] = c;
        depth[di++] = ' ';
        depth[di++] = ' ';
        depth[di] = 0;
    };

    auto pop = [&]() {
        depth[di -= 4] = 0;
    };
    //    recType pd;
    // if(node_p->parent) {
    //   //        auto  pd = node_p->parent->data;
    //   std::cout << "(" << node_p->ID  << ',' << node_p->parent_dist << ',' << node_p->level<< ',' <<  node_p->data << "," << node_p->parent->ID << ")" << std::endl;
    // } else {
    //   std::cout << "(" << node_p->ID  << ',' << node_p->parent_dist << ',' << node_p->level << ',' <<  node_p->data << "," << " no_parent )" << std::endl;
    // }
    ostr << "(" << node_p->ID  << ")" << std::endl;
    if(node_p->children.empty())
        return;

    for (std::size_t i = 0; i < node_p->children.size(); ++i)
    {
      ostr << depth;
      if (i < node_p->children.size() - 1)
      {
        ostr << " ├──";
        push('|');
      }
        else
        {
            ostr << " └──";
            push(' ');
        }
      print_(node_p->children[i], ostr);
        pop();
    }
}


/*** traverse the tree from root and do something with every node ***/
template <class recType, class Metric>
void Tree<recType, Metric>::traverse(const std::function<void(Node_ptr)> &f)
{
    //iterate though the tree...
    Node_ptr curNode = root;
    std::stack<Node_ptr> nodeStack;

    nodeStack.push(curNode);
    while (nodeStack.size() > 0)
    {
        curNode = nodeStack.top();
        nodeStack.pop();
        f(curNode); // .. and callback each node.
        for (const auto &child : *curNode)
        {
            nodeStack.push(child);
        }
    }
}

template <class recType, class Metric>
void Tree<recType, Metric>::traverse_child(const std::function<void(Node_ptr)> &f)
{
    //iterate though the tree...
    std::stack<Node_ptr> nodeStack;
    Node_ptr curNode = root;
    nodeStack.push(curNode);
    while (nodeStack.size() > 0)
    {
        curNode = nodeStack.top();
        nodeStack.pop();
        for (const auto &child : *curNode)
        {
            nodeStack.push(child); //.. and callback all child nodes.
            f(child);
        }
    }
    return;
}


// template<typename recType, class Metric>
// template <class Serializer>
// inline
// void Tree<recType,Metric>::serialize_node(Node_ptr node, std::ostringstream & ostr, Serializer & serializer) {
//   if(!node->children.empty()) {
//     ostr << node->ID << " " << node->level << " " << node->parent_dist
//          << " t" << serializer(node->data) << " ";
//   } else {
//     ostr << node->ID << " " << node->level << " " << node->parent_dist
//          << " f" << serializer(node->data) <<" ";
//   }
// }
// template <class recType, class Metric>
// template <class Serializer>
// inline
// void Tree<recType,Metric>::serialize_aux(Node_ptr node, std::ostringstream & ostr, Serializer & serializer) {
//   std::stack<Node_ptr> nodeStack;
//   Node_ptr curNode = node;
//   nodeStack.push(curNode);
//   while(!nodeStack.empty()) {
//     curNode = nodeStack.top();
//     nodeStack.pop();
//     serialize_node(curNode, ostr, serializer);
//     for(auto & c : curNode->children) {
//       nodeStack.push(c);
//     }
//     ostr << ")";
//   }
// }
template <class recType, class Metric>
template <class Archive>
inline
void Tree<recType,Metric>::serialize_aux(Node_ptr node, Archive & archive) {
  //  std::stack<Node_ptr> nodeStack;
  //  Node_ptr curNode = node;
  //  nodeStack.push(curNode);
  //  while(!nodeStack.empty()) {
  //    curNode = nodeStack.top();
  //    nodeStack.pop();
  SerializedNode<recType,Metric> sn(node);
  if(node->children.size() > 0 ) {
    //sn.save(archive, 0);
    archive << SERIALIZATION_NVP2("node",sn);
    for(auto & c : node->children) {
      serialize_aux(c, archive);
    }
    SerializedNode<recType, Metric> snn(nullptr);
    //snn.save(archive, 0);
    archive << SERIALIZATION_NVP2("node",snn);
  } else {
    //    sn.save(archive, 0);
    archive << SERIALIZATION_NVP2("node",sn);
    //archive & SerializedNode<recType, Metric>(node);
  }
}

template <class recType, class Metric>
template <class Archive>
inline
void Tree<recType,Metric>::serialize(Archive & archive) {
  //  std::ostringstream ostr;
  serialize_aux(root,archive);
  //  return ostr.str();
}
template <class recType, class Metric>
template <class Archive>
inline
auto Tree<recType,Metric>::deserialize_node(Archive & istr) -> SerializedNode<recType, Metric> {
  SerializedNode<recType, Metric> n;
  istr & n;
  return n;
  // if(istr.peek() == ')') {
  //   istr.ignore();
  //   return std::make_tuple(nullptr,false);
  // }
  // unsigned ID;
  // int level;
  // Distance parent_distance;
  // char childrens;
  // istr >> ID >> level >> parent_distance >> childrens;
  // Node_ptr node = new Node();
  // node->ID = ID;
  // node->level = level;
  // node->parent_dist = parent_distance;
  // node->data = deserializer(istr);
  // bool has_children = (childrens == 't');
  // return std::make_tuple(node,has_children);
}

template <class recType, class Metric>
template <class Archive, class Stream>
inline
void Tree<recType,Metric>::deserialize(Archive & input, Stream & stream) {
  SerializedNode<recType, Metric> node;
  //  input & root_node;
  //root_node.load(input, 0);
  try {
    input >> SERIALIZATION_NVP2("node", node);
    std::stack<Node_ptr> parentstack;
    parentstack.push(node.node);
    while(!stream.eof()) {
      //auto node = deserialize_node(input);
      SerializedNode<recType, Metric> node;
      //node.load(input, 0);
      input >> SERIALIZATION_NVP2("node", node);
      //input & snode;
      if(!node.is_null) {
        parentstack.top()->children.push_back(node.node);
        node.node->parent = parentstack.top();
        if(node.has_children) {
          parentstack.push(node.node);
        }
      } else {
        parentstack.pop();
      }
    }
  } catch(...) { /* hack to catch end of stream */}
  root = node.node;
}
template <class recType, class Metric>
inline
bool Tree<recType,Metric>::same_tree(const Node_ptr lhs, const Node_ptr rhs) const {
  if(lhs == rhs) {
    return true;
  }
  if(lhs->ID != rhs->ID || lhs->level != rhs->level
     || lhs->parent_dist != rhs->parent_dist || lhs->data != rhs->data ) {
    return false;
  }

  if(lhs->children.size() != rhs->children.size())
    return false;
  for(std::size_t i = 0; i < lhs->children.size(); i++) {
    if(!same_tree(lhs->children[i], rhs->children[i]))
      return false;
  }
  return true;
}
} // end namespace
