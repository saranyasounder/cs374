// Name: Saranya Sounder Rajan

// Complete this file to write a C program that adheres to the assignment
// specification. Refer to the assignment document in Canvas for the
// requirements.

// You can, and probably SHOULD, use a lot of your work from assignment 1 to
// get started on this assignment. In particular, when you get to the part
// about processing the selected file, you'll need to load and parse all of the
// data from the selected CSV file into your program. In assignment 1, you
// created structure types, linked lists, and a bunch of code to facilitate
// doing exactly that. Just copy and paste that work into here as a starting
// point :)

// This assignment has no requirements about linked lists, so you can
// use a dynamic array (and expand it with realloc(), for example) instead if
// you would like. But then you'd have to redo a bunch of work.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>

//struct for movies
struct movie
{
    char *title;
    int *year;
    char *language;
    double *rating;
    struct movie *next;
};

//to free memory for movies
void freeMovies(struct movie *head)
{
    while (head)
    {
        struct movie *temp = head;
        head = head->next;
        free(temp->title);
        free(temp->language);
        free(temp->year);
        free(temp->rating);
        free(temp);
    }
}

//function to read a csv file
int readFile(char *filePath)
{
    if (!filePath){
        return -2;
    }
    FILE *file = fopen(filePath, "r");
    if (!file)
    {
        return -2;
    }
    printf("Now processing the chosen file named %s\n", filePath);

    int randomNum = rand() % 99999;
    char dirName[30];
    sprintf(dirName, "sounders.movies.%d", randomNum);  //creating new directory
    mkdir(dirName, 0750); // creating a directory
    printf("Created directory with name %s\n", dirName);

    char *line = NULL;
    size_t buffer_size = 0;
    struct movie *head = NULL, *tail = NULL;

    int trackYears[100] = {0};

    getline(&line, &buffer_size, file);

    while (getline(&line, &buffer_size, file) != -1)
    {
        struct movie *new_node = malloc(sizeof(struct movie));
        new_node->title = malloc(256);
        new_node->language = malloc(256);
        new_node->year = malloc(sizeof(int));
        new_node->rating = malloc(sizeof(double));
        new_node->next = NULL;

        char *token = strtok(line, ",");
        if (token)
            strcpy(new_node->title, token);

        token = strtok(NULL, ",");
        if (token)
            *(new_node->year) = atoi(token);

        token = strtok(NULL, ",");
        if (token)
            strcpy(new_node->language, token);

        token = strtok(NULL, ",");
        if (token)
            *(new_node->rating) = strtof(token, NULL);

        int year = *(new_node->year);
        trackYears[year % 100] = year;

        char newFile[50];
        sprintf(newFile, "%s/%d.txt", dirName, year);
        int fd = open(newFile, O_WRONLY | O_CREAT | O_APPEND, 0640); //setting it to rw-r----- , also will append new data instead of overwriting
        if (fd != -1)
        {
            dprintf(fd, "%s\n", new_node->title);
            close(fd);
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
    }

    free(line);
    fclose(file);
    freeMovies(head);
    return 0;
}

bool smallestFile()
{
    DIR *current = opendir("."); //opening the current directory
    if (!current){
        return false;
    }
    struct dirent *newFile; //new struct for directory
    struct stat fileInfo;  //new struct containing info about permissions
    int smallest = 99999999;
    char *smallestFile = NULL;

    while ((newFile = readdir(current)) != NULL)
    {
        char *checkFile = strrchr(newFile->d_name, '.');
        if (checkFile && strncmp("movies_", newFile->d_name, 7) == 0 && strcmp(".csv", checkFile) == 0)
        {
            stat(newFile->d_name, &fileInfo);
            if (fileInfo.st_size < smallest) //comparing byte size to find the smallest file
            {
                smallest = fileInfo.st_size;
                free(smallestFile);
                smallestFile = strdup(newFile->d_name);
            }
        }
    }
    closedir(current);

    if (smallestFile)
    {
        int flag = readFile(smallestFile);
        free(smallestFile);
        return flag != -2;
    }
    return false;
}

bool largestFile()
{
    DIR *current = opendir(".");
    if (!current)
        return false;

    struct dirent *newFile; //new struct for directory
    struct stat fileInfo;  //new struct containing info about permissions
    int largest = -1;
    char *largestFile = NULL;

    while ((newFile = readdir(current)) != NULL)
    {
        char *checkFile = strrchr(newFile->d_name, '.');
        if (checkFile && strncmp("movies_", newFile->d_name, 7) == 0 && strcmp(".csv", checkFile) == 0)
        {
            stat(newFile->d_name, &fileInfo);
            if (fileInfo.st_size > largest) //comapring byte size to find the largest file
            {
                largest = fileInfo.st_size;
                free(largestFile);
                largestFile = strdup(newFile->d_name);
            }
        }
    }
    closedir(current);

    if (largestFile)
    {
        int flag = readFile(largestFile);
        free(largestFile);
        return flag != -2;
    }
    return false;
}

void options(int option)
{
    if (option == 1)
    {
        largestFile();
    }
    else if (option == 2)
    {
        smallestFile();
    }
    else if (option == 3) 
    {
        int status = 0;
        char file[256];
        printf("\nEnter the complete file name: ");
        scanf("%s", file);
        int flag = readFile(file);
        if (flag == -2) //will have to display the 3 options instead of the main menu
        {
            printf("\nThe file %s was not found. Try again.\n", file);
            int newOption;
            printf("\nWhich file do you want to process?\n");
            printf("Enter 1 to pick the largest file\n");
            printf("Enter 2 to pick the smallest file\n");
            printf("Enter 3 to specify the name of a file\n");
            printf("Enter a choice from 1 to 3: ");
            scanf("%d", &newOption);
            options(newOption);
        }
        
    }
}


void displayOptions()
{
    int mainChoice;
    do
    {
        printf("\n1. Select file to process\n");
        printf("2. Exit the program\n");
        printf("Enter a choice 1 or 2: ");
        scanf("%d", &mainChoice);
        printf("\n");

        if (mainChoice == 1)
        {
            int option;
            printf("Which file do you want to process?\n");
            printf("Enter 1 to pick the largest file\n");
            printf("Enter 2 to pick the smallest file\n");
            printf("Enter 3 to specify the name of a file\n");
            printf("Enter a choice from 1 to 3: ");
            scanf("%d", &option);
            options(option);
        }
    } while (mainChoice != 2);
}

int main()
{
    displayOptions();
    return 0;
}
