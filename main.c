#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Levenshtein Distance */
#define RECURSIVE 0
#define MEMOIZATION 1
#define DYNAMIC_PROGRAMMING 2
#define MODE DYNAMIC_PROGRAMMING


typedef enum {
    DONE,
    ADD,
    REMOVE,
    REPLACE,
    IGNORE,
} Action;

const char* ACTIONS[] = {
    [ADD]     = "ADD",
    [REMOVE]  = "REMOVE",
    [REPLACE] = "REPLACE",
    [IGNORE]  = "IGNORE",
    [DONE]    = "DONE",
};

typedef struct {
    Action*  data;
    ssize_t  rows;
    ssize_t  columns;
} ActionMatrix;

Action* action_matrix_at(ActionMatrix* array, ssize_t row, ssize_t column) {
    return &array->data[array->columns * row + column];
}

typedef struct {
    ssize_t* data;
    ssize_t  rows;
    ssize_t  columns;
} Array2D;

ssize_t* at(Array2D* array, ssize_t row, ssize_t column) {
    return &array->data[array->columns * row + column];
}


typedef struct {
    const char* data;
    ssize_t size;
} Str;
#define STR(x) (Str) { .data=x, .size=((ssize_t)sizeof(x))-1 }
#define STR_FMT "%.*s"
#define STR_ARG(x) (int)(x).size, (x).data

Str slice(Str source, ssize_t start, ssize_t end) {
    if (source.size <= start) return STR("");
    if (source.size <= -end)  return STR("");
    if (source.size <=  end)  end = source.size;
    if (end < 0)              end = source.size + end;

    ssize_t size = end-start;
    return (Str) { .data=source.data+start, .size=size };
}

char last(Str source) {
    if (source.size == 0)
        return '\0';
    return source.data[source.size-1];
}



ssize_t min(ssize_t a, ssize_t b, ssize_t c) {
    if (a < b) {
        return a < c ? a : c;
    } else {
        return b < c ? b : c;
    }
}



#if MODE == RECURSIVE
ssize_t distance(Str source, Str target) {
    if (source.size == 0) return target.size;
    if (target.size == 0) return source.size;
    if (last(source) == last(target)) return distance(slice(source, 0, -1), slice(target, 0, -1));
    ssize_t remove  = distance(slice(source, 0, -1), target);
    ssize_t add     = distance(source, slice(target, 0, -1));
    ssize_t replace = distance(slice(source, 0, -1), slice(target, 0, -1));
    return 1 + min(remove, add, replace);
}



#elif MODE == MEMOIZATION
ssize_t distance_rec(Str source, Str target, Array2D* memo) {
    ssize_t result = *at(memo, source.size, target.size);
    if (result != -1) {
        return result;
    }

    if (source.size == 0) {
        result = target.size;
        *at(memo, source.size, target.size) = result;
        return result;
    }
    if (target.size == 0) {
        result = source.size;
        *at(memo, source.size, target.size) = result;
        return result;
    }
    if (last(source) == last(target)) {
        result = distance_rec(slice(source, 0, -1), slice(target, 0, -1), memo);
        *at(memo, source.size, target.size) = result;
        return result;
    }

    ssize_t remove  = distance_rec(slice(source, 0, -1), target, memo);
    ssize_t add     = distance_rec(source, slice(target, 0, -1), memo);
    ssize_t replace = distance_rec(slice(source, 0, -1), slice(target, 0, -1), memo);

    result = 1 + min(remove, add, replace);
    *at(memo, source.size, target.size) = result;
    return result;
}


ssize_t distance(Str source, Str target) {
    ssize_t size = (source.size + 1) * (target.size + 1);
    Array2D memo = (Array2D) {
        .rows    = source.size+1,
        .columns = target.size+1,
        .data    = malloc(size)
    };
    for (int i = 0; i < size; ++i) {
        memo.data[i] = -1;
    }

    ssize_t result = distance_rec(source, target, &memo);

    free(memo.data);
    return result;
}
#elif MODE == DYNAMIC_PROGRAMMING
int smallest(ssize_t a, ssize_t b, ssize_t c) {
    bool x = (a < b && a < c);
    bool y = (b < a && b < c);
    bool z = (c < a && c < b);
    return x + (2*y) + (3*z);
}


void print_path(Array2D memo, Str source, Str target) {
    ssize_t i = source.size;
    ssize_t j = target.size;
    if (i == 0) {
        while (j--)
            printf("- %s -", "REMOVE");
        return;
    } else if (j == 0) {
        while (i--)
            printf("- %s -", "ADD");
        return;
    }

    ssize_t remove  = *at(&memo, i,   j-1);
    ssize_t add     = *at(&memo, i-1, j  );
    ssize_t replace = *at(&memo, i-1, j-1);

    switch (smallest(remove, add, replace)) {
        case 0:
            print_path(memo, source, slice(target, 0, j-1));
            printf("- %s -", "REMOVE");
        break;
        case 1:
            print_path(memo, slice(source, 0, i-1), target);
            printf("- %s -", "ADD");
        break;
        default:
            print_path(memo, slice(source, 0, i-1), slice(target, 0, j-1));
            printf("- %s -", last(source) == last(target) ? "IGNORE" : "REPLACE");
        break;
    }
}

ssize_t distance(Str source, Str target) {
    printf("Path from " STR_FMT " to " STR_FMT "\n", STR_ARG(source), STR_ARG(target));

    ssize_t size = (source.size + 2) * (target.size + 2);
    Array2D memo = (Array2D) {
        .rows    = source.size+2,
        .columns = target.size+2,
        .data    = calloc(sizeof(ssize_t), size)
    };

    for (int i = 1; i < source.size+1; ++i) {
        *at(&memo, i, 0) = i;
    }
    for (int j = 1; j < target.size + 1; ++j) {
        *at(&memo, 0, j) = j;
    }

    for (int i = 1; i < source.size+1; ++i) {
        for (int j = 1; j < target.size+1; ++j) {
            char x = source.data[i-1];
            char y = target.data[j-1];

            bool is_same = (x == y);

            ssize_t remove  = *at(&memo, i, j-1)   + 1;
            ssize_t add     = *at(&memo, i-1, j)   + 1;
            ssize_t replace = *at(&memo, i-1, j-1) + !is_same;

            ssize_t result;
            switch (smallest(remove, add, replace)) {
                case 0:
                    result = remove;
                    break;
                case 1:
                    result = add;
                    break;
                case 2:
                    result = replace;
                break;
                default:
                    result = min(remove, add, replace);
                break;
            }

            *at(&memo, i, j) += result;
        }
    }

    for (int i = 0; i < source.size + 1; ++i) {
        for (int j = 0; j < target.size + 1; ++j) {
            printf("%-8zd ", *at(&memo, i, j));
        }
        printf("\n");
    }

    print_path(memo, source, target);
    printf("\n");

    ssize_t result = *at(&memo, source.size, target.size);

    free(memo.data);
    return result;
}
#endif


int main() {
    // 0
    printf("Distance: %zd\n", distance(STR("apple"), STR("apple")));
    printf("\n");

    // 1
    printf("Distance: %zd\n", distance(STR("apple"), STR("papple")));
    printf("Distance: %zd\n", distance(STR("apple"), STR("appled")));
    printf("Distance: %zd\n", distance(STR("apple"), STR("appdle")));
    printf("Distance: %zd\n", distance(STR("apple"), STR("apdle")));
    printf("\n");

    // 2
    printf("Distance: %zd\n", distance(STR("apple"), STR("aplep")));
    printf("\n");

    // 5
    printf("Distance: %zd\n", distance(STR("apple"), STR("xxxxx")));
    printf("\n");

    return 0;
}
