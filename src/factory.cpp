// src/factory.cpp -- owner: Architect.
// TODO(Structure A owner): replace the RankCompression line with RankCompressionUF.
// TODO(Structure B owner): replace the Naive, Rank and Compression lines with your classes.
#include "unionfind.hpp"
#include "placeholder_uf.hpp"
#include <cstdio>
#include <cstring>

namespace perc {

std::unique_ptr<UnionFind> makeUnionFind(Variant v, int n) {
    static bool warned[4] = {false, false, false, false};
    int k = static_cast<int>(v);
    if (!warned[k]) {
        warned[k] = true;
        std::fprintf(stderr, "[factory] variant '%s' is not implemented yet: using PlaceholderUF\n",
                     variantName(v));
    }
    switch (v) {
        case Variant::Naive:           return std::make_unique<PlaceholderUF>(n);  // TODO NaiveUF
        case Variant::Rank:            return std::make_unique<PlaceholderUF>(n);  // TODO RankUF
        case Variant::Compression:     return std::make_unique<PlaceholderUF>(n);  // TODO CompressionUF
        case Variant::RankCompression: return std::make_unique<PlaceholderUF>(n);  // TODO RankCompressionUF
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
    for (Variant v : {Variant::Naive, Variant::Rank, Variant::Compression, Variant::RankCompression}) {
        if (std::strcmp(s, variantName(v)) == 0) { out = v; return true; }
    }
    return false;
}

}  // namespace perc
