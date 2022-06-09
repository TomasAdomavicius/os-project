#define _OPEN_SYS_ITOA_EXT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char* MATH_OPS = "*/+-";

double calculate(char *val_one, char *val_two, char *math_op)
{
    double tmp_total = 0;
    if (*math_op == MATH_OPS[0]) tmp_total = atoi(val_one) * atoi(val_two);
    if (*math_op == MATH_OPS[1]) tmp_total = atoi(val_one) / atoi(val_two);
    if (*math_op == MATH_OPS[2]) tmp_total = atoi(val_one) + atoi(val_two);
    if (*math_op == MATH_OPS[3]) tmp_total = atoi(val_one) - atoi(val_two);

    return tmp_total;
}

struct evaluation_factors calculate_by_math_operator(struct evaluation_factors e)
{
    char *val_one = (char *)malloc (sizeof (char) * strlen(e.assignment));
    char *val_two = (char *)malloc (sizeof (char) * strlen(e.assignment));
    
    char *math_op = (char *)malloc (sizeof (char) * strlen(e.assignment));
    int math_op_count = 0;
    
    int i = 0;
    int i_two = 0;

    while(i < strlen(e.assignment)) {
        for (int j = 0; j < strlen(MATH_OPS); j++) {
            if (e.assignment[i] == MATH_OPS[j]) {
                math_op[0] = e.assignment[i];
                math_op_count += 1;
                i += 1;
            }
        }

        if (math_op_count == 0) val_one[i] = e.assignment[i];
        if (math_op_count == 1) {
            val_two[i_two] = e.assignment[i];
            i_two += 1;
        }
        
        if (math_op_count == 2) break;
        i += 1;
    }

    if (strlen(val_one) == 0 || strlen(val_two) == 0 || strstr(val_one, " ") || strstr(val_two, " ")) {
        printf("invalid expression\n");
        e.valid = false;
    }
    else {
        double total = calculate(val_one, val_two, math_op);
        gcvt(total, 10, e.assignment);
    }
    // printf("%s\n", val_one);
    // printf("%s\n", val_two);

    return e;
}

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
    return calculate_by_math_operator(e);
}
