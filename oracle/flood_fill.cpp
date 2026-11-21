// oracle/flood_fill.cpp
#include "flood_fill.hpp"
#include <stdexcept>

namespace perc {

namespace {
// up, down, left, right. No wraparound: out-of-grid neighbours are skipped.
const int DR[4] = {-1, 1, 0, 0};
const int DC[4] = {0, 0, -1, 1};

int checkedL(int L) {
    if (L < 1 || L > 46340)
        throw std::invalid_argument("FloodFillOracle: need 1 <= L <= 46340");
    return L;
}
}  // namespace

FloodFillOracle::FloodFillOracle(int L)
    : L_(checkedL(L)), numOpen_(0),
      open_(static_cast<size_t>(L_) * L_, 0),
      full_(static_cast<size_t>(L_) * L_, 0),
      queue_(static_cast<size_t>(L_) * L_, 0),
      dirty_(false) {}

void FloodFillOracle::check(int r, int c) const {
    if (r < 0 || r >= L_ || c < 0 || c >= L_)
        throw std::out_of_range("FloodFillOracle: coordinates outside [0, L)");
}

bool FloodFillOracle::open(int r, int c) {
    check(r, c);
    const int idx = r * L_ + c;
    if (open_[idx]) return false;
    open_[idx] = 1;
    ++numOpen_;
    dirty_ = true;
    return true;
}

bool FloodFillOracle::isOpen(int r, int c) const {
    check(r, c);
    return open_[r * L_ + c] != 0;
}

// BFS from every open top-row site, moving only through open sites.
// Afterwards full_[i] == 1 exactly for the sites reachable from the top row.
void FloodFillOracle::refresh() {
    const int n = L_ * L_;
    for (int i = 0; i < n; ++i) full_[i] = 0;
    int head = 0, tail = 0;
    for (int c = 0; c < L_; ++c) {
        if (open_[c]) { full_[c] = 1; queue_[tail++] = c; }
    }
    while (head < tail) {
        const int v = queue_[head++];
        const int r = v / L_, c = v % L_;
        for (int d = 0; d < 4; ++d) {
            const int nr = r + DR[d], nc = c + DC[d];
            if (nr < 0 || nr >= L_ || nc < 0 || nc >= L_) continue;
            const int nb = nr * L_ + nc;
            if (open_[nb] && !full_[nb]) { full_[nb] = 1; queue_[tail++] = nb; }
        }
    }
    dirty_ = false;
}

bool FloodFillOracle::isFull(int r, int c) {
    check(r, c);
    if (dirty_) refresh();
    return full_[r * L_ + c] != 0;
}

bool FloodFillOracle::percolates() {
    if (dirty_) refresh();
    for (int c = 0; c < L_; ++c)
        if (full_[(L_ - 1) * L_ + c]) return true;
    return false;
}

int FloodFillOracle::numOpen() const { return numOpen_; }
int FloodFillOracle::L() const { return L_; }

// BFS component labelling over open sites. Sources are taken in increasing index, so
// the label of a component is its smallest site index.
int FloodFillOracle::labelComponents(std::vector<int>& label) {
    const int n = L_ * L_;
    label.assign(n, -1);
    int best = 0;
    for (int s = 0; s < n; ++s) {
        if (!open_[s] || label[s] != -1) continue;
        int head = 0, tail = 0;
        queue_[tail++] = s;
        label[s] = s;
        while (head < tail) {
            const int v = queue_[head++];
            const int r = v / L_, c = v % L_;
            for (int d = 0; d < 4; ++d) {
                const int nr = r + DR[d], nc = c + DC[d];
                if (nr < 0 || nr >= L_ || nc < 0 || nc >= L_) continue;
                const int nb = nr * L_ + nc;
                if (open_[nb] && label[nb] == -1) { label[nb] = s; queue_[tail++] = nb; }
            }
        }
        if (tail > best) best = tail;
    }
    return best;
}

int FloodFillOracle::largestCluster() {
    std::vector<int> label;
    return labelComponents(label);
}

std::vector<int> FloodFillOracle::componentLabels() {
    std::vector<int> label;
    labelComponents(label);
    return label;
}

}  // namespace perc
