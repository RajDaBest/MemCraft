#include <stdio.h>
#include <string.h>

int main(void)
{
    char str[500] = "hello, world\nlet's go\nmoan";

    char *line_one = strtok(str, "\n");
    char *line_two = strtok(NULL, "\n");

    printf("%s\n%s\n", line_one, line_two);
}