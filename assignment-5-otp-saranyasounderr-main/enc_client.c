#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netdb.h>      

#define MAX_BUFFER_SIZE 1024

// Client identifier to distinguish between encryption and decryption clients
char* clientIdentifier = "1";

// Function to calculate the size of a file
long calculateFileSize(const char *fileName) {
    FILE *filePointer = fopen(fileName, "r"); // Open the file for reading
    if (filePointer == NULL) {
        fprintf(stderr, "Failed to open file %s\n", fileName);
        exit(EXIT_FAILURE);
    }
    fseek(filePointer, 0, SEEK_END); // Move the file pointer to the end of the file
    long size = ftell(filePointer); // Get the current position of the file pointer (which is the file size)
    fclose(filePointer); // Close the file
    return size - 1; // Return the file size minus 1 (to exclude the newline character)
}

// Function to handle errors and exit the program
void handleError(const char *errorMessage) {
    perror(errorMessage);
    exit(0);
}

// Function to configure the server address structure
void configureAddress(struct sockaddr_in* serverAddress, int port, char* hostname) {
    memset((char*) serverAddress, '\0', sizeof(*serverAddress)); // Clear the server address structure
    serverAddress->sin_family = AF_INET; // Set address family to IPv4
    serverAddress->sin_port = htons(port); // Set port number, converting to network byte order
    struct hostent* hostDetails = gethostbyname(hostname); // Get host details by hostname
    if (hostDetails == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the host's IP address to the server address structure
    memcpy((char*)&serverAddress->sin_addr.s_addr,
           hostDetails->h_addr_list[0],
           hostDetails->h_length);
}

int main(int argc, char *argv[]) {
    int connectionSocket, port, bytesSent, bytesReceived;
    struct sockaddr_in serverAddress; // Server address structure
    char dataBuffer[MAX_BUFFER_SIZE]; // Buffer for storing data
    char confirmationMessage[15] = "ACK: received."; // Confirmation message
    int confirmationSize = strlen(confirmationMessage); // Length of the confirmation message
    
    int ackBufferCapacity = 255; // Buffer size for acknowledgment
    char* ackBuffer = malloc(sizeof(char) * (ackBufferCapacity + 1)); // Allocate memory for acknowledgment buffer

    // Check if the correct number of arguments is provided
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s file key port\n", argv[0]);
        exit(0);
    }

    // Assign command-line arguments to variables
    char* inputFile = argv[1]; // File containing the plaintext
    char* encryptionKeyFile = argv[2]; // File containing the encryption key
    port = atoi(argv[3]); // Port number to connect to the server

    // Calculate the size of the encryption key file
    long encryptionKeySize = calculateFileSize(encryptionKeyFile);
    char* messageContent = malloc(sizeof(char) * (encryptionKeySize + 1)); // Allocate memory for the message content
    memset(messageContent, '\0', encryptionKeySize + 1); // Clear the message content buffer

    // Valid characters for the one-time pad encryption (A-Z and space)
    char validChars[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    // Open the input file for reading
    FILE* fileReader = fopen(inputFile, "r");
    if (fileReader == NULL) {
        fprintf(stderr, "File (%s) does not exist\n", inputFile);
        exit(EXIT_FAILURE);
    }

    /*
     * Reading input file content
     */
    char fileChar; // Variable to store each character read from the file
    int charCount = 0; // Counter to track the number of characters read

    while (true) {
        fileChar = fgetc(fileReader); // Read a character from the file
        if (fileChar == '\n' || fileChar == EOF) break; // Stop reading if newline or end of file is reached
        
        // Check if the character is valid (A-Z or space)
        bool isValidChar = false;
        for (int i = 0; i < 28; i++) {
            if (fileChar == validChars[i]) {
                isValidChar = true;
                break;
            }
        }

        // If the character is invalid, print an error and exit
        if (!isValidChar) {
            fprintf(stderr, "Invalid character in file %s: %c\n", inputFile, fileChar);
            exit(EXIT_FAILURE);
        }

        // Store the valid character in the message content buffer
        messageContent[charCount++] = fileChar;

        // Check if the plaintext exceeds the key size
        if (encryptionKeySize < charCount) {
            fprintf(stderr, "Plaintext exceeds key size.\n");
            exit(EXIT_FAILURE);
        }
    }
    fclose(fileReader); // Close the input file

    /*
     * Establishing connection with the server
     */
    connectionSocket = socket(AF_INET, SOCK_STREAM, 0); // Create a socket for the client
    if (connectionSocket < 0) {
        handleError("CLIENT: ERROR opening socket");
    }
    configureAddress(&serverAddress, port, "localhost"); // Configure the server address structure
    if (connect(connectionSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "CLIENT: ERROR connecting\n");
        return 2; // Exit with error code 2 if connection fails
    }

    /*
     * Sending client ID
     */
    char* serverIdentifierBuffer = malloc(sizeof(char) * 256); // Allocate memory for server identifier buffer
    memset(serverIdentifierBuffer, '\0', 256); // Clear the server identifier buffer
    bytesReceived = recv(connectionSocket, serverIdentifierBuffer, 1, 0); // Receive server identifier
    if (bytesReceived < 0) perror("Error reading server ID\n");
    
    bytesSent = send(connectionSocket, clientIdentifier, strlen(clientIdentifier), 0); // Send client identifier
    if (bytesSent < 0) perror("Error sending client ID\n");

    // Check if the server identifier matches the client identifier
    if (strcmp(serverIdentifierBuffer, clientIdentifier) != 0) {
        fprintf(stderr, "Unauthorized server connection attempt. Closing connection..\n");
        close(connectionSocket);
        return 2; // Exit with error code 2 if server is unauthorized
    }

    /*
     * Sending encryption key size
     */
    char encryptionKeySizeString[20]; // Buffer to store the encryption key size as a string
    sprintf(encryptionKeySizeString, "%ld", encryptionKeySize); // Convert the key size to a string
    
    bytesSent = send(connectionSocket, encryptionKeySizeString, sizeof(encryptionKeySizeString), 0); // Send key size
    bytesReceived = recv(connectionSocket, ackBuffer, confirmationSize, 0); // Receive acknowledgment

    /*
     * Sending encryption key file
     */
    bytesSent = send(connectionSocket, encryptionKeyFile, strlen(encryptionKeyFile), 0); // Send key file name
    bytesReceived = recv(connectionSocket, ackBuffer, confirmationSize, 0); // Receive acknowledgment

    /*
     * Sending plaintext message size
     */
    int actualMessageSize = strlen(messageContent); // Get the size of the plaintext message
    char messageSizeString[20]; // Buffer to store the message size as a string
    sprintf(messageSizeString, "%d", actualMessageSize); // Convert the message size to a string
    
    bytesSent = send(connectionSocket, messageSizeString, sizeof(messageSizeString), 0); // Send message size
    bytesReceived = recv(connectionSocket, ackBuffer, confirmationSize, 0); // Receive acknowledgment

    /*
     * Sending plaintext message
     */
    int sendIterations = (actualMessageSize / 100) + 1; // Calculate the number of iterations needed to send the message
    int sentChars = 0; // Counter to track the number of characters sent
    
    while (sendIterations != 0) {
        int numCharsToSend = (sendIterations == 1) ? (actualMessageSize % 100) : 100; // Calculate the number of characters to send in this iteration
        if (numCharsToSend == 0) numCharsToSend = 100; // If the remaining characters are 0, send 100 characters
        
        bytesSent = send(connectionSocket, messageContent + sentChars, numCharsToSend, 0); // Send the message chunk
        bytesReceived = recv(connectionSocket, ackBuffer, confirmationSize, 0); // Receive acknowledgment
        sentChars += numCharsToSend; // Update the counter
        sendIterations--; // Decrement the remaining iterations
    }

    /*
     * Receiving encrypted message
     */
    char* encryptedMessage = malloc(sizeof(char) * (actualMessageSize + 1)); // Allocate memory for the encrypted message
    memset(encryptedMessage, '\0', actualMessageSize + 1); // Clear the encrypted message buffer
    
    sendIterations = (actualMessageSize / 100) + 1; // Calculate the number of iterations needed to receive the message
    sentChars = 0; // Counter to track the number of characters received
    
    while (sendIterations != 0) {
        int numCharsToReceive = (sendIterations == 1) ? (actualMessageSize % 100) : 100; // Calculate the number of characters to receive in this iteration
        if (numCharsToReceive == 0) numCharsToReceive = 100; // If the remaining characters are 0, receive 100 characters

        bytesReceived = recv(connectionSocket, dataBuffer, numCharsToReceive, 0); // Receive the encrypted message chunk
        bytesSent = send(connectionSocket, confirmationMessage, confirmationSize, 0); // Send acknowledgment
        
        strcat(encryptedMessage, dataBuffer); // Append the received chunk to the encrypted message buffer
        sentChars += numCharsToReceive; // Update the counter
        sendIterations--; // Decrement the remaining iterations
    }

    // Print the encrypted message to standard output
    printf("%s\n", encryptedMessage);
    
    close(connectionSocket); // Close the connection socket
    
    return 0; // Exit the program successfully
}