#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace std;

bool should_close = false;

void input_thread(int client_socket, int remaining_time) {
    auto start_time = chrono::steady_clock::now();

    while (true) {
        // Simulate time passing (you need a more accurate way based on your requirements)
        this_thread::sleep_for(chrono::seconds(1));

        auto current_time = chrono::steady_clock::now();
        auto elapsed_time = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();

        // Check if the client should close the connection
        if (elapsed_time >= remaining_time) {
            cout << "Sending 'close' to the child server.\n";
            char buffer[1024];
            strcpy(buffer, "close");
            send(client_socket, buffer, strlen(buffer), 0);
            should_close = true;
            break;
        }
    }
}

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        cerr << "Error creating socket\n";
        return -1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(5000); // Use the master server port

    while (true) {
        if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
            cerr << "Error connecting to server\n";
            close(client_socket);
            return -1;
        }

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        cout << "Enter the client type: ";
        cin >> buffer;
        // Send the client type to the master server
        send(client_socket, buffer, strlen(buffer), 0);

        memset(buffer, 0, sizeof(buffer));
        cout << "Enter the client time in seconds: ";
        cin >> buffer;
        // Send the client time to the master server
        send(client_socket, buffer, strlen(buffer), 0);

        int remaining_time = atoi(buffer);

        // Start the input thread
        thread input(input_thread, client_socket, remaining_time);

        // Receive the port number of the child server from the master server
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received == -1) {
            cerr << "Error receiving data\n";
        } else if (bytes_received == 0) {
            cout << "Connection closed by server\n";
            break;
        } else {
            // Extract the port number from the received message
            int child_server_port = atoi(buffer);

            // Connect to the child server using the received port number
            sockaddr_in child_server_address{};
            child_server_address.sin_family = AF_INET;
            child_server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
            child_server_address.sin_port = htons(child_server_port);

            // Create a new socket for connecting to the child server
            close(client_socket); // Close the connection to the master server
            client_socket = socket(AF_INET, SOCK_STREAM, 0);

            if (client_socket == -1) {
                cerr << "Error creating socket\n";
                return -1;
            }

            if (connect(client_socket, (struct sockaddr*)&child_server_address, sizeof(child_server_address)) == -1) {
                cerr << "Error connecting to child server\n";
                close(client_socket);
                return -1;
            }

            cout << "Connected to child server on port " << child_server_port << "\n";

            // Wait for the input thread to finish
            input.join();
        }
    }

    close(client_socket);
    return 0;
}
