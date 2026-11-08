// include/unionfind.hpp -- owner: Architect. Contract: INTERFACE.md section 3 (v0.3).
#pragma once
#include <cstdint>
#include <memory>
using namespace std;

namespace perc {

    struct Counters {
        uint64_t finds  = 0;   // calls to find(), including those made inside unite()/size()/connected()
        uint64_t hops   = 0;   // parent-pointer steps walked from x to its root (first pass only)
        uint64_t unions = 0;   // successful merges (unite() returned true)
    };

    enum class Variant { Naive, Rank, Compression, RankCompression };

    class UnionFind {
    public:
        explicit UnionFind(int n) : n_(n) {}
        virtual ~UnionFind() = default;

        virtual int  find(int x)         = 0;   // root of x's set
        virtual bool unite(int a, int b) = 0;   // true iff two different sets were merged
        virtual int  size(int x)         = 0;   // number of elements in x's set
        virtual const char* name() const = 0;

        // Debug accessors (INTERFACE.md 3.3): const, no side effects, never counted, must not call find().
        virtual int parentOf(int x)   const = 0;   // parent[x]; a root returns itself
        virtual int rankOf(int x)     const = 0;   // stored rank of root x; -1 if the variant has no rank
        virtual int sizeAtRoot(int r) const = 0;   // size[r]; meaningful only when parentOf(r) == r

        bool connected(int a, int b) { return find(a) == find(b); }
        int  numElements() const     { return n_; }
        const Counters& counters() const { return cnt_; }
        void resetCounters()         { cnt_ = Counters{}; }

    protected:
        int n_;
        Counters cnt_;
    };

    // src/factory.cpp
    unique_ptr<UnionFind> makeUnionFind(Variant v, int n);
    const char* variantName(Variant v);               // "naive" "rank" "compress" "rank_compress"
    bool parseVariant(const char* s, Variant& out);

}  // namespace perc
