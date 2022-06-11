#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMM_SIZE 6

/**
 * @brief Display the evaluation of an assignment
 *          - variable_name: left-side variable
 *          - assignment: right-side value
 *          - evaluating_value: false for non-quoted value and true for quoted value
 *          - valid: true if valid and false for invalid
 */

struct evaluation_factors {
    char *variable_name;
    char *assignment;
    bool evaluating_value;  
    bool valid;
};

bool is_print_command_valid(char *line)
{
    // NOTE: Need to handle single quote as invalid
    char *command = strndup(line, COMM_SIZE);

    if (!strcmp(command, "print ")) return true;
    return false;
}

char *retrieve_string_inside_quotation_mark(char *line)
{
    // Retrieve the string inside the quotation marks
    char *display_str = (char *)malloc (sizeof (char) * strlen(line));
    for (int i = (COMM_SIZE + 1); i < (strlen(line)-1); i++) {
        display_str[i-(COMM_SIZE + 1)] = line[i];
    }
    return display_str;
}

char *is_dollar_present(char *str)
{
    return strchr(str, '$');
}

char *extract_substring_of_dollar_string(char *str, char *var_str, char cond)
{
    int index = strcspn(str, "$");
    char *substring = (char *)malloc (sizeof (char) * strlen(str));

    if (cond == 'p') {
        for (int i = 0; i < index; i++) substring[i] = str[i];
    } else {
        for (int i = index+1+strlen(var_str); i < strlen(str); i++) substring[i] = str[i];
    }

    return substring;
}

// Display the result after processing and extraction
void display_assignment_result(struct evaluation_factors e, char *line) {
    // bool res = is_print_command_valid(line);
    // if (!res) {
    //     printf("%s\n", "Command is invalid!");
    //     return;
    // }

    char *display_str = retrieve_string_inside_quotation_mark(line);
    if (is_dollar_present(display_str)) {
        // Extract prefix (anything before $)
        char *prefix = extract_substring_of_dollar_string(display_str, "", 'p');
        // Extract suffix (anything after the assignment of $)
        char *suffix = extract_substring_of_dollar_string(display_str, e.variable_name, 's');
        // printf("%s\n", e.variable_name);

        printf("%s%s%s\n", prefix, e.assignment, suffix);
    } else   // Display string as usual
        printf("%s\n", display_str);
}
