#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , map_{}
    , index_unass(0)
    , eof_mark{false}
    , end_index{0} {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    if (eof) {
        eof_mark = true;
        end_index = index + data.size();
    }
    
    if (index > index_unass) {
        insert_map(data, index);
    } else if (index <= index_unass && index + data.size() >= index_unass) {
        // 合并，写入到output stream中
        insert_map(data, index);
        auto it = map_.begin();
        for (; it != map_.end(); it++) {
            if (it->first <= index_unass) {
                if (it->first + it->second.size() <= index_unass) {
                    continue;
                }
                uint64_t writable = it->first + it->second.size() - index_unass;
                index_unass += _output.write(it->second.substr(index_unass - it->first, writable));
            } else {
                break;  // 超过了合并的界限
            }
        }
        map_.erase(map_.begin(), it);
        if (eof_mark && index_unass >= end_index) {
            _output.end_input();
        }
    }
}

void StreamReassembler::insert_map(const string &data, const uint64_t index) {
    if (map_[index].size() < data.size()) {
        map_[index] = data;
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    uint64_t total = 0;
    uint64_t left = 0 , right = 0;
    for(auto i = map_.begin() ; i != map_.end() ;i++){
        if(i == map_.begin()){
            left = i->first;
            right = i->second.size() + left;
            total += right - left;
        }else{
            left = std::max(right , i->first);
            right = std::max(right , i->first + i->second.size());
            total += right - left;
        }
    }
    return total;
}

bool StreamReassembler::empty() const { return map_.empty(); }
