#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint64_t temp = n + isn.raw_value();
    temp &= ((1ll << 32) - 1);
    uint32_t result_raw_value = static_cast<uint32_t>(temp);
    return WrappingInt32{result_raw_value};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // int32_t offset = n.raw_value() - wrap(checkpoint , isn).raw_value();
    // checkpoint ... n
    /**
     * checkpoint = n*2^32 + k
     * n1 = (n-1)*2^32 + abs
     * n2 = n*2^32 + abs
     * if n2 - checkpoint >= checkpoint - n1 , then choose n1 otherwise n2
     */
    // uint64_t ans = checkpoint + offset;
    // if(checkpoint >= (1ull <<32) && offset >= (1u <<31)){
    //     return ans - (1ull << 32);
    // }
    // return ans;
    
    WrappingInt32 c = wrap(checkpoint, isn);
    int64_t offset = n.raw_value() - c.raw_value();
    int64_t ans = checkpoint + offset;
    if(offset > (1ll << 31) && ans >= (1ll << 32)){
        ans = ans - (1ll << 32);
    }
    return ans;
}
