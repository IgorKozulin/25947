#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/resource.h>
#include <errno.h>

extern char **environ; // массив строк содержащий все переменные окружения текущего процесса

long parse_long(const char *s){ // function for parsing of numbers
    char *endptr; // указатель на первый символ после числа, которое не удалось распознать
    errno = 0; // обнуляем глобальный код ошибки перед вызовом. Иначе можем увидеть старую ошибку
    long val = strtol(s, &endptr, 10); // преобразовываем строку в число, которое кладём в val, а endptr покажет, где парсинг остановился.
    if (errno == ERANGE) // если число оказалось слишком большим или маленьким для типа long
    {
        fprintf(stderr, "Число вне диапозона: %s\n", s);
        exit(1);
    }
    if (*endptr != '\0' || endptr == s) // проверка на мусор в строке(если после числа остались буквы "123fvc" || ни одного числа нету "abc")
    {
        fprintf(stderr, "Неккоректное число: %s\n", s);
        exit(1);
    }
    return val;
    
}

int main(int argc, char *argv[])
{
    const char *optstring = "ispuU:cC:dvV:"; // строка описывающая какие опции разрешены

    typedef struct 
    {
        char opt; // опция
        char *arg; // указатель значение аргумента опции
    } option_t;

    option_t opts[100];
    int nopts = 0;


    // Основной цикл обработки опций
    int c;
    while ((c = getopt(argc, argv, optstring)) != -1)
    {
        if (c == '?') // если встречаем неизвестную нам опции выводим ошибку и OPT is unknown
        {
            fprintf(stderr, "OPT is unknown\n");
            exit(1);
        }
        // если все ок, тогда сохраняем опцию, ее значение аргумента и увеличиваем счетчик
        opts[nopts].opt = c;
        opts[nopts].arg = optarg;
        nopts++;
    }
    
    // Обратный цикл обработки опций справа налево
    for (int i = nopts - 1; i >= 0; i--)
    {
        switch (opts[i].opt)
        {
        case 'i': // i - идентификаторы пользователя/группы
            printf("real uid=%d, effective uid =%d\n", getuid(), geteuid()); // реальные идентификаторы (кем реально запущен процесс)
            printf("real gid=%d, effective gid =%d\n", getgid(), getegid()); // эффективные от чьего имени процесс действует прямо сейчас;
            break;
        case 'p': // p - идентификаторы процесса
            printf("pid=%d, ppid=%d, pgid=%d\n", getpid(), getppid(), getpgrp()); 
            // getpid() — ID текущего процесса, 
            // getppid() — ID родительского процесса (кто его запустил, обычно shell), 
            // getpgrp() — ID группы процессов, к которой принадлежит текущий процесс.
            break;
        case 's': // s - стать лидером группы процессов
            if (setpgid(0, 0) == -1) 
            // setpgid(текущий процесс, используй pid текущего процесса как новый pgid) - меняет группу процессов у указанного процесса.
            {
                perror("setpgid");
            }
            else {printf("processes is now group leader\n");}
            break;
        case 'd':{ // d - текущая директория
            char buf[PATH_MAX];
            if (getcwd(buf, sizeof(buf)) == NULL)
            // getcwd - заполняет буфер buf абсолютным путём текущей рабочей директории.
            // PATH_MAX - системная константа максимально допустимой длины пути,
            {
                perror("getcwd");
            }
            else {printf("cwd = %s\n", buf);}
            break;
        }
        case 'v':{ // v - вывод переменных окружения
            // Проходим по массиву environ
            // и печатаем каждую строку — каждая уже в формате "ИМЯ=значение".
            for (int j = 0; environ[j] != NULL; j++)
            {
                printf("%s\n", environ[j]);
            }
            break;
            
        }
        case 'V':{ // V - добавление/изменение переменной окружения
            char *copy = strdup(opts[i].arg);// strdup создаёт копию строки в динамической памяти
            // putenev - сохраняет указатель на строку strdup в массиве environ
            if (copy == NULL){perror("strdup");}
            else if (putenv(copy) != 0) {perror("putenv");}
            break;
        }
        case 'u':{ // u - вывод текущего ulimit
            struct rlimit r1;
            // getrlimit(RLIMIT_FSIZE, &r1) заполняет структуру r1 данными о лимите на размер файла, который процесс может создать.
            if(getrlimit(RLIMIT_FSIZE, &r1) == -1){perror("getrlimit");}
            else {printf("rlimit (file size) %ld\n", (long)r1.rlim_cur);}
            break;
        }
        case 'U':{ // U - изменение ulimit
            long newval = parse_long(opts[i].arg);
            struct rlimit r2;
            getrlimit(RLIMIT_FSIZE, &r2);// читаем текущий лимит 
            r2.rlim_cur = newval; // и меняем только rlim_cur на новое значение
            if (setrlimit(RLIMIT_FSIZE, &r2) == -1)
            {
                perror("setrlimit");
            }
            else{
                printf("ulimit set to %ld\n", newval);
            }
            break;
            
        }
        case 'c':{ // c - размер core-файла
            struct rlimit r3;
            if(getrlimit(RLIMIT_CORE, &r3) == -1){perror("getrlimit");}
            else {printf("core file size limit = %ld\n", (long)r3.rlim_cur);}
            break;
        }

        case 'C':{ // C - размер core-файла
            long newsize = parse_long(opts[i].arg);
            struct rlimit r4;
            getrlimit(RLIMIT_CORE, &r4);
            r4.rlim_cur = newsize;
            if (setrlimit(RLIMIT_CORE, &r4) == -1)
            {
                perror("setrlimit");
            }
            else{
                printf("core file size limit set to %ld\n", newsize );
            }
            break;
            
        }

        default:
        printf("case default: %c\n", opts[i].opt);
            break;
        }
    }
    
    

    return 0;
}
