//
// Created by kdl4u_000 on 1/9/2019.
//


// Returns hostname for the local computer
void checkHostName(int hostname) {
    if (hostname == -1)
    {
        perror("gethostname");
        exit(1);
    }
}

// Returns host information corresponding to host name
void checkHostEntry(struct hostent * hostentry) {
    if (hostentry == NULL)
    {
        perror("gethostbyname");
        exit(1);
    }
}

// Converts space-delimited IPv4 addresses
// to dotted-decimal format
void checkIPbuffer(char *IPbuffer) {
    if (NULL == IPbuffer)
    {
        perror("inet_ntoa");
        exit(1);
    }
}

unsigned long ip2int(const char *ip)
{
    const char *end = ip + strlen(ip);
    unsigned long n = 0;
    while (ip < end) {
        n <<= 8;
        n |= strtoul(ip, (char **)&ip, 10);
        ip++;
    }

    return n;
}

char* concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char * getMyAddress() {
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;

    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    checkHostName(hostname);

    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
    checkHostEntry(host_entry);

    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*)
            host_entry->h_addr_list[0]));

    //printf("Hostname: %s\n", hostbuffer);
    //printf("Host IP: %s", IPbuffer);
    return IPbuffer;
}

int ipComp(const char ip1[], const char ip2[]) {
    unsigned long ip_addr1 = ip2int(ip1);
    unsigned long ip_addr2 = ip2int(ip2);

    return (ip_addr1 >= ip_addr2);
}
