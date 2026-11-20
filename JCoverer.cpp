#include "JCoverer.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include <s2/s2cap.h>
#include <s2/s2cell.h>
#include <s2/s2cell_id.h>

namespace {

// Java PriorityQueue sift rules with the 2011 S2 QueueEntriesComparator, which
// is reversed (x < y ? 1 : -1): the LARGEST priority is polled first. Since
// priorities are negative, coarse cells with few children come out first.
// On ties the left child is preferred in sift-down (right only if strictly larger).
template <typename T>
class JHeap {
public:
    struct Entry { int priority; T value; };

    bool   empty() const { return q_.empty(); }
    size_t size()  const { return q_.size(); }

    void push(int priority, T value) {
        q_.push_back({priority, value});
        int k = static_cast<int>(q_.size()) - 1;
        while (k > 0) {
            int p = (k - 1) >> 1;
            if (priority <= q_[p].priority) break;
            q_[k] = q_[p];
            k = p;
        }
        q_[k] = {priority, value};
    }

    Entry pop() {
        Entry res = q_[0];
        Entry x   = q_.back();
        q_.pop_back();
        int n = static_cast<int>(q_.size());
        if (n > 0) {
            int k = 0;
            while (k < (n >> 1)) {
                int child = 2 * k + 1;
                int r     = child + 1;
                if (r < n && q_[child].priority < q_[r].priority)
                    child = r;
                if (x.priority >= q_[child].priority) break;
                q_[k] = q_[child];
                k = child;
            }
            q_[k] = x;
        }
        return res;
    }

private:
    std::vector<Entry> q_;
};

static const double kMinWidth = 2.0 * std::sqrt(2.0) / 3.0;

static int MaxLevelFor(double value) {
    if (value <= 0) return 30;
    int exp;
    std::frexp(kMinWidth / value, &exp);
    return std::max(0, std::min(30, exp - 1));
}

struct Candidate {
    S2CellId cid;
    S2Cell   cell;
    bool     terminal    = false;
    int      num_children = 0;
    int      nterm        = 0;
    std::vector<std::unique_ptr<Candidate>> children;
};

} // anonymous namespace

namespace JavaCovering {

S2CellUnion GetCovering(const S2Loop& loop,
                        int max_cells, int max_level, int min_level, int level_mod)
{
    S2Cap cap = loop.GetCapBound();
    int mcs = 2 * level_mod;

    std::vector<S2CellId> result;
    JHeap<Candidate*> pq;
    std::vector<std::unique_ptr<Candidate>> owned; // owns top-level seeds

    auto new_candidate = [&](S2CellId cid) -> std::unique_ptr<Candidate> {
        S2Cell cell(cid);
        if (!loop.MayIntersect(cell)) return nullptr;
        auto c = std::make_unique<Candidate>();
        c->cid  = cid;
        c->cell = cell;
        int lvl = cid.level();
        if (lvl >= min_level) {
            if (lvl + level_mod > max_level || loop.Contains(cell))
                c->terminal = true;
        }
        return c;
    };

    std::function<int(Candidate*)> add;

    auto rec = [&](auto& self, S2CellId cid, int lv, Candidate* parent) -> int {
        if (lv == 0) {
            auto ch = new_candidate(cid);
            if (!ch) return 0;
            int t = ch->terminal ? 1 : 0;
            parent->children.push_back(std::move(ch));
            return t;
        }
        int n = 0;
        for (int k = 0; k < 4; ++k)
            n += self(self, cid.child(k), lv - 1, parent);
        return n;
    };

    add = [&](Candidate* cand) -> int {
        if (!cand) return 0;
        if (cand->terminal) {
            result.push_back(cand->cid);
            return 1;
        }
        int num_levels = (cand->cid.level() < min_level) ? 1 : level_mod;
        int nterm = rec(rec, cand->cid, num_levels, cand);
        cand->num_children = static_cast<int>(cand->children.size());
        cand->nterm = nterm;
        if (cand->num_children == 0) return 0;
        if (nterm == (1 << mcs) && cand->cid.level() >= min_level) {
            cand->terminal = true;
            return add(cand);
        }
        int priority = -((((cand->cid.level() << mcs) + cand->num_children) << mcs) + nterm);
        pq.push(priority, cand);
        return 0;
    };

    // Keep ownership of seeded candidates; children are owned by their parents.
    auto seed = [&](S2CellId cid) {
        auto c = new_candidate(cid);
        if (c) {
            Candidate* raw = c.get();
            owned.push_back(std::move(c));
            add(raw);
        }
    };

    bool started = false;
    if (max_cells >= 4) {
        double ang = std::acos(std::max(-1.0, std::min(1.0, 1.0 - cap.height())));
        int level = std::min(MaxLevelFor(2.0 * ang), std::min(max_level, 29));
        if (level_mod > 1 && level > min_level)
            level -= (level - min_level) % level_mod;
        if (level > 0) {
            S2CellId leaf(cap.center());
            std::vector<S2CellId> neighbors;
            leaf.AppendVertexNeighbors(level, &neighbors);
            for (const auto& cid : neighbors)
                seed(cid);
            started = true;
        }
    }
    if (!started) {
        for (int face = 0; face < 6; ++face)
            seed(S2CellId::FromFacePosLevel(face, 0, 0));
    }

    while (!pq.empty()) {
        auto [priority, cand] = pq.pop();
        int n = cand->num_children;
        bool expand = (cand->cid.level() < min_level
                    || n == 1
                    || static_cast<int>(result.size() + pq.size() + n) <= max_cells);
        if (expand) {
            for (auto& ch : cand->children)
                add(ch.get());
        } else {
            cand->terminal = true;
            add(cand);
        }
    }

    // Normalize: deduplicate, absorb descendants, merge sibling quads.
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    std::vector<S2CellId> out;
    for (S2CellId cid : result) {
        if (!out.empty() && out.back().contains(cid)) continue;
        while (!out.empty() && cid.contains(out.back())) out.pop_back();
        while (out.size() >= 3 && cid.level() > min_level) {
            S2CellId p = cid.parent();
            if (out[out.size()-1].level() == cid.level() &&
                out[out.size()-2].level() == cid.level() &&
                out[out.size()-3].level() == cid.level() &&
                out.back().parent()          == p &&
                out[out.size()-2].parent()   == p &&
                out[out.size()-3].parent()   == p) {
                out.erase(out.end()-3, out.end());
                cid = p;
            } else break;
        }
        out.push_back(cid);
    }

    // FromVerbatim: the S2CellUnion(vector) constructor would call Normalize(),
    // which merges sibling quads at every level and ignores min_level.
    return S2CellUnion::FromVerbatim(std::move(out));
}

} // namespace JavaCovering
