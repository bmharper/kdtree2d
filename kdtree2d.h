#pragma once

/*
A quick and (not so dirty) 2D KD Tree.

kd2::Tree tree;
tree.Initialize(world_bounds.x1, world_bounds.y1, world_bounds.x2, world_bounds.y2);
tree.Insert(id, x1, y1, x2, y2);
auto els = tree.Find(x1, y1, x2, y2);

Internals
---------
Even numbered nodes (including the root node) are split into top and bottom halves, and a third child that spans the center.
Odd numbered nodes are split into left, right, and center children.

When splitting, we always pass 'depth', so that we know on which dimension to split.

Each node has three children:
The first two children split the node in half. The third child is the same size
as it's two siblings, but it straddles them, so that it accepts any objects which
would otherwise get stuck on the "cracks".

	+----+----+----+----+
	|  Node A |  Node B |
	|    |    |    |    |
	|    | Node C  |    |
	|    |    |    |    |
	|    |    |    |    |
	+----+----+----+----+


*/

#include <assert.h>
#include <vector>
#include <utility>

namespace kd2 {

struct Box {
	float X1, Y1, X2, Y2;

	bool Overlaps(Box b) const {
		return b.X2 >= X1 && b.X1 <= X2 &&
		       b.Y2 >= Y1 && b.Y1 <= Y2;
	}
	bool IsInsideMe(Box b) const {
		return b.X1 >= X1 && b.Y1 >= Y1 &&
		       b.X2 <= X2 && b.Y2 <= Y2;
	}
};

struct Elem {
	size_t   Id;
	kd2::Box Box;
};

class Tree {
public:
	size_t              NodeSize = 64;
	size_t              NScanned = 0;
	void                Initialize(float x1, float y1, float x2, float y2);
	void                Insert(size_t id, float x1, float y1, float x2, float y2);
	void                Insert(size_t id, Box box);
	void                Find(Box box, std::vector<Elem>& els);   // Find all objects that overlap 'box', and return ID and box.
	void                Find(Box box, std::vector<size_t>& ids); // Find all objects that overlap 'box', and return only ID.
	std::vector<size_t> Find(Box box);                           // Find all objects that overlap 'box', and return only ID.
	bool                AnyOverlap(Box box);                     // Return true if any objects overlap 'box'

private:
	struct Node {
		Node*             ChildA = nullptr;
		Node*             ChildB = nullptr;
		Node*             ChildC = nullptr;
		std::vector<Elem> Elems;
		Node() {}
		Node(const Node& b);
		Node(Node&& b);
		~Node();
		Node& operator=(const Node& b);
		Node& operator=(Node&& b);
	};
	Node Root;
	Box  RootBox;

	void Split(Node* n, Box nbox, size_t depth);

	template <typename Visitor>
	void VisitorFind(Node* n, Box nbox, size_t depth, Box findBox, Visitor vis);

	static void Subdivide(size_t depth, Box b, Box& ca, Box& cb, Box& cc);
};

#ifdef KDTREE_2D_IMPLEMENTATION

inline Tree::Node::Node(const Node& b) {
	*this = b;
}

inline Tree::Node::Node(Node&& b) {
	*this = std::move(b);
}

inline Tree::Node::~Node() {
	delete ChildA;
	delete ChildB;
	delete ChildC;
}

inline Tree::Node& Tree::Node::operator=(const Tree::Node& b) {
	delete ChildA;
	delete ChildB;
	delete ChildC;
	if (b.ChildA) {
		ChildA = new Node(*b.ChildA);
		ChildB = new Node(*b.ChildB);
		ChildC = new Node(*b.ChildC);
	} else {
		ChildA = nullptr;
		ChildB = nullptr;
		ChildC = nullptr;
	}
	Elems = b.Elems;
	return *this;
}

inline Tree::Node& Tree::Node::operator=(Tree::Node&& b) {
	delete ChildA;
	delete ChildB;
	delete ChildC;
	ChildA   = b.ChildA;
	ChildB   = b.ChildB;
	ChildC   = b.ChildC;
	b.ChildA = nullptr;
	b.ChildB = nullptr;
	b.ChildC = nullptr;
	Elems    = std::move(b.Elems);
	return *this;
}

inline void Tree::Initialize(float x1, float y1, float x2, float y2) {
	assert(!Root.ChildA && Root.Elems.size() == 0);
	RootBox = {x1, y1, x2, y2};
}

inline void Tree::Insert(size_t id, float x1, float y1, float x2, float y2) {
	Insert(id, {x1, y1, x2, y2});
}

inline void Tree::Insert(size_t id, Box box) {
	Node*  n     = &Root;
	Box    nbox  = RootBox;
	size_t depth = 0;
	for (;; depth++) {
		if (!n->ChildA) {
			// leaf node
			break;
		}
		Box boxA, boxB, boxC;
		Subdivide(depth, nbox, boxA, boxB, boxC);
		bool qa = boxA.IsInsideMe(box);
		bool qb = boxB.IsInsideMe(box);
		bool qc = boxC.IsInsideMe(box);
		if (!(qa || qb || qc)) {
			// not strictly inside any child, so we cannot subdivide further
			break;
		}
		if (qa) {
			n    = n->ChildA;
			nbox = boxA;
		} else if (qb) {
			n    = n->ChildB;
			nbox = boxB;
		} else {
			n    = n->ChildC;
			nbox = boxC;
		}
	}
	n->Elems.push_back({id, box});
	if (!n->ChildA && n->Elems.size() >= NodeSize)
		Split(n, nbox, depth);
}

template <typename Visitor>
void Tree::VisitorFind(Node* n, Box nbox, size_t depth, Box findBox, Visitor vis) {
	struct QueueItem {
		Node*  Node;
		Box    Box;
		size_t Depth;
	};
	std::vector<QueueItem> queue;
	queue.push_back({n, nbox, depth});

	while (queue.size() != 0) {
		auto q = queue.back();
		queue.pop_back();

		NScanned += q.Node->Elems.size();
		for (const auto& el : q.Node->Elems) {
			if (el.Box.Overlaps(findBox)) {
				if (!vis(el))
					return;
			}
		}
		if (!q.Node->ChildA)
			continue;
		Box boxA, boxB, boxC;
		Subdivide(q.Depth, q.Box, boxA, boxB, boxC);
		if (boxA.Overlaps(findBox))
			queue.push_back({q.Node->ChildA, boxA, q.Depth + 1});
		if (boxB.Overlaps(findBox))
			queue.push_back({q.Node->ChildB, boxB, q.Depth + 1});
		if (boxC.Overlaps(findBox))
			queue.push_back({q.Node->ChildC, boxC, q.Depth + 1});
	}
}

inline void Tree::Find(Box box, std::vector<Elem>& els) {
	struct visitor {
		std::vector<Elem>* Out;

		bool operator()(Elem el) {
			Out->push_back(el);
			return true;
		}
	};

	visitor v = {&els};
	VisitorFind(&Root, RootBox, 0, box, v);
}

inline void Tree::Find(Box box, std::vector<size_t>& ids) {
	struct visitor {
		std::vector<size_t>* Out;

		bool operator()(const Elem& el) {
			Out->push_back(el.Id);
			return true;
		}
	};

	visitor v = {&ids};
	VisitorFind(&Root, RootBox, 0, box, v);
}

inline std::vector<size_t> Tree::Find(Box box) {
	std::vector<size_t> r;
	Find(box, r);
	return r;
}

inline bool Tree::AnyOverlap(Box box) {
	struct visitor {
		bool Any = false;

		bool operator()(const Elem& el) {
			Any = true;
			return false;
		}
	};

	visitor v;
	VisitorFind(&Root, RootBox, 0, box, v);
	return v.Any;
}

inline void Tree::Split(Node* n, Box nbox, size_t depth) {
	Box boxA, boxB, boxC;
	Subdivide(depth, nbox, boxA, boxB, boxC);
	n->ChildA = new Node();
	n->ChildB = new Node();
	n->ChildC = new Node();
	std::vector<Elem> pruned;
	for (auto elem : n->Elems) {
		bool qa = boxA.Overlaps(elem.Box);
		bool qb = boxB.Overlaps(elem.Box);
		bool qc = boxC.Overlaps(elem.Box);
		if (qa && qb && qc)
			pruned.push_back(elem);
		else if (qa)
			n->ChildA->Elems.push_back(elem);
		else if (qb)
			n->ChildB->Elems.push_back(elem);
		else
			n->ChildC->Elems.push_back(elem);
	}
	n->Elems = std::move(pruned);
}

inline void Tree::Subdivide(size_t depth, Box b, Box& ca, Box& cb, Box& cc) {
	if ((depth & 1) == 1) {
		// odd splits left/right
		float m1 = 0.33f * (b.X1 + b.X2);
		float m2 = 0.50f * (b.X1 + b.X2);
		float m3 = 0.66f * (b.X1 + b.X2);
		ca       = {b.X1, b.Y1, m2, b.Y2};
		cb       = {m2, b.Y1, b.X2, b.Y2};
		cc       = {m1, b.Y1, m3, b.Y2};
	} else {
		// even splits top/bottom
		float m1 = 0.33f * (b.Y1 + b.Y2);
		float m2 = 0.50f * (b.Y1 + b.Y2);
		float m3 = 0.66f * (b.Y1 + b.Y2);
		ca       = {b.X1, b.Y1, b.X2, m2};
		cb       = {b.X1, m2, b.X2, b.Y2};
		cc       = {b.X1, m1, b.X2, m3};
	}
}

#endif // KDTREE_2D_IMPLEMENTATION

} // namespace kd2
