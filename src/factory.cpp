// src/factory.cpp -- owner: Architect.
#include "unionfind.hpp"
#include "structA/rank_compression_uf.hpp"
#include "structB/naive_uf.hpp"
#include "structB/rank_uf.hpp"
#include "structB/compression_uf.hpp"

#include <cstring>
#include <memory>

namespace perc {

std::unique_ptr<UnionFind> makeUnionFind(Variant v, int n) {
    switch (v) {
        case Variant::Naive:
            return std::make_unique<NaiveUF>(n);
        case Variant::Rank:
            return std::make_unique<RankUF>(n);
        case Variant::Compression:
            return std::make_unique<CompressionUF>(n);
        case Variant::RankCompression:
            return std::make_unique<RankCompressionUF>(n);
    }
    return nullptr;
}

const char* variantName(Variant v) {
    switch (v) {
        case Variant::Naive:           return "naive";
        case Variant::Rank:            return "rank";
        case Variant::Compression:     return "compress";
        case Variant::RankCompression: return "rank_compress";
    }
    return "?";
}

bool parseVariant(const char* s, Variant& out) {
    for (Variant v : {Variant::Naive, Variant::Rank, Variant::Compression,
                      Variant::RankCompression}) {
        if (std::strcmp(s, variantName(v)) == 0) {
            out = v;
            return true;
        }
    }
    return false;
}

}  // namespace perc
