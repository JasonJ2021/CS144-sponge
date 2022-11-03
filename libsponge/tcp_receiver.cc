#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // set syn , fin flags
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
    uint64_t checkpoint = _reassembler.stream_out().bytes_written();        // checkpoint = first reassembled index = ackno
    string data = {seg.payload().str().begin(), seg.payload().str().end()}; // fetch the data
    int64_t index = unwrap(seg.header().seqno, syn_no, checkpoint);         // get the segment absulote index (+ fin + syn)
    if(index < 0 || (index == 0 && !seg.header().syn)){
        return;
    }
    if(index > 0){
        index--; // syn is not included as stream number
    }
    _reassembler.push_substring(data, index , seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!syn_flag) {
        return std::nullopt;
    }
    uint64_t checkpoint = _reassembler.stream_out().bytes_written();
    checkpoint++; // include syn
    if (_reassembler.stream_out().input_ended()) {
        checkpoint += 1; // fin is sent to reassembler , bytestream.input_ended == true 
        // only if the all stream has been written to bytestream , because of the SR implementation
    }
    return wrap(checkpoint, syn_no);
}

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
