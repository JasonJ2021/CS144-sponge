#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _capacity{capacity}, _buffer{}, total_written{0}, total_read{0}, input_end{false} {}

size_t ByteStream::write(const string &data) {
    size_t written = 0;
    // while (written < data.size() && remaining_capacity() > 0) {
    //     q.push_back(data[written++]);
    // }
    written = std::min(data.size() , remaining_capacity());
    _buffer.append(BufferList(move(string().assign(data.begin(),data.begin()+written))));
    total_written += written;
    return written;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t actual_len = std::min(_buffer.size(), len);
    string s=_buffer.concatenate();
    return string().assign(s.begin(), s.begin() + actual_len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t actual_len = std::min(_buffer.size(), len);
    total_read += actual_len;
    _buffer.remove_prefix(actual_len);
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t actual_read = std::min(len , _buffer.size());
    string ans = peek_output(actual_read);
    pop_output(actual_read);
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
