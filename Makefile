CC = c99
CFLAGS = -D_XOPEN_SOURCE=700 -O2

all: task1/task1 task2/task2 task3/task3 task4/task4 task5/task5 task6/task6 task7/task7

task1/task1: task1/main.c
	$(CC) $(CFLAGS) task1/main.c -o task1/task1

task2/task2: task2/main.c
	$(CC) $(CFLAGS) task2/main.c -o task2/task2

task3/task3: task3/main.c
	$(CC) $(CFLAGS) task3/main.c -o task3/task3

task4/task4: task4/main.c
	$(CC) $(CFLAGS) task4/main.c -o task4/task4

task5/task5: task5/main.c task5/line_table.h task5/query.h
	$(CC) $(CFLAGS) task5/main.c -o task5/task5

task6/task6: task6/main.c task6/line_table.h task6/query.h
	$(CC) $(CFLAGS) task6/main.c -o task6/task6

task7/task7: task7/main.c task7/query.h
	$(CC) $(CFLAGS) task7/main.c -o task7/task7

test:
	python3 test_tasks.py

clean:
	rm -f task1/task1 task2/task2 task3/task3 task4/task4 task5/task5 task6/task6 task7/task7
