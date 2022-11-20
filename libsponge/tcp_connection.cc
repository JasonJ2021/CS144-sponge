#include "tcp_connection.hh"

#include <iostream>
#include <limits>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::try_send() {
    while (!_sender.segments_out().empty()) {
        TCPSegment &segment_sent = _sender.segments_out().front();
        _sender.segments_out().pop();
        // 携带上ackno & ACK flags
        if (_receiver.ackno().has_value()) {
            segment_sent.header().ack = true;
            segment_sent.header().ackno = _receiver.ackno().value();
        }
        segment_sent.header().win = _receiver.window_size() > std::numeric_limits<uint16_t>::max()
                                        ? std::numeric_limits<uint16_t>::max()
                                        : _receiver.window_size();
        _segments_out.push(segment_sent);
    }
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    // 处理RST flag
    if (!is_active)
        return;
    _time_since_last_segment_received = 0;  // 空，无效segment是否算？
    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        is_active = false;
        // How to kill the connection?
        return;
    }
    // 把segment交给TCPReceiver , 使其能够查看seqno,syn,payload and fin
    _receiver.segment_received(seg);

    // 如果ACK flag被设置，告知TCPSender ackno and window_size;
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    // 如果到达的segment拥有任何seqno，TCPConnection保证有一个segment被发送
    bool should_sent = false;
    if (seg.length_in_sequence_space() != 0) {
        _sender.fill_window();
        should_sent = true;
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
    }
    if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) &&
        seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
        should_sent = true;
    }
    // 有要发送的segment，把ack,window_size放在这个Segment中
    if (should_sent) {
        // TCPSegment &segment_sent = _sender.segments_out().front();
        // _sender.segments_out().pop();
        // // 携带上ackno & ACK flags
        // if (_receiver.ackno().has_value()) {
        //     segment_sent.header().ack = true;
        //     segment_sent.header().ackno = _receiver.ackno().value();
        // }
        // segment_sent.header().win = _receiver.window_size() > std::numeric_limits<uint16_t>::max()
        //                                 ? std::numeric_limits<uint16_t>::max()
        //                                 : _receiver.window_size();
        // _segments_out.push(segment_sent);
        try_send();
    }
    // receiver.stream_out.eof ? end_input ?
    if (!_sender.stream_in().eof() && _receiver.stream_out().eof()) {
        _linger_after_streams_finish = false;
    }
}

bool TCPConnection::active() const { return is_active; }

size_t TCPConnection::write(const string &data) {
    size_t actual_written = _sender.stream_in().write(data);
    // 尝试进行发送
    _sender.fill_window();
    if (!_sender.segments_out().empty()) {
        // TCPSegment &segment_sent = _sender.segments_out().front();
        // _sender.segments_out().pop();
        // // 携带上ackno & ACK flags
        // if (_receiver.ackno().has_value()) {
        //     segment_sent.header().ack = true;
        //     segment_sent.header().ackno = _receiver.ackno().value();
        // }
        // segment_sent.header().win = _receiver.window_size() > std::numeric_limits<uint16_t>::max()
        //                                 ? std::numeric_limits<uint16_t>::max()
        //                                 : _receiver.window_size();
        // _segments_out.push(segment_sent);
        try_send();
    }
    return actual_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // 告知TCPSender时间的流逝
    _sender.tick(ms_since_last_tick);
    try_send();
    _time_since_last_segment_received += ms_since_last_tick;
    // 重传次数超过max_retx_attempts -> abort the connection
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        // abort the connection and send a reset segment to the peer
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        is_active = false;
        // 发送reset segment
        _sender.send_empty_segment();
        TCPSegment &segment_sent = _sender.segments_out().front();
        _sender.segments_out().pop();
        segment_sent.header().rst = true;
        while (!_segments_out.empty()) {
            _segments_out.pop();
        }
        _segments_out.push(segment_sent);
        try_send();
    }
    // Or cleanly end the connection
    // 满足PREQ 1~3并且_linger = false 时，cleanDone
    if (_receiver.stream_out().eof() && _sender.stream_in().eof() && !_sender.bytes_in_flight()) {
        if (!_linger_after_streams_finish) {
            is_active = false;
        } else if (_time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
            is_active = false;
        }
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    // 尝试发送 fin
    _sender.fill_window();
    // TCPSegment &segment_sent = _sender.segments_out().front();
    // _sender.segments_out().pop();
    // // 携带上ackno & ACK flags
    // if (_receiver.ackno().has_value()) {
    //     segment_sent.header().ack = true;
    //     segment_sent.header().ackno = _receiver.ackno().value();
    // }
    // segment_sent.header().win = _receiver.window_size() > std::numeric_limits<uint16_t>::max()
    //                                 ? std::numeric_limits<uint16_t>::max()
    //                                 : _receiver.window_size();
    // _segments_out.push(segment_sent);
    try_send();
}

void TCPConnection::connect() {
    // Initiate a connection by sending a SYN segment
    _sender.fill_window();
    // std::queue<TCPSegment> &out_queue = _sender.segments_out();
    // _segments_out.push(_sender.segments_out().front());
    // out_queue.pop();
    try_send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            // 发送reset segment
            _sender.send_empty_segment();
            TCPSegment &segment_sent = _sender.segments_out().front();
            _sender.segments_out().pop();
            segment_sent.header().rst = true;
            while (!_segments_out.empty()) {
                _segments_out.pop();
            }
            _segments_out.push(segment_sent);
            try_send();
            is_active = false;
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
