#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/resource.h>

extern char **environ; // Переменные окружения текущего процесса

// Преобразуем строку в неотрицательное число
long read_number(const char *text) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' || value < 0) {
        fprintf(stderr, "Invalid nonnegative number: %s\n", text);
        exit(1);
    }
    return value;
}

int main(int argc, char *argv[]) {
    int commands[128], count = 0, option;
    char *values[128];

    if (argc == 1) {
        puts("Options: -i -s -p -u -Unumber -c -Csize -d -v -Vname=value");
        return 0;
    }

    // Читаем опции слева направо и сохраняем их
    opterr = 0;
    while ((option = getopt(argc, argv, ":ispuU:cC:dvV:")) != -1) {
        if (option == '?' || option == ':') {
            fprintf(stderr, "Unknown option or missing argument: -%c\n",
                    optopt);
            return 1;
        }
        if (count == 128) {
            fprintf(stderr, "Too many options\n");
            return 1;
        }
        commands[count] = option;
        values[count++] = optarg;
    }

    // Отдельные аргументы без опции условием не предусмотрены
    if (optind < argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        return 1;
    }

    // Выполняем сохранённые опции справа налево
    for (int i = count - 1; i >= 0; --i) {
        int result = 0;

        switch (commands[i]) {
        case 'i': // Реальные и эффективные идентификаторы
            printf("UID=%lu EUID=%lu GID=%lu EGID=%lu\n",
                   (unsigned long)getuid(), (unsigned long)geteuid(),
                   (unsigned long)getgid(), (unsigned long)getegid());
            break;

        case 's': // Делаем процесс лидером группы
            result = setpgid(0, 0);
            break;

        case 'p': // Процесс, его родитель и группа процессов
            printf("PID=%ld PPID=%ld PGID=%ld\n",
                   (long)getpid(), (long)getppid(), (long)getpgrp());
            break;

        case 'u':
        case 'c': { // Выводим соответствующий лимит
            struct rlimit limit;
            int resource = commands[i] == 'u' ? RLIMIT_FSIZE : RLIMIT_CORE;
            result = getrlimit(resource, &limit);
            if (result == -1) break;

            const char *name = commands[i] == 'u' ? "File limit" : "Core limit";
            if (limit.rlim_cur == RLIM_INFINITY)
                printf("%s: unlimited\n", name);
            else
                printf("%s: %llu bytes\n", name,
                       (unsigned long long)limit.rlim_cur);
            break;
        }

        case 'U':
        case 'C': { // Меняем текущий лимит, сохраняя максимальный
            struct rlimit limit;
            long number = read_number(values[i]);
            int resource = commands[i] == 'U' ? RLIMIT_FSIZE : RLIMIT_CORE;
            result = getrlimit(resource, &limit);
            if (result == -1) break;

            limit.rlim_cur = (rlim_t)number;
            result = setrlimit(resource, &limit);
            break;
        }

        case 'd': { // Выводим текущую рабочую папку
            char path[PATH_MAX];
            if (getcwd(path, sizeof(path)) == NULL)
                result = -1;
            else
                puts(path);
            break;
        }

        case 'v': // Выводим все переменные окружения
            for (char **p = environ; *p != NULL; ++p)
                puts(*p);
            break;

        case 'V': { // Разделяем имя и значение переменной
            char *equals = strchr(values[i], '=');
            if (equals == NULL || equals == values[i]) {
                fprintf(stderr, "Expected -Vname=value\n");
                return 1;
            }

            *equals = '\0';
            result = setenv(values[i], equals + 1, 1);
            *equals = '='; // Восстанавливаем исходную строку
            break;
        }
        }

        // Общая проверка ошибок системных функций
        if (result == -1) {
            fprintf(stderr, "Option -%c: ", commands[i]);
            perror("system call");
            return 1;
        }
    }
    return 0;
}