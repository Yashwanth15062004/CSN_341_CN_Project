#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include<fstream>

using namespace std;
int max_threshold=2;
vector<vector<int>> child_ports(10, vector<int>(5, 0)); // Example child server ports
mutex child_ports_mutex;
vector<pair<int,int>> v;
int find_child_port(int client_type) {
    lock_guard<mutex> lock(child_ports_mutex);
    int k=-1;
    int tim=0;
    for(int i=0;i<child_ports.size();i++){
        if(child_ports[i][4]==0&&(child_ports[i][0] == client_type&&child_ports[i][2]<max_threshold)){
        if(tim<child_ports[i][1]){
            tim=child_ports[i][1];
            k=i;
        }
            }
    }
    if(k!=-1){
        child_ports[k][2]++;
        child_ports[k][3]++;
        return 5001+k;
    }
   // sort(v.begin(),v.end(),greater<int>());
    for (int i = 0; i < child_ports.size(); i++) {
        
        if (child_ports[i][0] == 0&&child_ports[i][4]==0) {
            if (child_ports[i][0] == 0) {
                // Assign port number for the new client type
                child_ports[i][0] = client_type;
            }
            child_ports[i][2]++;
            child_ports[i][3]++;
            cout<<"\033[32mswitching on the child server "<<5001+i<<"\033[0m"<<endl;
            cout<<"for2"<<5001+i<<endl;
            return 5001 + i;
        }
    }

    return 0; // No available port found
}

void handle_client(int client_socket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

    if (bytes_received > 0) {
        // Handle the client on the master server (redirect to a child server)
        int client_type;
        int remaining_time;
        sscanf(buffer, "%d", &client_type);
        cout<<client_type<<endl;
         memset(buffer, 0, sizeof(buffer));
         recv(client_socket, buffer, sizeof(buffer), 0);
         sscanf(buffer, "%d", &remaining_time);
        cout<<remaining_time<<endl;
        int child_port = find_child_port(client_type);
        if (child_port != 0) {
            child_ports[child_port-5001][1]=max(child_ports[child_port-5001][1],remaining_time);
            if(child_ports[child_port-5001][3]%3==0) child_ports[child_port-5001][4]=child_ports[child_port-5001][1]+50;
            // Send the port number of the child server to the client
            memset(buffer, 0, sizeof(buffer));
            snprintf(buffer, sizeof(buffer), "%d", child_port);
            send(client_socket, buffer, strlen(buffer), 0);

            // Send the client type and remaining time to the child server
            memset(buffer, 0, sizeof(buffer));
            snprintf(buffer, sizeof(buffer), "%d %d", client_type, remaining_time);
            send(child_port, buffer, strlen(buffer), 0);

            // Receive "close" from the client
            memset(buffer, 0, sizeof(buffer));
            recv(client_socket, buffer, sizeof(buffer), 0);

            if (strcmp(buffer, "close") == 0) {
                lock_guard<mutex> lock(child_ports_mutex);
                child_ports[child_port - 5001][2]--;
               // if(child_ports[child_port - 5001][2]==0) child_ports[child_port - 5001][0]=0;
                cout << "Connection closed by client\n";
            }
        } else {
            // Handle the case when no available port is found
            cerr << "No available port for client type " << client_type << "\n";
        }
    }

    close(client_socket);
}

void start_child_server(int port) {
    int child_server = socket(AF_INET, SOCK_STREAM, 0);
    if (child_server == -1) {
        cerr << "Error creating child server socket\n";
        return;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(child_server, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        cerr << "Error binding child server socket\n";
        close(child_server);
        return;
    }

    if (listen(child_server, 1) == -1) {
        cerr << "Error listening on child server socket\n";
        close(child_server);
        return;
    }

    cout << "Child server listening on port " << port << "\n";

    while (true) {
        sockaddr_in client_address{};
        socklen_t client_address_size = sizeof(client_address);
        int client_socket = accept(child_server, (struct sockaddr*)&client_address, &client_address_size);

        if (client_socket == -1) {
            cerr << "Error accepting connection on child server\n";
            continue;
        }

        cout << "Accepted connection on child server from " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << "\n";

        // Handle the client request on the child server
        // You can implement your own logic here
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        recv(client_socket, buffer, sizeof(buffer), 0);

        if (strcmp(buffer, "close") == 0) {
            lock_guard<mutex> lock(child_ports_mutex);
            child_ports[port - 5001][2]--;
            if(child_ports[port - 5001][2]==0) child_ports[port - 5001][0]=0;
            cout << "Connection closed by client\n";
            if(child_ports[port - 5001][0]==0){
               cout<<"\033[31mswitching off the child server "<<port<<"\033[0m"<<endl; 
            }
        }

        close(client_socket);
    }

    close(child_server);
}
string func(int j){
    string p=to_string(j);
    p+="   ";
    for(int j=10-p.length();j>=0;j--){
        p=" "+p;
    }return p;
}

int main() {
    // Start child servers as separate threads
    vector<thread> child_threads;
    for (int port = 5001; port <= 5010; port++) {
        child_threads.emplace_back(start_child_server, port);
    }

    // Start the master server
    int master_server = socket(AF_INET, SOCK_STREAM, 0);

    if (master_server == -1) {
        cerr << "Error creating master server socket\n";
        return -1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(5000); // Use any available port for the master server

    if (bind(master_server, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        cerr << "Error binding master server socket\n";
        close(master_server);
        return -1;
    }

    if (listen(master_server, 5) == -1) {
        cerr << "Error listening on master server socket\n";
        close(master_server);
        return -1;
    }

    cout << "Master server listening on port 5000\n";

    // Create a thread to reduce counters periodically
    thread timer_thread([]() {
         ofstream outFile("server_status.txt");
    outFile <<"child server          ";
    for(int j=5001;j<5011;j++){
        outFile<<func(j);
    }outFile<<"\n"<<endl;
        while (true) {
            // Simulate time passing (you need a more accurate way based on your requirements)
            this_thread::sleep_for(chrono::seconds(1));

            // Decrement counters if they are greater than 0
            {
                lock_guard<mutex> lock(child_ports_mutex);
                 outFile <<"Company_type          ";
                for (int i = 0; i < child_ports.size(); ++i) {
                     outFile <<func(child_ports[i][0]);
                    if (child_ports[i][1] > 0) {
                        child_ports[i][1]--;
                    }
                    if (child_ports[i][4] > 0) {
                        child_ports[i][4]--;
                    }
                }
                outFile<<endl;
                outFile <<"Number of clients     ";
                for (int i = 0; i < child_ports.size(); ++i) {
                    outFile <<func(child_ports[i][2]);
                }outFile<<endl;
                outFile <<"Time to live          ";
                for (int i = 0; i < child_ports.size(); ++i) {
                    outFile <<func(child_ports[i][4]);
                }
                outFile<<"\n"<<endl;
            }
        }
    });

    while (true) {
        sockaddr_in client_address{};
        socklen_t client_address_size = sizeof(client_address);
        int client_socket = accept(master_server, (struct sockaddr*)&client_address, &client_address_size);

        if (client_socket == -1) {
            cerr << "Error accepting connection on master server\n";
            continue;
        }

        cout << "Accepted connection on master server from " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << "\n";

        // Handle the client on the master server (redirect to a child server)
        handle_client(client_socket);
    }

    // Close child server threads
    for (thread& thread : child_threads) {
        thread.join();
    }

    // Wait for the timer thread to finish
    timer_thread.join();

    close(master_server);
    return 0;
}