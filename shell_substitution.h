#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMM_SIZE 6

// Display result of the assignment
struct evaluation_factors {
    bool valid;
    char *assignment;
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
    // char *quotation_mark_str = strchr(line, '"');
    char *display_str = (char *)malloc (sizeof (char) * strlen(line));
    for (int i = (COMM_SIZE + 1); i < (strlen(line)-1); i++) 
        display_str[i-(COMM_SIZE + 1)] = line[i];
    return display_str;
}

char *is_dollar_present(char *str)
{
    return strchr(str, '$');
}

char *extract_substring_of_equal_sign_string(char *assignment, char cond)
{
    int index = strcspn(assignment, "=");
    char *tmp_str = (char *)malloc (sizeof (char) * strlen(assignment));

    if (cond == 'p')
        for (int i = 0; i < index; i++) strncat(tmp_str, &assignment[i], 1);
    else
        for (int i = index+1; i < strlen(assignment); i++) strncat(tmp_str, &assignment[i], 1);
    
    return tmp_str;
}

char *extract_substring_of_dollar_string(char *str, char *var_str, char cond)
{
    int index = strcspn(str, "$");
    char *substring = (char *)malloc (sizeof (char) * strlen(str));

    if (cond == 'p') {
        for (int i = 0; i < index; i++) strncat(substring, &str[i], 1);
    } else {
        for (int i = index+1+strlen(var_str); i < strlen(str); i++) strncat(substring, &str[i], 1);
    }

    return substring;
}
