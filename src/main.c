#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Function prototypes
int count_columns(const char *line);
void calculate_column_widths(char ***data, int rows, int cols, int *col_widths);
char ***read_csv(const char *filename, int *rows, int *cols);
void display_csv(char ***data, int rows, int cols);
void display_heatmap(char ***data, int rows, int cols);
void free_csv(char ***data, int rows, int cols);
void hsv_to_rgb(int h, int s, int v, int *r, int *g, int *b);
int find_class_column(char ***data, int cols);

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

// Function to convert HSV to RGB for ANSI colors
void hsv_to_rgb(int h, int s, int v, int *r, int *g, int *b) {
    float hf = h / 360.0f;
    float sf = s / 100.0f;
    float vf = v / 100.0f;

    float c = vf * sf;
    float x = c * (1 - fabs(fmod(hf * 6, 2) - 1));
    float m = vf - c;

    float rf = 0, gf = 0, bf = 0;

    if (hf < 1.0 / 6.0) {
        rf = c; gf = x; bf = 0;
    } else if (hf < 2.0 / 6.0) {
        rf = x; gf = c; bf = 0;
    } else if (hf < 3.0 / 6.0) {
        rf = 0; gf = c; bf = x;
    } else if (hf < 4.0 / 6.0) {
        rf = 0; gf = x; bf = c;
    } else if (hf < 5.0 / 6.0) {
        rf = x; gf = 0; bf = c;
    } else {
        rf = c; gf = 0; bf = x;
    }

    *r = (int)((rf + m) * 255);
    *g = (int)((gf + m) * 255);
    *b = (int)((bf + m) * 255);
}

// Function to find the index of the 'class' column (case-insensitive)
int find_class_column(char ***data, int cols) {
    for (int j = 0; j < cols; j++) {
        if (strcasecmp(data[0][j], "class") == 0) { // Compare case-insensitively
            return j;
        }
    }
    return -1; // Return -1 if 'class' column is not found
}

// Function to display the heatmap
void display_heatmap(char ***data, int rows, int cols) {
    printf("\nHeatmap View:\n");

    // Calculate maximum column widths
    int *col_widths = (int *)malloc(cols * sizeof(int));
    if (!col_widths) {
        printf("Memory allocation failed\n");
        return;
    }
    calculate_column_widths(data, rows, cols, col_widths);

    // Find the 'class' column index
    int class_col = find_class_column(data, cols);
    if (class_col == -1) {
        printf("Error: 'class' column not found.\n");
        free(col_widths);
        return;
    }

    // Map unique class values to colors
    char **classes = malloc(rows * sizeof(char *));
    int class_count = 0;

    // print header for all columns
    for (int j = 0; j < cols; j++) {
        printf("%-*s ", col_widths[j] + 1, data[0][j]);
    }
    printf("\n");

    for (int i = 1; i < rows; i++) { // Start at 1 to skip header
        int found = 0;
        for (int j = 0; j < class_count; j++) {
            if (strcasecmp(classes[j], data[i][class_col]) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            classes[class_count] = data[i][class_col];
            class_count++;
        }
    }

    // Dynamically allocate memory for class_colors
    int **class_colors = (int **)malloc(class_count * sizeof(int *));
    for (int i = 0; i < class_count; i++) {
        class_colors[i] = (int *)malloc(3 * sizeof(int));
    }

    // Assign HSV colors to each class
    for (int i = 0; i < class_count; i++) {
        hsv_to_rgb((i * 360 / class_count), 100, 100, &class_colors[i][0], &class_colors[i][1], &class_colors[i][2]);
    }

    // Iterate over the rows and columns
    for (int i = 1; i < rows; i++) { // Skip the header row
        for (int j = 0; j < cols; j++) {
            if (j == class_col) continue; // Skip the class column for now
            double value = atof(data[i][j]); // Convert to a numeric value
            int intensity = (int)(value * 255 / 10); // Scale to 0-255 (assuming max value ~10)
            intensity = intensity > 255 ? 255 : (intensity < 0 ? 0 : intensity);

            // Use ANSI escape codes for color and align dynamically
            printf("\033[48;2;%d;%d;%dm %-*s \033[0m", 
                   intensity, 0, 255 - intensity, col_widths[j], data[i][j]);
        }

        // Display class with color
        for (int j = 0; j < class_count; j++) {
            if (strcasecmp(classes[j], data[i][class_col]) == 0) {
                printf("\033[38;2;%d;%d;%dm %-*s \033[0m", 
                       class_colors[j][0], class_colors[j][1], class_colors[j][2], col_widths[class_col], data[i][class_col]);
                break;
            }
        }
        printf("\n");
    }

    // Free dynamically allocated memory for class_colors
    for (int i = 0; i < class_count; i++) {
        free(class_colors[i]);
    }
    free(class_colors);
    free(classes);
    free(col_widths);
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
