// To compile and run: make run

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function to count columns in a single line
int count_columns(const char *line) {
    int count = 0;
    const char *temp = line;
    while (*temp) {
        if (*temp == ',') count++;
        temp++;
    }
    return count + 1; // Add 1 for the last column
}

// Function to find the maximum width of each column
void calculate_column_widths(char ***data, int rows, int cols, int *col_widths) {
    for (int j = 0; j < cols; j++) {
        col_widths[j] = 0; // Initialize column width
        for (int i = 0; i < rows; i++) {
            int len = strlen(data[i][j]);
            if (len > col_widths[j]) {
                col_widths[j] = len;
            }
        }
    }
}

// Function to read the CSV file and load data into memory
char ***read_csv(const char *filename, int *rows, int *cols) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Could not open file %s\n", filename);
        return NULL;
    }

    // Determine the number of rows and columns
    char line[1024];
    *rows = 0;
    *cols = 0;
    while (fgets(line, sizeof(line), file)) {
        if (*rows == 0) {
            *cols = count_columns(line);
        }
        (*rows)++;
    }

    rewind(file); // Reset file pointer to the beginning

    // Allocate memory for rows
    char ***data = (char ***)malloc(*rows * sizeof(char **));
    if (!data) {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read and parse each line
    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        data[i] = (char **)malloc(*cols * sizeof(char *));
        if (!data[i]) {
            printf("Memory allocation failed\n");
            fclose(file);
            return NULL;
        }

        char *token = strtok(line, ",\n");
        int j = 0;
        while (token != NULL) {
            data[i][j] = (char *)malloc((strlen(token) + 1) * sizeof(char));
            if (!data[i][j]) {
                printf("Memory allocation failed\n");
                fclose(file);
                return NULL;
            }
            strcpy(data[i][j], token);
            token = strtok(NULL, ",\n");
            j++;
        }
        i++;
    }

    fclose(file);
    return data;
}

// Function to display the CSV data
void display_csv(char ***data, int rows, int cols) {
    int *col_widths = (int *)malloc(cols * sizeof(int));
    if (!col_widths) {
        printf("Memory allocation failed\n");
        return;
    }

    // Calculate the maximum width of each column
    calculate_column_widths(data, rows, cols, col_widths);

    // Print the data with proper alignment
    printf("\nLoaded CSV Data:\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%-*s ", col_widths[j] + 1, data[i][j]);
        }
        printf("\n");
    }

    free(col_widths);
}

// Function to display the data as a heatmap
void display_heatmap(char ***data, int rows, int cols) {
    printf("\nHeatmap View:\n");

    // Iterate over the rows and columns
    for (int i = 1; i < rows; i++) { // Skip the header row
        for (int j = 0; j < cols - 1; j++) { // Skip the last column if it's non-numeric
            double value = atof(data[i][j]); // Convert to a numeric value
            int intensity = (int)(value * 255 / 10); // Scale to 0-255 (assuming max value ~10)
            intensity = intensity > 255 ? 255 : (intensity < 0 ? 0 : intensity);

            // Use ANSI escape codes for color
            printf("\033[48;2;%d;%d;%dm %6.2f \033[0m", intensity, 0, 255 - intensity, value);
        }
        printf("\n");
    }
}

// Function to free allocated memory
void free_csv(char ***data, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            free(data[i][j]);
        }
        free(data[i]);
    }
    free(data);
}

// Main function
int main() {
    char filename[100];
    int rows = 0, cols = 0;
    char ***data = NULL;

    printf("Enter the filename to open: ");
    scanf("%s", filename);

    // Load CSV data
    data = read_csv(filename, &rows, &cols);
    if (data == NULL) {
        return 1; // Exit if file could not be loaded
    }

    // Main loop for user input
    char choice;
    do {
        printf("\nOptions:\n");
        printf("'d': Display Data\n");
        printf("'h': Heatmap View\n");
        printf("'q': Quit\n");
        printf("Enter your choice: ");
        scanf(" %c", &choice); // Read user input

        switch (tolower(choice)) {
            case 'd':
                display_csv(data, rows, cols);
                break;
            case 'h':
                display_heatmap(data, rows, cols);
                break;
            case 'q':
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid option. Try again.\n");
                break;
        }
    } while (tolower(choice) != 'q');

    // Free memory
    free_csv(data, rows, cols);

    return 0;
}
