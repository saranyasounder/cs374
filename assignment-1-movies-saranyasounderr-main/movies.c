// Name: Saranya Sounder Rajan

// Complete this file to write a C program that adheres to the assignment
// specification. Refer to the assignment document in Canvas for the
// requirements. Refer to the example-directory/ contents for some example
// code to get you started.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define a struct for representing a movie
struct movie
{
    char *title;
    int *year;
    char *language;
    double *rating;
    struct movie *next;
};

// Function to display movies released in a specified year
void yearReleased(struct movie *movieList)
{
    int yearInput;
    printf("Enter the year for which you want to see movies (1900-2021): ");
    scanf("%d", &yearInput);

    // Validate the year input
    while (yearInput < 1900 || yearInput > 2021)
    {
        printf("Invalid entry! Enter the year for which you want to see movies (1900-2021): ");
        scanf("%d", &yearInput);
    }
    printf("\n");
    struct movie *temp = movieList;
    int count = 0;

    // Traverse the linked list and display movies matching the year
    while (temp != NULL)
    {
        if (*(temp->year) == yearInput)
        {
            printf("%s\n", temp->title);
            count++;
        }
        temp = temp->next;
    }

    // Handle the case where no movies were found
    if (count == 0)
    {
        printf("No data about movies released in the year %d.\n", yearInput);
    }
}

// Function to display the highest-rated movie for each year
void highestRated(struct movie *movieList)
{
    int low = 1900;
    int high = 2021;

    while (low <= high)
    {
        struct movie *temp = movieList;
        struct movie *storeHigh = NULL;
        double highestRating = 0.0;

        // Traverse the list and find the highest-rated movie for the current year
        while (temp != NULL)
        {
            if (*(temp->year) == low)
            {
                if (*(temp->rating) > highestRating)
                {
                    storeHigh = temp;
                    highestRating = *(temp->rating);
                }
            }
            temp = temp->next;
        }

        // Print the highest-rated movie for the year
        if (storeHigh != NULL)
        {
            printf("%d %.1lf %s\n", *(storeHigh->year), *(storeHigh->rating), storeHigh->title);
        }

        low++;
    }
}

// Function to display movies released in a specified language
void languageSearch(struct movie *movieList)
{
    char lang[20];
    printf("Enter the language for which you want to see movies: ");
    scanf("%s", lang);
    struct movie *temp = movieList;
    int count = 0;
    printf("\n");

    // Traverse the list and display movies matching the language
    while (temp != NULL)
    {
        if (strstr(temp->language, lang) != NULL) // Check if 'lang' is a substring of 'temp->language'. strstr will return NULL if there isnt a match
        {
            printf("%d %s\n", *(temp->year), temp->title);
            count = 1;
        }
        temp = temp->next;
    }

    // Handle the case where no movies were found
    if (count == 0)
    {
        printf("No data about movies released in %s\n", lang);
    }
}

// Function to handle menu options and call the respective function
void options(int choice, struct movie *movieList)
{
    if (choice == 1)
    {
        yearReleased(movieList);
    }
    else if (choice == 2)
    {
        highestRated(movieList);
    }
    else if (choice == 3)
    {
        languageSearch(movieList);
    }
}

// Function to display menu options
void displayOptions(struct movie *movieList)
{
    int choice = 0;
    while (choice != 4)
    {
        printf("\n1. Show movies released in the specified year\n");
        printf("2. Show highest rated movie for each year\n");
        printf("3. Show the title and year of release of all movies in a specific language\n");
        printf("4. Exit from the program\n");
        printf("\nEnter your choice (1-4): ");
        scanf("%d", &choice);
        printf("\n");
        options(choice, movieList);
    }
}

// Function to free the memory allocated for the linked list
void freeMovies(struct movie *head)
{
    struct movie *temp;
    while (head)
    {
        temp = head;
        head = head->next;
        free(temp->title);
        free(temp->language);
        free(temp->year);
        free(temp->rating);
        free(temp);
    }
}

// Function to read movies from a file and create a linked list
struct movie *readFile(FILE *file, int *movie_count)
{
    char *line = NULL;
    size_t buffer_size = 0;
    ssize_t line_length;

    // Read and skip the header line
    getline(&line, &buffer_size, file);

    struct movie *head = NULL;
    struct movie *tail = NULL;
    *movie_count = 0;

    // Process each line in the file
    while ((line_length = getline(&line, &buffer_size, file)) != -1)
    {
        struct movie *new_node = malloc(sizeof(struct movie));
        new_node->title = malloc(256 * sizeof(char));
        new_node->language = malloc(256 * sizeof(char));
        new_node->year = malloc(sizeof(int));
        new_node->rating = malloc(sizeof(double));
        new_node->next = NULL;

        char *token = strtok(line, ",");
        if (token)
        {
            strcpy(new_node->title, token);
        }

        token = strtok(NULL, ",");
        if (token)
        {
            *(new_node->year) = atoi(token);
        }

        token = strtok(NULL, ",");
        if (token)
        {
            strcpy(new_node->language, token);
        }

        token = strtok(NULL, ",");
        if (token)
        {
            *(new_node->rating) = strtof(token, NULL);
        }

        if (!head)
        {
            head = new_node;
            tail = new_node;
        }
        else
        {
            tail->next = new_node;
            tail = new_node;
        }

        (*movie_count)++;
    }

    free(line);
    return head;
}

// Main function
int main(int argc, char **argv)
{
    // Open the input file
    FILE *data_file = fopen(argv[1], "r");
    if (!data_file)
    {
        printf("Error opening file %s\n", argv[1]);
        return 1;
    }

    int movie_count = 0;
    struct movie *movieList = readFile(data_file, &movie_count);
    fclose(data_file);

    printf("Processed file %s and parsed data for %d movies\n", argv[1], movie_count);

    // Display menu and process user choices
    displayOptions(movieList);

    // Free allocated memory
    freeMovies(movieList);
    return 0;
}
