#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMM_SIZE 8

bool is_print_command_valid(char *line)
{
    // Retrieve the first 8 characters forming the "echo -e "
    // NOTE: Need to handle single quote as invalid
    char command[COMM_SIZE + 1];
    for (int i = 0; i < COMM_SIZE; i++) command[i] = line[i];
    command[COMM_SIZE] = '\0';

    if (!strcmp(command, "echo -e ")) return true;
    return false;
}

char *retrieve_string_inside_quotation_mark(char *line)
{
    // Retrieve the string inside the quotation marks
    char *display_str = (char *)malloc (sizeof (char) * strlen(line));
    for (int i = (COMM_SIZE + 1); i < (strlen(line)-1); i++) 
        display_str[i-(COMM_SIZE + 1)] = line[i];
    return display_str;
}

int get_index_of_char_in_string(char *assignment)
{
    // Retrieve index of '=' sign
    int index = 0;
    for (int i = 0; i < strlen(assignment); i++) {
        if (assignment[i] == '=') return i;
    }
    return index;
}

char *extract_substring_before_equal_sign(char *assignment)
{
    char *tmp_str = (char *)malloc (sizeof (char) * strlen(assignment));
    for (int i = 0; i < strlen(assignment); i++) {
        if (assignment[i] == '=') break;
        tmp_str[i] = assignment[i];
    }
    return tmp_str;
}

char *extract_value_after_equal_sign(char *assignment, int start_idx)
{
    // Extract value after '=' sign
    char *value_str = (char *)malloc (sizeof (char) * strlen(assignment));
    for (int i = start_idx+1; i < strlen(assignment); i++) {
        value_str[i-(start_idx+1)] = assignment[i];
    }
    return value_str;
}

int get_index_of_character(char *str)
{
    int index = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == '$') index = i;
    }
    return index;
}

char *extract_prefix_from_string(char *str, int index)
{
    char *prefix = (char *)malloc (sizeof (char) * strlen(str));
    for (int i = 0; i < index; i++) strncat(prefix, &str[i], 1);
    return prefix;
}

char *extract_suffix_from_string(char *str, char *var, int index)
{
    char *suffix = (char *)malloc (sizeof (char) * strlen(str));
    for (int i = index+1+strlen(var); i < strlen(str); i++) {
        strncat(suffix, &str[i], 1);
    }
    suffix[strlen(str)] = '\0';
    return suffix;
}
