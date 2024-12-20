#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>

// Function prototypes
int count_columns(const char *line);
void calculate_column_widths(char ***data, int rows, int cols, int *col_widths);
char ***read_csv(const char *filename, int *rows, int *cols);
void display_csv(char ***data, int rows, int cols);
void display_heatmap(char ***data, int rows, int cols);
void free_csv(char ***data, int rows, int cols);
void hsv_to_rgb(int h, int s, int v, int *r, int *g, int *b);
int find_class_column(char ***data, int cols);
int list_csv_files(const char *folder, char files[][256], int max_files);
double **calculate_min_max(char ***data, int rows, int cols, int class_col);
void normalize_data(char ***data, char ****original_data, int rows, int cols, int class_col);
void denormalize_data(char ***data, char ***original_data, int rows, int cols);
void print_data_summary(char ***data, int rows, int cols, int class_col);
void free_original_data(char ***original_data, int rows, int cols);

void free_original_data(char ***original_data, int rows, int cols) {
    if (!original_data) return;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            free(original_data[i][j]);
        }
        free(original_data[i]);
    }
    free(original_data);
}

void print_data_summary(char ***data, int rows, int cols, int class_col) {
    printf("\n=== Data Summary ===\n");

    // Case count
    int case_count = rows - 1; // Exclude header
    printf("Case Count: %d\n", case_count);

    // Class count and names
    if (class_col != -1) {
        char **unique_classes = malloc(rows * sizeof(char *));
        int class_count = 0;

        for (int i = 1; i < rows; i++) { // Start at 1 to skip header
            int found = 0;
            for (int j = 0; j < class_count; j++) {
                if (strcasecmp(data[i][class_col], unique_classes[j]) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                unique_classes[class_count] = data[i][class_col];
                class_count++;
            }
        }

        printf("Class Count: %d\n", class_count);
        printf("Class Names: ");
        for (int i = 0; i < class_count; i++) {
            printf("%s", unique_classes[i]);
            if (i < class_count - 1) printf(", ");
        }
        printf("\n");

        free(unique_classes);
    } else {
        printf("Class Count: Not Applicable (No Class Column)\n");
    }

    // Attribute count
    int attribute_count = (class_col != -1) ? cols - 1 : cols;
    printf("Attribute Count: %d\n", attribute_count);

    // Attribute names
    printf("Attribute Names: ");
    for (int j = 0; j < cols; j++) {
        if (j == class_col) continue; // Skip class column
        printf("%s", data[0][j]); // Header row
        if (j < cols - 1 && !(j == class_col - 1)) printf(", ");
    }
    printf("\n");
    printf("====================\n");
}

// Function to calculate min and max for each column
double **calculate_min_max(char ***data, int rows, int cols, int class_col) {
    double **min_max = (double **)malloc(cols * sizeof(double *));
    for (int j = 0; j < cols; j++) {
        min_max[j] = (double *)malloc(2 * sizeof(double)); // [min, max]
        if (j == class_col) {
            min_max[j][0] = min_max[j][1] = NAN; // Skip class column
        } else {
            min_max[j][0] = INFINITY;
            min_max[j][1] = -INFINITY;
            for (int i = 1; i < rows; i++) { // Skip header row
                double value = atof(data[i][j]);
                if (value < min_max[j][0]) min_max[j][0] = value;
                if (value > min_max[j][1]) min_max[j][1] = value;
            }
        }
    }
    return min_max;
}

// Function to list all CSV files in the "data" folder
int list_csv_files(const char *folder, char files[][256], int max_files) {
    DIR *dir = opendir(folder);
    if (!dir) {
        printf("Could not open directory: %s\n", folder);
        return 0;
    }

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < max_files) {
        if (strstr(entry->d_name, ".csv")) { // Check if file ends with ".csv"
            strncpy(files[count], entry->d_name, 256);
            count++;
        }
    }

    closedir(dir);
    return count; // Return the number of CSV files found
}

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
    int has_class = class_col != -1; // Check if the class column exists

    // Compute min and max for all columns (excluding class column)
    double *col_min = (double *)malloc(cols * sizeof(double));
    double *col_max = (double *)malloc(cols * sizeof(double));
    for (int j = 0; j < cols; j++) {
        if (j == class_col) {
            col_min[j] = col_max[j] = NAN; // Skip class column
        } else {
            col_min[j] = INFINITY;
            col_max[j] = -INFINITY;
            for (int i = 1; i < rows; i++) { // Skip header
                double value = atof(data[i][j]);
                if (value < col_min[j]) col_min[j] = value;
                if (value > col_max[j]) col_max[j] = value;
            }
        }
    }

    // Map unique class values to colors
    char **classes = has_class ? malloc(rows * sizeof(char *)) : NULL;
    int class_count = 0;

    // Print header for all columns
    for (int j = 0; j < cols; j++) {
        if (j == class_col) continue; // Skip class column if present
        printf("%-*s ", col_widths[j] + 1, data[0][j]);
    }
    if (has_class) { // Print the class header if it exists
        printf("%-*s ", col_widths[class_col] + 1, data[0][class_col]);
    }
    printf("\n");

    // Process unique class values only if class column exists
    if (has_class) {
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
    }

    // Dynamically allocate memory for class_colors
    int **class_colors = NULL;
    if (has_class) {
        class_colors = (int **)malloc(class_count * sizeof(int *));
        for (int i = 0; i < class_count; i++) {
            class_colors[i] = (int *)malloc(3 * sizeof(int));
        }

        // Assign HSV colors to each class
        for (int i = 0; i < class_count; i++) {
            hsv_to_rgb((i * 360 / class_count), 100, 100, &class_colors[i][0], &class_colors[i][1], &class_colors[i][2]);
        }
    }

    // Iterate over the rows and columns
    for (int i = 1; i < rows; i++) { // Skip the header row
        for (int j = 0; j < cols; j++) {
            if (j == class_col) continue; // Skip class column

            double value = atof(data[i][j]); // Convert to a numeric value
            int intensity = 0;
            if (col_min[j] != col_max[j]) { // Avoid division by zero
                intensity = (int)((value - col_min[j]) / (col_max[j] - col_min[j]) * 255);
            }
            intensity = intensity > 255 ? 255 : (intensity < 0 ? 0 : intensity);

            // Use ANSI escape codes for color and align dynamically
            printf("\033[48;2;%d;%d;%dm %-*s \033[0m", 
                   intensity, 0, 255 - intensity, col_widths[j], data[i][j]);
        }

        // Display class with color if it exists
        if (has_class) {
            for (int j = 0; j < class_count; j++) {
                if (strcasecmp(classes[j], data[i][class_col]) == 0) {
                    printf("\033[38;2;%d;%d;%dm %-*s \033[0m", 
                           class_colors[j][0], class_colors[j][1], class_colors[j][2], col_widths[class_col], data[i][class_col]);
                    break;
                }
            }
        }
        printf("\n");
    }

    // Free dynamically allocated memory for class_colors and classes if applicable
    if (has_class) {
        for (int i = 0; i < class_count; i++) {
            free(class_colors[i]);
        }
        free(class_colors);
        free(classes);
    }
    free(col_min);
    free(col_max);
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

void denormalize_data(char ***data, char ***original_data, int rows, int cols) {
    if (!original_data) {
        printf("Error: Original data not backed up. Cannot denormalize.\n");
        return;
    }

    // Restore data from backup
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            free(data[i][j]);
            data[i][j] = strdup(original_data[i][j]);
        }
    }

    printf("Data restored to original values.\n");
}

void normalize_data(char ***data, char ****original_data, int rows, int cols, int class_col) {
    // Backup original data if not already backed up
    if (!*original_data) {
        *original_data = (char ***)malloc(rows * sizeof(char **));
        for (int i = 0; i < rows; i++) {
            (*original_data)[i] = (char **)malloc(cols * sizeof(char *));
            for (int j = 0; j < cols; j++) {
                (*original_data)[i][j] = strdup(data[i][j]);
            }
        }
    }

    double **min_max = calculate_min_max(data, rows, cols, class_col);
    if (!min_max) {
        printf("Error: Unable to calculate min/max for normalization.\n");
        return;
    }

    char temp[32]; // Temporary buffer for normalized values
    for (int j = 0; j < cols; j++) {
        if (j == class_col) continue; // Skip class column

        for (int i = 1; i < rows; i++) { // Skip the header row
            if (!data[i][j] || strlen(data[i][j]) == 0) continue;

            double value = atof(data[i][j]);
            if (isnan(value)) {
                printf("Warning: Non-numeric value in column %d, row %d. Skipping normalization.\n", j, i);
                continue;
            }

            if (min_max[j][0] != min_max[j][1]) {
                double normalized = (value - min_max[j][0]) / (min_max[j][1] - min_max[j][0]);
                snprintf(temp, sizeof(temp), "%.6f", normalized);

                // Safely reallocate memory for normalized value
                free(data[i][j]); // Free old value
                data[i][j] = strdup(temp); // Allocate new value
                if (!data[i][j]) {
                    printf("Error: Memory allocation failed for normalized value.\n");
                    break;
                }
            } else {
                // Handle case where min == max
                free(data[i][j]);
                data[i][j] = strdup("0.000000");
                if (!data[i][j]) {
                    printf("Error: Memory allocation failed for zero value.\n");
                    break;
                }
            }
        }
    }

    // Free min/max memory
    for (int j = 0; j < cols; j++) {
        free(min_max[j]);
    }
    free(min_max);

    printf("Data normalization completed.\n");
}

// Main function
int main() {
    char folder[] = "data"; // Directory containing CSV files
    char files[100][256];   // Array to store up to 100 filenames
    char filepath[300];     // Path to the currently loaded file
    int rows = 0, cols = 0;
    char ***original_data = NULL; // Local backup pointer
    char ***data = NULL;
    int normalize = 0; // Flag to track normalization mode (0 = OFF, 1 = ON)
    int class_col = -1; // Store class column index

    // Load available CSV files
    int file_count = list_csv_files(folder, files, 100);
    if (file_count == 0) {
        printf("No CSV files found in the folder '%s'. Exiting...\n", folder);
        return 1;
    }

    char option;
    do {
        // If no data is loaded, prompt user to select a file
        if (data == NULL) {
            printf("\nAvailable CSV files:\n");
            for (int i = 0; i < file_count; i++) {
                printf("[%d] %s\n", i + 1, files[i]);
            }

            int choice;
            printf("Enter the number of the file to open: ");
            scanf("%d", &choice);

            if (choice < 1 || choice > file_count) {
                printf("Invalid choice. Exiting...\n");
                return 1;
            }

            // Construct the full file path
            snprintf(filepath, sizeof(filepath), "%s/%s", folder, files[choice - 1]);

            // Load CSV data
            data = read_csv(filepath, &rows, &cols);
            if (data == NULL) {
                printf("Error loading the selected file. Returning to menu...\n");
                continue;
            }

            // After loading the data
            printf("File '%s' successfully loaded.\n", files[choice - 1]);

            // Find class column
            class_col = find_class_column(data, cols);
            if (class_col == -1) {
                printf("Warning: 'class' column not found. Proceeding without class-specific logic.\n");
            }

            // Print data summary
            print_data_summary(data, rows, cols, class_col);
        }

        // Display menu options
        printf("\nOptions:\n");
        printf("'d': Display Data\n");
        printf("'h': Heatmap View\n");
        printf("'r': Reload Data\n");
        printf("'n': Toggle Normalization (%s)\n", normalize ? "ON" : "OFF");
        printf("'q': Quit\n");
        printf("Enter your choice: ");
        scanf(" %c", &option);

        switch (tolower(option)) {
            case 'd': {
                display_csv(data, rows, cols);
                break;
            }
            case 'h': {
                display_heatmap(data, rows, cols);
                break;
            }
            case 'r': {
                free_original_data(original_data, rows, cols);
                original_data = NULL;
                free_csv(data, rows, cols);
                data = NULL;
                rows = cols = 0;
                normalize = 0; // Reset normalization flag
                printf("Reloading data...\n");
                break;
            }
            case 'n': {
                if (!data) {
                    printf("No data loaded to normalize/denormalize.\n");
                    break;
                }

                normalize = !normalize; // Toggle normalization mode
                if (normalize) {
                    printf("Normalizing data...\n");
                    normalize_data(data, &original_data, rows, cols, class_col);
                } else {
                    printf("Restoring original data...\n");
                    denormalize_data(data, original_data, rows, cols);
                }
                break;
            }
            case 'q': {
                printf("Exiting...\n");
                break;
            }
            default: {
                printf("Invalid option. Try again.\n");
                break;
            }
        }
    } while (tolower(option) != 'q');

    // Free memory if data is loaded
    if (data != NULL) free_csv(data, rows, cols);
    if (original_data) free_original_data(original_data, rows, cols);

    return 0;
}
