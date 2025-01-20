
//lsxxc production made on Jan 18 2025 

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>

#define PACKET_SIZE 64
#define TIMEOUT 1

// Calculate checksum for ICMP packet
unsigned short calculate_checksum(void *buffer, int length) {
    unsigned short *buf = reinterpret_cast<unsigned short *>(buffer);
    unsigned int sum = 0;
    unsigned short result;

    for (; length > 1; length -= 2)
        sum += *buf++;
    if (length == 1)
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

//ICMP ping request
bool ping(const std::string &ip_address) {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("Socket error");
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_address.c_str(), &addr.sin_addr);

    char packet[PACKET_SIZE];
    memset(packet, 0, PACKET_SIZE);

    struct icmp *icmp_hdr = reinterpret_cast<struct icmp *>(packet);
    icmp_hdr->icmp_type = ICMP_ECHO;
    icmp_hdr->icmp_code = 0;
    icmp_hdr->icmp_id = getpid() & 0xFFFF;
    icmp_hdr->icmp_seq = 1;
    icmp_hdr->icmp_cksum = calculate_checksum(packet, sizeof(struct icmp));

    if (sendto(sockfd, packet, sizeof(struct icmp), 0, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) <= 0) {
        perror("Sendto error");
        close(sockfd);
        return false;   //flase if packet does not go through 
    }

    struct timeval timeout = {TIMEOUT, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));

    char buffer[PACKET_SIZE];
    socklen_t addr_len = sizeof(addr);
    if (recvfrom(sockfd, buffer, PACKET_SIZE, 0, reinterpret_cast<struct sockaddr *>(&addr), &addr_len) <= 0) {
        close(sockfd);
        return false; // Timeout or no response
    }

    close(sockfd);
    return true; // Host is reachable
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <base_ip> <range>\n";
        std::cerr << "Example: " << argv[0] << " 192.168.1 10\n";
        return 1;
    }

    std::string base_ip = argv[1];
    int range = std::stoi(argv[2]);

    std::cout << "Scanning IP range " << base_ip << ".0 - " << base_ip << "." << range << "\n";

    for (int i = 1; i <= range; ++i) {
        std::string ip_address = base_ip + "." + std::to_string(i);
        std::cout << "Pinging " << ip_address << "... ";

        if (ping(ip_address)) {
            std::cout << "[ACTIVE]\n";
        } else {
            std::cout << "[INACTIVE]\n";
        }
    }

    return 0;
}

