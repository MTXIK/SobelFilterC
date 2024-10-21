# Название итогового исполняемого файла
TARGET = sobel_filter

# Компилятор
CC = gcc

# Флаги компилятора
CFLAGS = -Wall -pthread -lm

# Заголовочные файлы и библиотеки для работы с изображениями (stb_image)
STB_FILES = ./src/stb_image.h ./src/stb_image_write.h

# Исходные файлы
SRC = ./src/sobel_filter.c

# Правило по умолчанию
all: $(TARGET)

# Правило сборки программы
$(TARGET): $(SRC) $(STB_FILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Очистка скомпилированных файлов
clean:
	rm -f $(TARGET)

# Очистка скомпилированных файлов и временных файлов редакторов
distclean: clean
	rm -f *~

# Правило для повторной сборки программы
rebuild: distclean all
