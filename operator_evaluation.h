#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EQUAL "-eq"
#define NOT_EQUAL "-ne"
#define GREATER_THAN "-gt"
#define LESS_THAN "-lt"
#define GREATER_EQUAL_THAN "-ge"
#define LESS_EQUAL_THAN "-le"

const char* MATH_OPS = "*/+-%";


double calculate(char *val_one, char *val_two, char *math_op)
{
    double tmp_total = 0;
    if (*math_op == MATH_OPS[0]) tmp_total = atoi(val_one) * atoi(val_two);
    if (*math_op == MATH_OPS[1]) tmp_total = atoi(val_one) / atoi(val_two);
    if (*math_op == MATH_OPS[2]) tmp_total = atoi(val_one) + atoi(val_two);
    if (*math_op == MATH_OPS[3]) tmp_total = atoi(val_one) - atoi(val_two);
    if (*math_op == MATH_OPS[4]) tmp_total = atoi(val_one) % atoi(val_two);

    return tmp_total;
}

bool is_non_nummeric_char_present(char *val)
{
    // Validate if a non-nummeric character is present
    for (int i = 0; i < strlen(val); i++) {
        if (val[i] < 48 || val[i] > 57) return true;
    }
    return false;
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

    // Handle cases such as ' *3', '3* ', '*3' and '3*'
    if (strlen(val_one) == 0 || strlen(val_two) == 0 || strstr(val_one, " ") || strstr(val_two, " ")) {
        printf("invalid expression\n");
        e.valid = false;
    }
    else {
        // Validate if a non-nummeric character is present
        if (!is_non_nummeric_char_present(val_one) && !is_non_nummeric_char_present(val_two)) {
            double total = calculate(val_one, val_two, math_op);
            gcvt(total, 10, e.assignment);
        }
    }

    return e;
}

double evaluate(char *val_one, char *val_two, char *math_op)
{
    if ((strstr(math_op, EQUAL) && atoi(val_one) == atoi(val_two)) || 
        (strstr(math_op, NOT_EQUAL) && atoi(val_one) != atoi(val_two)) ||
        (strstr(math_op, GREATER_THAN) && atoi(val_one) > atoi(val_two)) ||
        (strstr(math_op, LESS_THAN) && atoi(val_one) < atoi(val_two)) ||
        (strstr(math_op, GREATER_EQUAL_THAN) && atoi(val_one) >= atoi(val_two)) ||
        (strstr(math_op, LESS_EQUAL_THAN) && atoi(val_one) <= atoi(val_two))) return true;
    return false;
}

struct evaluation_factors evaluate_relational_operator(struct evaluation_factors e, char *rel_op)
{
    // Must contain exactly 2 gaps in front and at the back of the relational operator
    char *val_one = (char *)malloc (sizeof (char) * strlen(e.assignment));
    char *val_two = (char *)malloc (sizeof (char) * strlen(e.assignment));

    int i = 0;
    int i_two = 0;
    int gap_count = 0;
    while(i < strlen(e.assignment)) {
        if (e.assignment[i] == ' ') {
            gap_count += 1;
            i += 1;
        }

        if (gap_count == 0) val_one[i] = e.assignment[i];
        if (gap_count == 2) {
            val_two[i_two] = e.assignment[i];
            i_two += 1;
        }        
        i += 1;
    }

    // Validate if a non-nummeric character is present
    if (!is_non_nummeric_char_present(val_one) && !is_non_nummeric_char_present(val_two)) {
        bool result = evaluate(val_one, val_two, rel_op);
        gcvt(result, 10, e.assignment);
    }

    return e;
}
