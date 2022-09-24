#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : size{capacity}, q{}, total_written{0}, total_read{0}, input_end{false} {}

size_t ByteStream::write(const string &data) {
    size_t written = 0;
    while (written < data.size() && remaining_capacity() > 0) {
        q.push_back(data[written++]);
    }
    total_written += written;
    return written;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t actual_len = std::min(q.size(), len);
    string temp = "";
    for (size_t i = 0; i < actual_len; i++) {
        temp += q[i];
    }
    return temp;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t actual_len = std::min(q.size(), len);
    total_read += actual_len;
    while (actual_len) {
        q.pop_front();
        actual_len--;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ans = "";
    size_t actual_read = 0;
    while (len > 0 && q.size() > 0) {
        actual_read++;
        ans += q[0];
        q.pop_front();
    }
    total_read += actual_read;
    return ans;
}

// void ByteStream::end_input() {}

// bool ByteStream::input_ended() const { return {}; }

// size_t ByteStream::buffer_size() const { return {}; }

// bool ByteStream::buffer_empty() const { return {}; }

// bool ByteStream::eof() const { return false; }

// size_t ByteStream::bytes_written() const { return {}; }

// size_t ByteStream::bytes_read() const { return {}; }

// size_t ByteStream::remaining_capacity() const { return {}; }
