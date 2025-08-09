#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

// Server identifier to distinguish between encryption and decryption servers
char* serverIdentifier = "2";

// Valid characters for the one-time pad encryption (A-Z and space)
char validCharacters[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

// Function to locate the index of a character in the validCharacters array
int locateCharacterIndex(char searchCharacter) {
    for (int i = 0; i < 27; i++) {
        if (validCharacters[i] == searchCharacter) return i; // Return the index if the character is found
    }
    fprintf(stderr, "Error: locateCharacterIndex could not find the character.\n");
    return -1; // Return -1 if the character is not found
}

// Function to handle errors and exit the program
void displayError(const char *message) {
    perror(message);
    exit(1);
}

// Function to initialize the server address structure
void initializeAddress(struct sockaddr_in* address, int portNumber){
    memset((char*) address, '\0', sizeof(*address)); // Clear the address structure
    address->sin_family = AF_INET; // Set address family to IPv4
    address->sin_port = htons(portNumber); // Set port number, converting to network byte order
    address->sin_addr.s_addr = INADDR_ANY; // Allow any incoming IP address
}

int main(int argc, char *argv[]){
    int clientSocket, bytesRead, bytesSent; // Variables for socket communication
    char buffer[BUFFER_SIZE]; // Buffer for storing data
    size_t bufferCapacity = sizeof(char) * BUFFER_SIZE; // Size of the buffer
    char acknowledgmentMessage[15] = "ACK: received."; // Acknowledgment message
    int acknowledgmentSize = strlen(acknowledgmentMessage); // Length of the acknowledgment message
    char* acknowledgmentBuffer = malloc(sizeof(char) * (BUFFER_SIZE)); // Allocate memory for acknowledgment buffer
    char* keyFilePath = malloc(sizeof(char) * 255); // Allocate memory for key file path
    struct sockaddr_in serverAddress, clientAddress; // Address structures for server and client
    socklen_t clientAddressSize = sizeof(clientAddress); // Size of the client address structure

    // Check if the correct number of arguments is provided
    if (argc < 2) {
        fprintf(stderr,"USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create a socket for the server
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        fprintf(stderr, "ERROR opening socket");
        exit(EXIT_FAILURE);
    }
    
    // Initialize the server address structure with the provided port number
    initializeAddress(&serverAddress, atoi(argv[1]));

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
        displayError("ERROR on binding");
    
    // Listen for incoming connections, with a backlog of 5
    listen(serverSocket, 5);

    // Main server loop to handle client connections
    while(true) {
        // Accept a new client connection
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressSize);
        if (clientSocket < 0) {
            fprintf(stderr, "ERROR on accept");
            continue; // Skip to the next iteration if accept fails
        }

        /*
         * Authenticating client-server connection
         */
        char* clientIdentifierBuffer = malloc(sizeof(char) * 256); // Allocate memory for client identifier buffer
        memset(clientIdentifierBuffer, '\0', 256); // Clear the client identifier buffer
        bytesSent = send(clientSocket, serverIdentifier, strlen(serverIdentifier), 0); // Send server identifier to client
        if (bytesSent < 0) perror("Error sending serverIdentifier\n");

        bytesRead = recv(clientSocket, clientIdentifierBuffer, 1, 0); // Receive client identifier
        if (bytesRead < 0) perror("Error reading clientIdentifierBuffer\n");

        // Check if the client identifier matches the server identifier
        if (strcmp(clientIdentifierBuffer, serverIdentifier) != 0) {
            perror("Unauthorized client attempted connection. Closing.\n");
            close(clientSocket); // Close the connection if client is unauthorized
            continue; // Skip to the next iteration
        }

        /*
         * Receiving key size
         */
        memset(buffer, '\0', bufferCapacity); // Clear the buffer
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0); // Receive key size from client
        if (bytesRead < 0) perror("ERROR reading key size");
        
        bytesSent = send(clientSocket, acknowledgmentMessage, acknowledgmentSize, 0); // Send acknowledgment to client
        if (bytesSent < 0) perror("Error sending acknowledgment for key size");

        int keySizeCapacity = 255; // Buffer size for key size
        char* keySizeString = malloc(sizeof(char) * (keySizeCapacity + 1)); // Allocate memory for key size string
        memset(keySizeString, '\0', keySizeCapacity + 1); // Clear the key size buffer
        strcpy(keySizeString, buffer); // Copy received key size to keySizeString

        /*
         * Receiving key file path
         */
        memset(buffer, '\0', bufferCapacity); // Clear the buffer
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0); // Receive key file path from client
        if (bytesRead < 0) fprintf(stderr, "ERROR reading key file path\n");

        bytesSent = send(clientSocket, acknowledgmentMessage, 14, 0); // Send acknowledgment to client
        memset(keyFilePath, '\0', 255); // Clear the key file path buffer
        strcpy(keyFilePath, buffer); // Copy received key file path to keyFilePath

        /*
         * Receiving encrypted message size
         */
        memset(buffer, '\0', bufferCapacity); // Clear the buffer
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0); // Receive encrypted message size from client
        if (bytesRead < 0) fprintf(stderr, "ERROR reading encrypted message size\n");
        
        bytesSent = send(clientSocket, acknowledgmentMessage, acknowledgmentSize, 0); // Send acknowledgment to client
        int encryptedMessageSize = atoi(buffer); // Convert received message size to integer

        /*
         * Receiving encrypted message
         */
        int keySize = atoi(keySizeString); // Convert key size to integer
        int receiveIterations = (encryptedMessageSize / 100) + 1; // Calculate the number of iterations needed to receive the message
        int count = 0; // Counter to track the number of characters received
        char* encryptedMessage = malloc(sizeof(char) * (keySize + 1)); // Allocate memory for encrypted message
        memset(encryptedMessage, '\0', keySize + 1); // Clear the encrypted message buffer

        while (receiveIterations != 0) {
            int charsToReceive = (receiveIterations == 1) ? (encryptedMessageSize % 100) : 100; // Calculate the number of characters to receive in this iteration
            if (charsToReceive == 0) charsToReceive = 100; // If the remaining characters are 0, receive 100 characters

            memset(buffer, '\0', bufferCapacity); // Clear the buffer
            bytesRead = recv(clientSocket, buffer, charsToReceive, 0); // Receive the encrypted message chunk
            if (bytesRead < 0) fprintf(stderr, "ERROR reading encrypted message\n");

            bytesSent = send(clientSocket, acknowledgmentMessage, acknowledgmentSize, 0); // Send acknowledgment to client
            strcat(encryptedMessage, buffer); // Append the received chunk to the encrypted message buffer
            count += charsToReceive; // Update the counter
            receiveIterations--; // Decrement the remaining iterations
        }

        /*
         * Decrypting message
         */
        char* decryptionKey = malloc(sizeof(char) * (keySize + 1)); // Allocate memory for decryption key
        memset(decryptionKey, '\0', keySize + 1); // Clear the decryption key buffer

        FILE* keyFileHandler = fopen(keyFilePath, "r"); // Open the key file for reading
        if (keyFileHandler == NULL) {
            fprintf(stderr, "ERROR opening key file: %s\n", keyFilePath);
            exit(EXIT_FAILURE);
        }

        char character;
        int index = 0;
        while ((character = fgetc(keyFileHandler)) != EOF && character != '\n' && index < keySize) {
            decryptionKey[index++] = character; // Read the key from the file
        }
        fclose(keyFileHandler); // Close the key file

        char* decryptedMessage = malloc(sizeof(char) * (strlen(encryptedMessage) + 1)); // Allocate memory for decrypted message
        memset(decryptedMessage, '\0', strlen(encryptedMessage) + 1); // Clear the decrypted message buffer
        for (int i = 0; i < strlen(encryptedMessage); i++) {
            int encryptedCharIndex = locateCharacterIndex(encryptedMessage[i]); // Get index of encrypted character
            int keyCharIndex = locateCharacterIndex(decryptionKey[i]); // Get index of key character
            int newCharIndex = (encryptedCharIndex - keyCharIndex) % 27; // Calculate new character index using modulo 27
            if (newCharIndex < 0) newCharIndex += 27; // Handle negative indices
            decryptedMessage[i] = validCharacters[newCharIndex]; // Store the decrypted character
        }

        /*
         * Sending decrypted message
         */
        count = 0; // Reset counter for sending decrypted message
        receiveIterations = (encryptedMessageSize / 100) + 1; // Calculate the number of iterations needed to send the message
        while (receiveIterations != 0) {
            int charsToSend = (receiveIterations == 1) ? (encryptedMessageSize % 100) : 100; // Calculate the number of characters to send in this iteration
            if (charsToSend == 0) charsToSend = 100; // If the remaining characters are 0, send 100 characters

            bytesSent = send(clientSocket, decryptedMessage + count, charsToSend, 0); // Send the decrypted message chunk
            memset(acknowledgmentBuffer, '\0', BUFFER_SIZE); // Clear the acknowledgment buffer
            bytesRead = recv(clientSocket, acknowledgmentBuffer, acknowledgmentSize, 0); // Receive acknowledgment from client
            count += charsToSend; // Update the counter
            receiveIterations--; // Decrement the remaining iterations
        }

        /*
         * Closing connection
         */
        close(clientSocket); // Close the client socket
    }
}