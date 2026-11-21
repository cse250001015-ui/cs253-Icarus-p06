#pragma once
// Brute-force reference partition for differential checks. TEST CODE ONLY.
#include <vector>
struct RefPartition {
    std::vector<int> label;
    explicit RefPartition(int n) : label(n) { for (int i = 0; i < n; ++i) label[i] = i; }
    bool unite(int a, int b) {
        int la = label[a], lb = label[b];
        if (la == lb) return false;
        for (int& l : label) if (l == la) l = lb;
        return true;
    }
    int sizeOf(int x) const {
        int s = 0;
        for (int l : label) if (l == label[x]) ++s;
        return s;
    }
};
