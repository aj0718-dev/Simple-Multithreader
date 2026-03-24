#include <stdio.h>
#include <string.h>

int main() {
    char a[100] = "./sanskar";
    char* name;

    name = strtok(a, "./");

    printf("%s, %s\n", a, name);
    return 0;
}