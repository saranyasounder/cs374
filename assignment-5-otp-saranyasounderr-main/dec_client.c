#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

// Client identifier to distinguish between encryption and decryption clients
char* clientIdentifier = "2";

// Function to get the size of a file
long getFileSize(const char *filename) {
    FILE *file = fopen(filename, "r");  // Open the file for reading
    if (file == NULL) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END); // Move the file pointer to the end of the file
    long size = ftell(file);  // Get the current position of the file pointer (which is the file size)
    fclose(file); // Close the file
    return size - 1;  // Return the file size minus 1 (to exclude the newline character)
}

// Function to handle errors and exit the program
void displayError(const char *msg) {
    perror(msg);
    exit(0);
}

// Function to configure the server address structure
void configureAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname){
    memset((char*) address, '\0', sizeof(*address)); // Clear the address structure
    address->sin_family = AF_INET; // Set address family to IPv4
    address->sin_port = htons(portNumber); // Set port number, converting to network byte order
    struct hostent* hostInfo = gethostbyname(hostname); // Get host details by hostname
    if (hostInfo == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the host's IP address to the server address structure
    memcpy((char*) &address->sin_addr.s_addr,
           hostInfo->h_addr_list[0],
           hostInfo->h_length);
}

int main(int argc, char *argv[]) {
    int clientSocket, portNumber, bytesSent, bytesReceived; // Variables for socket communication
    struct sockaddr_in serverAddress; // Server address structure
    char buffer[BUFFER_SIZE]; // Buffer for storing data
    char acknowledgmentMessage[15] = "ACK: received."; // Acknowledgment message
    int acknowledgmentSize = strlen(acknowledgmentMessage); // Length of the acknowledgment message
    char* acknowledgmentBuffer = malloc(sizeof(char) * (BUFFER_SIZE)); // Allocate memory for acknowledgment buffer

    // Check if the correct number of arguments is provided
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s file key port\n", argv[0]);
        exit(0);
    }

    /*
    * Initializing input parameters
    */
    char* inputFile = argv[1]; // File containing the ciphertext
    char* keyFile = argv[2]; // File containing the decryption key
    portNumber = atoi(argv[3]); // Port number to connect to the server

    // Calculate the size of the key file
    long keySize = getFileSize(keyFile);
    char* cipherTextMessage = malloc(sizeof(char) * (keySize + 1));  // Allocate memory for ciphertext message
    memset(cipherTextMessage, '\0', keySize + 1); // Clear the ciphertext message buffer

    // Valid characters for the one-time pad encryption (A-Z and space)
    char allowedCharacters[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    // Open the input file for reading
    FILE* inputFileHandler = fopen(inputFile, "r");
    if (inputFileHandler == NULL) {
        fprintf(stderr, "File (%s) does not exist\n", inputFile);
        exit(EXIT_FAILURE);
    }

    /*
     * Copying file contents into cipherTextMessage variable
     */
    char fileCharacter; // Variable to store each character read from the file
    int characterCount = 0; // Counter to track the number of characters read
    char invalidCharacter; // Variable to store invalid characters for error reporting

    while (true) {
        fileCharacter = fgetc(inputFileHandler); // Read a character from the file
        if (fileCharacter == '\n' || fileCharacter == EOF) break; // Stop reading if newline or end of file is reached
        
        // Check if the character is valid (A-Z or space)
        bool isValidCharacter = false; 
        for (int i = 0; i < 28; i++) {
            if (fileCharacter == allowedCharacters[i]) { 
                isValidCharacter = true;
                break;
            }
            invalidCharacter = fileCharacter; // Store the invalid character for error reporting
        } 
        if (!isValidCharacter) { 
            fprintf(stderr, "Error: Invalid character in file %s. Character: %c\n", inputFile, invalidCharacter);
            exit(EXIT_FAILURE);
        }
        cipherTextMessage[characterCount++] = fileCharacter;  // Store the valid character in the ciphertext buffer

        // Check if the ciphertext exceeds the key size
        if (keySize < characterCount) { 
            fprintf(stderr, "Error: Cipher text larger than key size.\n");
            exit(EXIT_FAILURE);
        }
    } 
    fclose(inputFileHandler); // Close the input file

    /*
     * Establishing connection with the server
     */
    clientSocket = socket(AF_INET, SOCK_STREAM, 0); // Create a socket for the client
    if (clientSocket < 0) displayError("CLIENT: ERROR opening socket");
    configureAddressStruct(&serverAddress, portNumber, "localhost"); // Configure the server address structure
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "DEC CLIENT: ERROR connecting\n");
        return 2; // Exit with error code 2 if connection fails
    }

    /*
     * Sending Client ID
     */
    char* serverIdentifier = malloc(sizeof(char) * 256); // Allocate memory for server identifier buffer
    memset(serverIdentifier, '\0', 256); // Clear the server identifier buffer
    bytesReceived = recv(clientSocket, serverIdentifier, 1, 0); // Receive server identifier
    if (bytesReceived < 0) perror("Error reading client ID\n");
    
    bytesSent = send(clientSocket, clientIdentifier, strlen(clientIdentifier), 0); // Send client identifier
    if (bytesSent < 0) perror("Error writing client ID\n");
    
    // Check if the server identifier matches the client identifier
    if (strcmp(serverIdentifier, clientIdentifier) != 0) { 
        perror("Client attempted to access unauthorized server. Closing connection...\n");
        close(clientSocket); // Close the connection if server is unauthorized
        return 2; // Exit with error code 2
    }

    /*
     * Sending key size
     */
    memset(buffer, '\0', BUFFER_SIZE); // Clear the buffer
    char keySizeString[20]; // Buffer to store the key size as a string
    sprintf(keySizeString, "%ld", keySize); // Convert the key size to a string
    
    bytesSent = send(clientSocket, keySizeString, sizeof(keySizeString), 0); // Send key size to server
    bytesReceived = recv(clientSocket, acknowledgmentBuffer, acknowledgmentSize, 0); // Receive acknowledgment from server

    /*
     * Sending key file name
     */
    bytesSent = send(clientSocket, keyFile, strlen(keyFile), 0);  // Send key file name to server
    bytesReceived = recv(clientSocket, acknowledgmentBuffer, acknowledgmentSize, 0); // Receive acknowledgment from server

    /*
     * Sending cipher text message size
     */
    int cipherTextSize = strlen(cipherTextMessage); // Get the size of the ciphertext message
    char cipherTextMessageSize[20]; // Buffer to store the ciphertext size as a string
    sprintf(cipherTextMessageSize, "%d", cipherTextSize); // Convert the ciphertext size to a string
    
    bytesSent = send(clientSocket, cipherTextMessageSize, sizeof(cipherTextMessageSize), 0); // Send ciphertext size to server
    bytesReceived = recv(clientSocket, acknowledgmentBuffer, acknowledgmentSize, 0); // Receive acknowledgment from server

    /*
     * Sending cipher text message
     */
    int sendIterations = (cipherTextSize / 100) + 1; // Calculate the number of iterations needed to send the message
    int sentCharacters = 0; // Counter to track the number of characters sent
    
    while (sendIterations != 0) { 
        int numCharsToSend = (sendIterations == 1) ? (cipherTextSize % 100) : 100; // Calculate the number of characters to send in this iteration
        if (numCharsToSend == 0) numCharsToSend = 100; // If the remaining characters are 0, send 100 characters

        bytesSent = send(clientSocket, cipherTextMessage + sentCharacters, numCharsToSend, 0); // Send the ciphertext chunk
        bytesReceived = recv(clientSocket, acknowledgmentBuffer, acknowledgmentSize, 0); // Receive acknowledgment from server

        sentCharacters += numCharsToSend; // Update the counter
        sendIterations--; // Decrement the remaining iterations
    }

    /*
     * Receiving decrypted text
     */
    char* decryptedText = malloc(sizeof(char) * (cipherTextSize + 1)); // Allocate memory for decrypted text
    memset(decryptedText, '\0', cipherTextSize + 1); // Clear the decrypted text buffer
    
    sendIterations = (cipherTextSize / 100) + 1; // Calculate the number of iterations needed to receive the message
    sentCharacters = 0; // Counter to track the number of characters received
    
    while (sendIterations != 0) {  
        int numCharsToReceive = (sendIterations == 1) ? (cipherTextSize % 100) : 100; // Calculate the number of characters to receive in this iteration
        if (numCharsToReceive == 0) numCharsToReceive = 100; // If the remaining characters are 0, receive 100 characters

        bytesReceived = recv(clientSocket, buffer, numCharsToReceive, 0); // Receive the decrypted text chunk
        bytesSent = send(clientSocket, acknowledgmentMessage, acknowledgmentSize, 0); // Send acknowledgment to server
        
        strcat(decryptedText, buffer); // Append the received chunk to the decrypted text buffer
        sentCharacters += numCharsToReceive; // Update the counter
        sendIterations--; // Decrement the remaining iterations
    }

    // Print the decrypted text to standard output
    printf("%s\n", decryptedText);
    
    close(clientSocket); // Close the client socket

    return 0; // Exit the program successfully
}