#include <vector>
#include <utility>

// Mirrors Java PriorityQueue sift rules (min-heap, left-child preference on equal priority).
// Drop-in replacement for std::priority_queue<std::pair<int,Candidate*>,
//   std::vector<...>, std::greater<...>> with matching tie-breaking.
template <typename T>
class JHeap {
public:
    using Entry = std::pair<int, T>;

    bool empty() const { return q_.empty(); }
    size_t size() const { return q_.size(); }

    void push(Entry x) {
        q_.push_back(x);
        int k = static_cast<int>(q_.size()) - 1;
        while (k > 0) {
            int p = (k - 1) >> 1;
            if (x.first >= q_[p].first)   // stop when x >= parent (same as Java)
                break;
            q_[k] = q_[p];
            k = p;
        }
        q_[k] = x;
    }

    Entry pop() {
        Entry res = q_[0];
        Entry x = q_.back();
        q_.pop_back();
        int n = static_cast<int>(q_.size());
        if (n > 0) {
            int k = 0;
            while (k < (n >> 1)) {
                int child = 2 * k + 1;          // left child
                int r = child + 1;              // right child
                // prefer right only if left is STRICTLY greater (Java rule)
                if (r < n && q_[child].first > q_[r].first) {
                    child = r;
                }
                if (x.first <= q_[child].first) // stop when x <= smaller child
                    break;
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