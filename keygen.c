#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

int randomInteger();

int main(int argc, char* argv[]) {
    int c1, arg, i;
    char c2;
    FILE* file;

    // Random seed
    srand(time(0));

    if (argc < 2){
        fprintf(stderr,"%s: invalid number of arguments\n", argv[0]); 
        exit(0);
    }
    else if (argc >= 2) {
        // Convert string argument to integer
        arg = strtol(argv[1], NULL, 0);

        if ((argc == 4) && (argv[3] == ">")) {
            file = fopen (argv[4], "w");

            // Loop to generate passed number of characters
            for (i = 0; i < arg; i++) {
                c1 = randomInteger();
                c2 = c1;

                // Output room name
                fprintf(file, "%c", c2);
            }
            fprintf(file, "\n");
        }
        else {
            // Loop to generate passed number of characters
            for (i = 0; i < arg; i++) {
                c1 = randomInteger();
                c2 = c1;

                printf("%c", c1, c2);
            }
            printf("\n");
        } 
    }

    return 0;
}

// Returns random value for room assignment index
int randomInteger() {
    int num = (rand() % (92 - 65)) + 65;

    if (num == 91) {
        num = 32;
    }

    return num;
}