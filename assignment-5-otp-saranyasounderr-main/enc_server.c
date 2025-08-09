#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_CAPACITY 1024

// Server identifier to distinguish between encryption and decryption servers
char* serverIdentifier = "1";

// Valid characters for the one-time pad encryption (A-Z and space)
char validCharacters[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

// Function to display an error message and exit the program
void displayError(const char *message) {
    perror(message);
    exit(1);
} // end of "displayError" function

// Function to locate the index of a character in the validCharacters array
int locateCharacterIndex(char searchCharacter) {
    for (int i = 0; i < 27; i++) {
        if (validCharacters[i] == searchCharacter) return i;
    }
    fprintf(stderr, "locateCharacterIndex failed to find the character in validCharacters array.\n");
    return -1;
} // end of "locateCharacterIndex" function

// Function to initialize the server address structure
void initializeAddress(struct sockaddr_in* address, int portNumber){
    memset((char*) address, '\0', sizeof(*address)); // Clear the address structure
    address->sin_family = AF_INET; // Set address family to IPv4
    address->sin_port = htons(portNumber); // Set port number, converting to network byte order
    address->sin_addr.s_addr = INADDR_ANY; // Allow any incoming IP address
} // end of "initializeAddress" function

int main(int argc, char *argv[]){
    // Variables for socket communication
    int clientSocket, bytesRead, bytesSent;
    char dataBuffer[BUFFER_CAPACITY]; // Buffer for storing data
    size_t bufferSize = sizeof(char) * BUFFER_CAPACITY; // Size of the buffer
    char acknowledgment[15] = "ACK: received."; // Acknowledgment message
    int acknowledgmentSize = strlen(acknowledgment); // Length of the acknowledgment message
    int ackBufferSize = 255; // Buffer size for acknowledgment
    char* ackBuffer = malloc(sizeof(char) * (ackBufferSize + 1)); // Allocate memory for acknowledgment buffer
    int keyFilePathSize = 255; // Buffer size for key file path
    char* keyFilePath = malloc(sizeof(char) * (keyFilePathSize + 1)); // Allocate memory for key file path
    size_t keyFileBufferSize = sizeof(keyFilePath); // Size of the key file path buffer
    struct sockaddr_in serverAddr, clientAddr; // Address structures for server and client
    socklen_t clientAddrSize = sizeof(clientAddr); // Size of the client address structure

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
    initializeAddress(&serverAddr, atoi(argv[1]));

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
        displayError("ERROR on binding");
    }

    // Listen for incoming connections, with a backlog of 5
    listen(serverSocket, 5);

    // Main server loop to handle client connections
    while(true) {
        // Accept a new client connection
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);
        if (clientSocket < 0){
            fprintf(stderr, "ERROR on accept");
        }

        /*
         * Authenticating server and client ID's
         */
        char* clientIdentifier = malloc(sizeof(char) * 256); // Allocate memory for client identifier
        size_t clientIdentifierSize = sizeof(clientIdentifier); // Size of the client identifier buffer
        memset(clientIdentifier, '\0', clientIdentifierSize); // Clear the client identifier buffer
        bytesSent = send(clientSocket, serverIdentifier, strlen(serverIdentifier), 0); // Send server identifier to client
        if (bytesSent < 0) perror("Error sending serverIdentifier\n");
        bytesRead = recv(clientSocket, clientIdentifier, 1, 0); // Receive client identifier
        if (bytesRead < 0) perror("Error reading clientIdentifier\n");
        if (strcmp(clientIdentifier, serverIdentifier) != 0) { // Check if client identifier matches server identifier
            fprintf(stderr, "Unauthorized client attempted connection. Closing.\n");
            close(clientSocket);
            continue; // Skip to the next iteration if client is unauthorized
        }

        /*
         * Getting keySize
         */
        memset(dataBuffer, '\0', bufferSize); // Clear the data buffer
        bytesRead = recv(clientSocket, dataBuffer, BUFFER_CAPACITY - 1, 0); // Receive key size from client
        if (bytesRead < 0) perror("ERROR reading Key Size");
        bytesSent = send(clientSocket, acknowledgment, acknowledgmentSize, 0); // Send acknowledgment to client
        if (bytesSent < 0) perror("Error sending acknowledgment");

        int keySizeCapacity = 255; // Buffer size for key size
        char* keySize = malloc(sizeof(char) * (keySizeCapacity + 1)); // Allocate memory for key size
        size_t keySizeBuffer = sizeof(keySize); // Size of the key size buffer
        memset(keySize, '\0', keySizeBuffer); // Clear the key size buffer
        strcpy(keySize, dataBuffer); // Copy received key size to keySize variable

        /*
         * Getting key file path
         */
        memset(dataBuffer, '\0', bufferSize); // Clear the data buffer
        bytesRead = recv(clientSocket, dataBuffer, BUFFER_CAPACITY - 1, 0); // Receive key file path from client
        if (bytesRead < 0) fprintf(stderr, "ERROR reading Key File path\n");
        bytesSent = send(clientSocket, acknowledgment, 14, 0); // Send acknowledgment to client
        if (bytesSent < 0) perror("Error sending acknowledgment for key file\n");
        memset(keyFilePath, '\0', keyFileBufferSize); // Clear the key file path buffer
        strcpy(keyFilePath, dataBuffer); // Copy received key file path to keyFilePath variable

        /*
         * Getting plainText size
         */
        memset(dataBuffer, '\0', bufferSize); // Clear the data buffer
        bytesRead = recv(clientSocket, dataBuffer, BUFFER_CAPACITY - 1, 0); // Receive plaintext size from client
        if (bytesRead < 0) fprintf(stderr, "ERROR reading plain text size\n");
        bytesSent = send(clientSocket, acknowledgment, acknowledgmentSize, 0); // Send acknowledgment to client
        if (bytesSent < 0) perror("Error sending acknowledgment for plain text size\n");
        int plainTextSize = atoi(dataBuffer); // Convert received plaintext size to integer

        /*
         * Receiving plainText
         */
        int keySizeInt = atoi(keySize); // Convert key size to integer
        int remainingIterations = (plainTextSize / 100) + 1; // Calculate number of iterations needed to receive full plaintext
        int counter = 0; // Counter to track the number of characters received
        char* plainText = malloc(sizeof(char) * (keySizeInt + 1)); // Allocate memory for plaintext
        memset(plainText, '\0', keySizeInt + 1); // Clear the plaintext buffer

        while (remainingIterations != 0) {
            int numCharsToReceive = (remainingIterations == 1) ? (plainTextSize % 100) : 100; // Calculate number of characters to receive in this iteration
            memset(dataBuffer, '\0', bufferSize); // Clear the data buffer
            bytesRead = recv(clientSocket, dataBuffer, numCharsToReceive, 0); // Receive plaintext from client
            if (bytesRead < 0) fprintf(stderr, "Error reading plaintext\n");
            bytesSent = send(clientSocket, acknowledgment, acknowledgmentSize, 0); // Send acknowledgment to client
            if (bytesSent < 0) fprintf(stderr, "Error sending acknowledgment for plaintext\n");

            strcat(plainText, dataBuffer); // Append received plaintext to the plainText buffer
            counter += numCharsToReceive; // Update the counter
            remainingIterations--; // Decrement the remaining iterations
        }

        /*
         * Encrypting plainText
         */
        char* encryptionKey = malloc(sizeof(char) * (keySizeInt + 1)); // Allocate memory for encryption key
        memset(encryptionKey, '\0', keySizeInt + 1); // Clear the encryption key buffer
        FILE* fileHandler = fopen(keyFilePath, "r"); // Open the key file for reading
        if (fileHandler == NULL) {
            fprintf(stderr, "Failed to open keyFile: %s \n", keyFilePath);
            exit(EXIT_FAILURE);
        }

        char character;
        int index = 0;
        while ((character = fgetc(fileHandler)) != EOF && character != '\n' && index < keySizeInt) {
            encryptionKey[index++] = character; // Read the key from the file
        }
        fclose(fileHandler); // Close the key file

        char* encryptedMessage = malloc(sizeof(char) * (strlen(plainText) + 1)); // Allocate memory for encrypted message
        memset(encryptedMessage, '\0', strlen(plainText) + 1); // Clear the encrypted message buffer
        for (int i = 0; i < strlen(plainText); i++) {
            int plainCharIndex = locateCharacterIndex(plainText[i]); // Get index of plaintext character
            int keyCharIndex = locateCharacterIndex(encryptionKey[i]); // Get index of key character
            int newCharIndex = (plainCharIndex + keyCharIndex) % 27; // Calculate new character index using modulo 27
            encryptedMessage[i] = validCharacters[newCharIndex]; // Store the encrypted character
        }

        /*
         * Sending Encrypted Text
         */
        counter = 0; // Reset counter for sending encrypted text
        remainingIterations = (plainTextSize / 100) + 1; // Calculate number of iterations needed to send full encrypted text
        while (remainingIterations != 0) {
            int numCharsToSend = (remainingIterations == 1) ? (plainTextSize % 100) : 100; // Calculate number of characters to send in this iteration
            bytesSent = send(clientSocket, encryptedMessage + counter, numCharsToSend, 0); // Send encrypted text to client
            if (bytesSent < 0) fprintf(stderr, "ERROR writing encrypted text\n");
            memset(ackBuffer, '\0', BUFFER_CAPACITY); // Clear the acknowledgment buffer
            bytesRead = recv(clientSocket, ackBuffer, acknowledgmentSize, 0); // Receive acknowledgment from client
            if (bytesRead < 0) fprintf(stderr, "Error receiving acknowledgment while sending encrypted text\n");
            counter += numCharsToSend; // Update the counter
            remainingIterations--; // Decrement the remaining iterations
        }

        /*
         * Closing connection
         */
        close(clientSocket); // Close the client socket
    } // end of while loop
} // end of "main" function