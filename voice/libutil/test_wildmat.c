/*
 * Testprogram for the wildmat function
 */

#include <stdio.h>

/* Yes, we use gets not fgets.  Sue me. */
extern char     *gets();
extern int wildmat(char *text, char *p, int length);

int
main()
{
    char         p[80];
    char         text[80];

    printf("Wildmat tester.  Enter pattern, then strings to test.\n");
    printf("A blank line gets prompts for a new pattern; a blank pattern\n");
    printf("exits the program.\n");

    for ( ; ; ) {
        printf("\nEnter pattern:  ");
        (void)fflush(stdout);
        if (gets(p) == NULL || p[0] == '\0')
            break;
        for ( ; ; ) {
            printf("Enter text:  ");
            (void)fflush(stdout);
            if (gets(text) == NULL)
                exit(0);
            if (text[0] == '\0')
                /* Blank line; go back and get a new pattern. */
                break;
            printf("      %s\n", wildmat(text, p, strlen(p)) ? "YES" : "NO");
        }
    }

    exit(0);
}
