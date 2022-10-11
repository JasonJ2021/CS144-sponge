#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (seg.header().syn) {
        syn_flag = true;
        syn_no = seg.header().seqno;  // set the syn_no wrapping int32
    }
    if (seg.header().fin) {
        fin_flag = true;
    }
    if(!syn_flag){
        return;
    }
    uint64_t checkpoint = _reassembler.stream_out().bytes_written();
    string data = {seg.payload().str().begin(), seg.payload().str().end()};
    int64_t index = unwrap(seg.header().seqno, syn_no, checkpoint);
    if(index == 0 && !seg.header().syn){
        return;
    }
    if(index > 0){
        index--;
    }
    if(index < 0){
        return;
    }
    if(static_cast<uint64_t>(index) < checkpoint && index + data.size() > checkpoint){
        data = data.substr(checkpoint - index , data.size() - (checkpoint -index));
        index = checkpoint;
    }
    _reassembler.push_substring(data, index , seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!syn_flag) {
        return std::nullopt;
    }
    uint64_t checkpoint = _reassembler.stream_out().bytes_written();
    checkpoint++;
    if (_reassembler.stream_out().input_ended()) {
        checkpoint += 1;
    }
    return wrap(checkpoint, syn_no);
}

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
