#pragma once
#include <s2/s2cell_union.h>
#include <s2/s2loop.h>

namespace JavaCovering {
    // S2RegionCoverer::GetCovering equivalent with 2011 Java priority queue
    // tie-breaking (left-child preference on equal priority).
    // Defaults match covering.py: max_cells=20, max_level=13, min_level=6, level_mod=1.
    S2CellUnion GetCovering(const S2Loop& loop,
                            int max_cells = 20,
                            int max_level = 13,
                            int min_level = 6,
                            int level_mod = 1);
}
