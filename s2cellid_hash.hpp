#ifndef _SILICONTRIP_S2CELLIDHASH_
#define _SILICONTRIP_S2CELLIDHASH_

template<> struct std::hash<S2CellId> {
    std::size_t operator()(S2CellId const& s) const noexcept {
        std::size_t h1 = std::hash<uint64_t>{}(s.id());
        return h1;
    }
};

#endif
