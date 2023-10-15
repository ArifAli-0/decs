#include <iostream>
#include <string>
#include <fstream>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <filesystem>

using namespace std;

class Client
{
    int sockfd;
    string program_filename;
    string server_info;
    int iterations;
    int sleep_time;
    int success;
    vector<double> response_time;
    struct sockaddr_in server_addr;

    void send_file()
    {
        uint32_t file_size = filesystem::file_size(program_filename);
        // cout << "File size" << file_size << endl;
        uint32_t length_to_send = htonl(file_size);
        // uint32_t length_to_send = file_size;

        if (write(sockfd, &length_to_send, sizeof(length_to_send)) < 0)
            throw("error sending file size");

        ifstream fin(program_filename, ios::binary);
        if (!fin)
            throw("error opening file");
        char buffer[1024];
        // cout << "Sending file.." << endl;
        while (!fin.eof())
        {
            memset(buffer, 0, sizeof(buffer));
            fin.read(buffer, sizeof(buffer));
            int k = fin.gcount();
            // todo: handle if kernel fails to send the data
            int n = write(sockfd, buffer, k);
            if (n < 0)
                throw("error writing file");
        }
        // cout << "File sent" << endl;
    }

    void receive_response()
    {
        string response = "";
        uint32_t message_size;
        if (read(sockfd, &message_size, sizeof(message_size)) < 0)
            throw("file size read error");
        message_size = ntohl(message_size);
        // cout << "Received file size " << message_size << endl;

        char buffer[1024];
        uint32_t read_bytes = 0;
        // cout << "Receiving file" << endl;
        while (read_bytes < message_size)
        {
            memset(buffer, 0, sizeof(buffer));
            uint32_t current_read = read(sockfd, buffer, sizeof(buffer));
            //  if(current_read ==0 ) throw("socket read error");
            response += string(buffer);
            read_bytes += current_read;
        }
        // cout << "Received: " << endl;
        cout << response << endl;
    }

    void setup_socket()
    {
        int t1 = server_info.find(":");
        string ip = server_info.substr(0, t1);
        string t2 = server_info.substr(t1 + 1);
        int port = stoi(t2);

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            throw("Error opening socket");

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        int sc = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sc < 0)
            throw("Cannot connect");
    }

public:
    void submit()
    {
        for (int i = 0; i < iterations; i++)
        {
            setup_socket();
            auto start_time = chrono::high_resolution_clock::now();
            send_file();
            receive_response();
            auto end_time = chrono::high_resolution_clock::now();
            double total_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
            response_time.push_back(total_time);
            success++;
            close(sockfd);
            sleep(sleep_time);
        }
    }

    Client(string server_info, string program_filename, int iterations, int sleep_time) : server_info(server_info),
                                                                                          program_filename(program_filename), iterations(iterations), sleep_time(sleep_time)
    {
        success = 0;
    };
};

int main(int argc, char const *argv[])
{
    // Process arguments
    if (argc != 5)
    {
        throw("Usage : ./client <serverip:port> <filetosubmit> <loopNum> <sleepTime> ");
        exit(0);
    }
    string server_info = argv[1];
    string program_filename = argv[2];
    int iterations = stoi(argv[3]);
    int sleeptime = stoi(argv[4]);

    try
    {
        Client client(server_info, program_filename, iterations, sleeptime);
        client.submit();
    }
    catch (const char *msg)
    {
        cerr << msg << '\n';
    }
    return 0;
}
