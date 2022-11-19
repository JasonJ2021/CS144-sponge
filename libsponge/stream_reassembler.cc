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
    , _buffer(capacity , '\0')
    , _bitmap(capacity , false)
    {}
//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    if(index + data.size() <= index_unass){
        // invalid string
        if(eof){
            eof_mark = true;
        }
        if (eof_mark && empty()) {
            _output.end_input();
        }
        return;
    }
    if(index >= index_unass + _capacity){
        return;
    }
    if(eof){
        eof_mark = true;
    }
    if(index < index_unass){
        // index --- unass_bytes -- index + data.size()
        size_t offset = index_unass - index;
        size_t bytes_actual_written = std::min(data.size()-offset , _capacity - _output.buffer_size() - _unass_bytes); // PROBLEM
        if(offset + bytes_actual_written != data.size() && eof){
            eof_mark = false;
        }
        for(size_t i = 0 ; i < bytes_actual_written ; i++){
            if(_bitmap[i]){
                continue;
            }
            _unass_bytes += 1;
            _bitmap[i] = true;
            _buffer[i] = data[i + offset];
        } 
    }else{
        size_t offset = index - index_unass;
        size_t bytes_actual_written = std::min(data.size() , _capacity - _output.buffer_size() - offset); // PROBLEM
        if(bytes_actual_written != data.size() && eof){
            eof_mark = false;
        }
        for(size_t i = 0 ; i < bytes_actual_written ; i++){
            if(_bitmap[i + offset]){
                continue;
            }
            _unass_bytes += 1;
            _bitmap[i + offset] = true;
            _buffer[i + offset] = data[i];
        } 
    }
    mergeTo_output();
    if (eof_mark && empty()) {
        _output.end_input();
    }

}

void StreamReassembler::mergeTo_output(){
    while(_bitmap.front()){
       _output.write(_buffer.front());
        _unass_bytes -= 1;
        index_unass += 1;
        _bitmap.pop_front();
        _buffer.pop_front();
        _bitmap.push_back(false);
        _buffer.push_back('\0');
    }
}
size_t StreamReassembler::unassembled_bytes() const{
    return  _unass_bytes;
}
bool StreamReassembler::empty() const {
    return _unass_bytes == 0;
}
