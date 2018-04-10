# 2D KDTree

This is a simple 2D KD-Tree, with reasonable performance.
Each node has 3 children instead of the regular 2 children, so that objects
straddling both halves of the node don't get stuck "high up in the tree".

The tree needs to be initialized with the bounds of your dataset before you
start inserting objects.

## Example
```cpp
kd2::Tree tree;
tree.Initialize(minx, miny, maxx, maxy);
for (const auto& el) {
    tree.Insert(el.ID, el.x1, el.y1, el.x2, el.y2);
}

// retrieve only element IDs
std::vector<size_t> ids;
tree.Find({x1, y1, x2, y2}, ids);

// retrieve element IDs and their boxes
std::vector<kd2::Elem> elems;
tree.Find({x1, y1, x2, y2}, elems);

```

### Build

    #define KDTREE_2D_IMPLEMENTATION
    #include <kdtree2d.h>

### Test

    clang++ --std=c++11 -Wall -O2 test.cpp -o test && ./test

### Performance (i7-4770 @ 3.4 Ghz)
* Insert 1000000 elements: 165 ms
* Time per query, returning 25 objects: 235 ns
