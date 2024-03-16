#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

void sendResponse(int clientSocket, const std::string& status, const std::string& contentType, const std::string& content) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "Connection: close\r\n\r\n";
    response << content;

    send(clientSocket, response.str().c_str(), response.str().size(), 0);
}

void serveFile(int clientSocket, const std::string& filename, const std::string& contentType) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        sendResponse(clientSocket, "404 Not Found", "text/plain", "File Not Found");
        return;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string responseContent(fileSize, ' ');
    if (file.read(&responseContent[0], fileSize)) {
        sendResponse(clientSocket, "200 OK", contentType, responseContent);
    } else {
        sendResponse(clientSocket, "500 Internal Server Error", "text/plain", "Error reading file");
    }
}

void handleRequest(int clientSocket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    recv(clientSocket, buffer, BUFFER_SIZE, 0);
    std::istringstream iss(buffer);
    std::string requestType, requestPath;
    iss >> requestType >> requestPath;

    if (requestType == "GET") {
        if (requestPath == "/" || requestPath == "/index.html") {
            serveFile(clientSocket, "index.html", "text/html");
        } else if (requestPath == "/1.png") {
            serveFile(clientSocket, "1.png", "image/png");
        } else {
            sendResponse(clientSocket, "404 Not Found", "text/plain", "Not Found");
        }
    } else {
        sendResponse(clientSocket, "405 Method Not Allowed", "text/plain", "Method Not Allowed");
    }
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t sinSize = sizeof(struct sockaddr_in);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket\n";
        return 1;
    }

    // Prepare server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serverAddr.sin_zero), '\0', 8);

    // Bind socket to address
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
        std::cerr << "Error binding socket to address\n";
        return 1;
    }

    // Listen for connections
    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Error listening on socket\n";
        return 1;
    }

    std::cout << "Server running on port " << PORT << std::endl;

    while (1) {
        // Accept incoming connection
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &sinSize);
        if (clientSocket == -1) {
            std::cerr << "Error accepting connection\n";
            continue;
        }

        std::cout << "Connection accepted\n";

        // Handle client request
        handleRequest(clientSocket);

        // Close client connection
        close(clientSocket);
    }

    // Close server socket
    close(serverSocket);

    return 0;
}
