CC = gcc
CFLAGS = -Wall -Wextra -g

# 目標：依賴項目
lab5: lab5.o
	$(CC) $(CFLAGS) -o lab5 lab5.o

# 依賴項目的編譯規則
lab5.o: lab5.c
	$(CC) $(CFLAGS) -c lab5.c

# 清理編譯產生的檔案
clean:
	rm -f lab5 lab5.o
