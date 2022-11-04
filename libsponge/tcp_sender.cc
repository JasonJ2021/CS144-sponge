#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "wrapping_integers.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _bytes_in_flight{0}
    , _outstanding_segments{}
    , cur_window_size{1}
    , _cur_RTO{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    bool has_sent = false;
    if (!syn_set) {
        // construct the segment
        has_sent = true;
        TCPSegment syn_segment;
        syn_segment.header().seqno = next_seqno();
        syn_segment.header().syn = true;
        // push the segment to outstanding and out queue
        this->_segments_out.emplace(syn_segment);
        this->_outstanding_segments.emplace(syn_segment);
        syn_set = true;
        // cur_window_size--;
        // update the next_seq and in_flight_bytes
        _next_seqno += 1;
        _bytes_in_flight += syn_segment.length_in_sequence_space();
    }
    while (cur_window_size > 0 && !_stream.buffer_empty()) {
        has_sent = true;
        TCPSegment send_segment;
        send_segment.header().seqno = next_seqno();
        // construct the segment
        size_t payload_len = std::min(TCPConfig::MAX_PAYLOAD_SIZE, std::min(_stream.buffer_size(), cur_window_size));
        send_segment.payload() = Buffer(_stream.peek_output(payload_len));
        _stream.pop_output(payload_len);
        cur_window_size -= payload_len;
        if (_stream.eof() && _stream.buffer_size() == 0 && cur_window_size > 0 && !fin_set) {
            send_segment.header().fin = true;
            _next_seqno += 1;
            cur_window_size--;
            payload_len += 1;
            fin_set = true;
        }
        // push the segment to outstanding and out queue
        this->_segments_out.emplace(send_segment);
        this->_outstanding_segments.emplace(send_segment);
        // update the next_seq
        _next_seqno += payload_len;
        // update the in_flight_bytes
        _bytes_in_flight += send_segment.length_in_sequence_space();
    }
    // if (_stream.eof() && _stream.buffer_size() == 0 && cur_window_size > 0 && !fin_set) {
    //     // send fin segment
    //     TCPSegment send_segment;
    //     send_segment.header().seqno = next_seqno();
    //     send_segment.header().fin = true;

    //     _next_seqno += 1;
    //     _bytes_in_flight += send_segment.length_in_sequence_space();
    //     cur_window_size--;
    //     fin_set = true;
    //     this->_segments_out.emplace(send_segment);
    //     this->_outstanding_segments.emplace(send_segment);
    // }
    if (has_sent) {
        timer_started = true;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // set cur_windows_size and ackno_abs
    uint64_t ackno_abs = unwrap(ackno, this->_isn, this->next_seqno_absolute());
    if(ackno_abs > this->_next_seqno){
        return;
    }
    cur_window_size = window_size;
    bool new_data_acked = false;
    while (!this->_outstanding_segments.empty()) {
        uint64_t seg_seqno_unwrapped =
            unwrap(this->_outstanding_segments.front().header().seqno, this->_isn, this->next_seqno_absolute());
        if (seg_seqno_unwrapped + this->_outstanding_segments.front().length_in_sequence_space() <= ackno_abs) {
            // remove outstanding segments fully acked
            _bytes_in_flight -= this->_outstanding_segments.front().length_in_sequence_space();
            this->_outstanding_segments.pop();
            new_data_acked = true;
        } else {
            break;
        }
    }
    if (new_data_acked) {
        this->_cur_RTO = this->_initial_retransmission_timeout;
        if (!this->_outstanding_segments.empty()) {
            this->timer_started = true;
            this->time_elapsed = 0;
        }
        this->_consecutive_retransmissions = 0;
    }

    if (this->_outstanding_segments.empty()) {
        // ALL outstanding segments acked -> stop the timer
        this->timer_started = false;
        this->time_elapsed = 0;
        this->_cur_RTO = this->_initial_retransmission_timeout;
        this->_consecutive_retransmissions = 0;
    }
    fill_window();
    if (_stream.eof() && _stream.buffer_size() == 0 && cur_window_size > 0 && !fin_set) {
        // send fin segment
        TCPSegment send_segment;
        send_segment.header().seqno = next_seqno();
        send_segment.header().fin = true;

        _next_seqno += 1;
        _bytes_in_flight += send_segment.length_in_sequence_space();
        cur_window_size--;
        fin_set = true;
        this->_segments_out.emplace(send_segment);
        this->_outstanding_segments.emplace(send_segment);
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (timer_started) {
        time_elapsed += ms_since_last_tick;
        if (time_elapsed >= _cur_RTO) {
            this->_segments_out.push(this->_outstanding_segments.front());
            if (cur_window_size > 0) {
                this->_consecutive_retransmissions++;
                _cur_RTO = 2 * _cur_RTO;
            }
            time_elapsed = 0;
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment empty_segment;
    empty_segment.header().seqno = this->next_seqno();
    this->segments_out().emplace(empty_segment);
}
