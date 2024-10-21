#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>

// Включаем заголовочные файлы для работы с изображениями
// Используем библиотеку stb_image для загрузки изображений
//https://github.com/nothings/stb/tree/master
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Максимальное количество потоков (оптимально ввести максимальное количество потоков в вашем процессоре)
#define MAX_THREADS 8

// Структура для передачи данных в потоки
typedef struct {
    int thread_id;             // Идентификатор потока
    int num_threads;           // Общее количество потоков
    int width;                 // Ширина изображения
    int height;                // Высота изображения
    unsigned char *input_image;  // Входное изображение
    unsigned char *output_image; // Выходное изображение (с результатами)
} ThreadData;

// Ядра Собела для осей X и Y
int sobel_x[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
};

int sobel_y[3][3] = {
        {1, 2, 1},
        {0, 0, 0},
        {-1, -2, -1}
};

// Функция, выполняемая каждым потоком для применения фильтра Собела
void *apply_sobel(void *arg) {
    ThreadData *data = (ThreadData *)arg; // Преобразуем аргумент в структуру данных потока
    int start_row = (data->height / data->num_threads) * data->thread_id; // Начальная строка для данного потока
    int end_row = (data->thread_id == data->num_threads - 1) ? data->height : start_row + (data->height / data->num_threads); // Конечная строка для потока

    // Применяем фильтр Собела к каждой строке, закрепленной за данным потоком
    for (int y = start_row; y < end_row; y++) {
        for (int x = 0; x < data->width; x++) {
            int gx = 0, gy = 0; // Переменные для градиентов по X и Y

            // Применяем свертку с ядром Собела (по обеим осям)
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int ix = x + kx; // Координата X для соседних пикселей
                    int iy = y + ky; // Координата Y для соседних пикселей

                    // Проверяем границы изображения, чтобы не выйти за них
                    if (ix >= 0 && ix < data->width && iy >= 0 && iy < data->height) {
                        int pixel = data->input_image[iy * data->width + ix]; // Получаем значение пикселя
                        gx += pixel * sobel_x[ky + 1][kx + 1]; // Применяем ядро для оси X
                        gy += pixel * sobel_y[ky + 1][kx + 1]; // Применяем ядро для оси Y
                    }
                }
            }

            // Вычисляем величину градиента
            int magnitude = (int)sqrt(gx * gx + gy * gy);

            // Ограничиваем значения градиента от 0 до 255
            if (magnitude > 255) magnitude = 255;
            if (magnitude < 0) magnitude = 0;

            // Записываем результат в выходное изображение
            data->output_image[y * data->width + x] = (unsigned char)magnitude;
        }
    }

    pthread_exit(NULL); // Завершаем поток
}

int main(int argc, char *argv[]) {
    // Проверка корректности ввода аргументов
    if (argc != 4) {
        printf("Usage: %s <input_image> <output_image> <num_threads>\n", argv[0]);
        return -1;
    }

    int num_threads = atoi(argv[3]); // Количество потоков, переданное как аргумент

    // Проверяем, что количество потоков допустимо
    if (num_threads <= 0 || num_threads > MAX_THREADS) {
        printf("Number of threads must be between 1 and %d\n", MAX_THREADS);
        return -1;
    }

    int width, height, channels; // Переменные для хранения размеров изображения и количества каналов

    // Загружаем изображение в градациях серого
    unsigned char *input_image = stbi_load(argv[1], &width, &height, &channels, 1);
    if (input_image == NULL) {
        printf("Error loading image %s\n", argv[1]);
        return -1;
    }

    // Выделяем память для выходного изображения
    unsigned char *output_image = malloc(width * height);
    if (output_image == NULL) {
        printf("Error allocating memory for output image\n");
        stbi_image_free(input_image); // Освобождаем память, выделенную под входное изображение
        return -1;
    }

    pthread_t threads[num_threads]; // Массив потоков
    ThreadData thread_data[num_threads]; // Массив структур с данными для каждого потока

    // Измеряем время начала выполнения
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Создаем потоки
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i; // Идентификатор потока
        thread_data[i].num_threads = num_threads; // Общее количество потоков
        thread_data[i].width = width; // Ширина изображения
        thread_data[i].height = height; // Высота изображения
        thread_data[i].input_image = input_image; // Входное изображение
        thread_data[i].output_image = output_image; // Выходное изображение
        pthread_create(&threads[i], NULL, apply_sobel, (void *)&thread_data[i]); // Запуск потока
    }

    // Ожидаем завершения всех потоков
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Измеряем время завершения выполнения
    gettimeofday(&end_time, NULL);
    double execution_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0; // Переводим секунды в миллисекунды
    execution_time += (end_time.tv_usec - start_time.tv_usec) / 1000.0; // Добавляем микросекунды

    printf("Execution time with %d threads: %f ms\n", num_threads, execution_time);

    // Сохраняем выходное изображение в формате PNG
    if (!stbi_write_png(argv[2], width, height, 1, output_image, width)) {
        printf("Error writing image %s\n", argv[2]);
    }

    // Освобождаем выделенную память
    stbi_image_free(input_image);
    free(output_image);

    return 0;
}
