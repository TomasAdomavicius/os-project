#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct evaluation_factors extract_assign(struct evaluation_factors e, char *line) 
{
    // Retrieve value after '='
    int index = strcspn(line, "=");

    e.variable_name = (char *)malloc (sizeof (char) * strlen(line));
    for (int i = 0; i < index; i++) strncat(e.variable_name, &line[i], 1);

    e.assignment = (char *)malloc (sizeof (char) * strlen(line));
    for (int i = index+1; i < strlen(line); i++) strncat(e.assignment, &line[i], 1);

    return e;
}

struct evaluation_factors classify(struct evaluation_factors e)
{
    // Check if the value is in `` or not
    char *first_backtick = strchr(e.assignment, '`');
    char *second_backtick = strrchr(e.assignment, '`');

    e.evaluating_value = false;
    e.valid = true;
    if (first_backtick && second_backtick) {
        e.evaluating_value = true;
        if (!strstr(e.assignment, "expr ")) e.valid = false;
    }

    return e;
}

struct evaluation_factors extract_evaluate(struct evaluation_factors e) 
{
    // Extract value after the first space in 'expr'
    int index = strcspn(e.assignment, " ");
    char *tmp_val = (char *)malloc (sizeof (char) * strlen(e.assignment));
    for (int i = index+1; i < strlen(e.assignment)-1; i++) strncat(tmp_val, &e.assignment[i], 1);
    e.assignment = tmp_val;

    // Calculate total value from basic mathematical operators
    // printf("%s\n", e.assignment);
    if (strstr(e.assignment, EQUAL)) return evaluate_relational_operator(e, EQUAL);
    if (strstr(e.assignment, NOT_EQUAL)) return evaluate_relational_operator(e, NOT_EQUAL);
    if (strstr(e.assignment, GREATER_THAN)) return evaluate_relational_operator(e, GREATER_THAN);
    if (strstr(e.assignment, LESS_THAN)) return evaluate_relational_operator(e, LESS_THAN);
    if (strstr(e.assignment, GREATER_EQUAL_THAN)) return evaluate_relational_operator(e, GREATER_EQUAL_THAN);
    if (strstr(e.assignment, LESS_EQUAL_THAN)) return evaluate_relational_operator(e, LESS_EQUAL_THAN);

    return calculate_by_math_operator(e);
}

// Evaluate if the assignment is syntax-correct
struct evaluation_factors verify_assignment_syntax(struct evaluation_factors e, char *line) {
    /* Validate:
        - Variable is followed by "="
        - Only 1 "=" in the assignment
    */
    int index = 0;
    int equal_sign_count = 0;
    while (index < strlen(line)) {
        if (line[index] == '=') equal_sign_count += 1;
        index += 1;
    }

    if (equal_sign_count == 1) {
        e = extract_assign(e, line);
        e = classify(e);
    }

    return e;
}
