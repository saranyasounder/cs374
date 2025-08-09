#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv) {
    srand(time(NULL)); // Initialize the random number generator using the current time as the seed

    char availableChars[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "; // Define a set of characters that can be used in the key (A-Z and space)

    // Ensure that the user provides exactly one argument
    if (argc == 2) { 
        int lengthOfKey = atoi(argv[1]); // Convert the provided argument to an integer to determine the key length
        
        char generatedKey[lengthOfKey + 1]; // Create an array to store the generated key, plus one extra space for the null terminator
        
        for (int i = 0; i < lengthOfKey; i++) {
            generatedKey[i] = availableChars[rand() % 27]; // Randomly select a character from the available set
        }

        generatedKey[lengthOfKey] = '\0'; // Append a null terminator to make it a valid string

        printf("%s\n", generatedKey); // Print the generated key followed by a newline
    } 
    else {
        fprintf(stderr, "Invalid input. Please provide the key length as an argument.\n"); // Display an error message in case of incorrect input
        return 1; // Exit with an error code
    }

    return 0; // Successfully exit the program
} // End of main function
