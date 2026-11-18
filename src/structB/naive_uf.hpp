#pragma once
#include "unionfind.hpp"
#include <vector>

namespace perc {
class NaiveUF : public UnionFind {
public:
    explicit NaiveUF(int n);
    int  find(int x) override;
    bool unite(int a, int b) override;
    int  size(int x) override;
    const char* name() const override { return "naive"; }
    int parentOf(int x) const override;
    int rankOf(int x) const override;
    int sizeAtRoot(int r) const override;
private:
    std::vector<int> parent_, size_;
};
}
