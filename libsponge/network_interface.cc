#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    auto next_ethernet = _ip_2_ethernet.find(next_hop_ip);
    if (next_ethernet != _ip_2_ethernet.end()) {
        // 如果当前ip -> MAX is cached , send it right away
        EthernetHeader header;
        header.dst = next_ethernet->second.second;
        header.src = _ethernet_address;
        header.type = header.TYPE_IPv4;
        EthernetFrame frame;
        Buffer buffer = header.serialize() + dgram.serialize().concatenate();
        frame.parse(buffer);
        frames_out().push(std::move(frame));
    } else {
        // 当前dst MAC地址位置未知
        // 如果并且在5s内没有发送对应IP地址的MAC查询请求的话,发送一个ARP request，
        auto wait_datagram = _ip_2_waitDatagram.find(next_hop_ip);
        if (wait_datagram == _ip_2_waitDatagram.end()) {
            // 如果dst ip还没有进行ARP发送
            std::deque<InternetDatagram> deq;
            deq.push_back(dgram);
            _ip_2_waitDatagram.insert({next_hop_ip, {0, deq}});
            send_arp_request(next_hop_ip);
        } else if (wait_datagram->second.first > FIVE_SECONDS) {
            // 发送超过5秒，还是要进行ARP发送
            send_arp_request(next_hop_ip);
            wait_datagram->second.first = 0;
            wait_datagram->second.second.push_back(dgram);
        }
    }
}

void NetworkInterface::send_arp_request(uint32_t next_hop_ip) {
    EthernetHeader header;
    header.dst = ETHERNET_BROADCAST;
    header.src = _ethernet_address;
    header.type = header.TYPE_ARP;
    ARPMessage arp_message;
    arp_message.opcode = ARPMessage::OPCODE_REQUEST;
    arp_message.sender_ethernet_address = _ethernet_address;
    arp_message.sender_ip_address = _ip_address.ipv4_numeric();
    arp_message.target_ip_address = next_hop_ip;
    EthernetFrame frame;
    Buffer buffer = header.serialize() + arp_message.serialize();
    frame.parse(buffer);
    frames_out().push(std::move(frame));
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst != ETHERNET_BROADCAST && frame.header().dst != _ethernet_address) {
        return std::nullopt;
    }
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram datagram;
        if (datagram.parse(frame.payload().concatenate()) == ParseResult::NoError) {
            return std::optional<InternetDatagram>{datagram};
        }
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp_message;
        if (arp_message.parse(frame.payload().concatenate()) != ParseResult::NoError) {
            return std::nullopt;
        }
        if (arp_message.opcode == ARPMessage::OPCODE_REQUEST) {
            if(arp_message.target_ip_address != _ip_address.ipv4_numeric()){
                return std::nullopt; // 如果目标不是现在这个eth，忽略~
            }
            // 先处理请求，要把src ip address -> ether addr记录到table中
            _ip_2_ethernet[arp_message.sender_ip_address] = std::make_pair(0, arp_message.sender_ethernet_address);
            // 发送一个reply arpmessage
            arp_message.opcode = ARPMessage::OPCODE_REPLY;
            arp_message.target_ip_address = arp_message.sender_ip_address;
            arp_message.target_ethernet_address = frame.header().src;
            arp_message.sender_ip_address = _ip_address.ipv4_numeric();
            arp_message.sender_ethernet_address = _ethernet_address;
            EthernetHeader header;
            header.dst = frame.header().src;
            header.src = _ethernet_address;
            header.type = header.TYPE_ARP;
            EthernetFrame frame_sending;
            Buffer buffer = header.serialize() + arp_message.serialize();
            frame_sending.parse(buffer);
            frames_out().push(std::move(frame_sending));
            return std::nullopt;
        }
        if (arp_message.opcode == ARPMessage::OPCODE_REPLY) {
            // 把接受到的target_ip_address -> target_ethernet_address之间的关系建立.
            _ip_2_ethernet[arp_message.sender_ip_address] = std::make_pair(0, arp_message.sender_ethernet_address);
            // 把之前因为没有ethernet而没有发送的datagram发送一下
            auto wait_datagram = _ip_2_waitDatagram.find(arp_message.sender_ip_address);
            auto datagram_deque = wait_datagram->second.second;
            while (!datagram_deque.empty()) {
                EthernetHeader header;
                header.dst = arp_message.sender_ethernet_address;
                header.src = _ethernet_address;
                header.type = header.TYPE_IPv4;
                EthernetFrame frame_sending;
                Buffer buffer = header.serialize() + datagram_deque.front().serialize().concatenate();
                frame_sending.parse(buffer);
                frames_out().push(std::move(frame_sending));
                datagram_deque.pop_front();
            }
            _ip_2_waitDatagram.erase(wait_datagram);
        }
    }
    return std::nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // 需要更新ip -> ethernet table，把超过30秒的映射entry踢掉
    for(auto it = _ip_2_ethernet.begin() ; it != _ip_2_ethernet.end() ; ){
        it->second.first += ms_since_last_tick;
        if(it->second.first >= THIRTY_SECONDS){
            it = _ip_2_ethernet.erase(it);
            continue;
        }
        it++;
    }
    // 需要更新 wait_datagram队列
    for(auto it = _ip_2_waitDatagram.begin() ; it != _ip_2_waitDatagram.end() ; it++){
        it->second.first += ms_since_last_tick;
    }
}
