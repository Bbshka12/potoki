#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define QUEUE_SIZE 10
#define PRODUCERS 4
#define CONSUMERS 2
#define TARGET_SUM 10000

// Глобальные переменные
int queue[QUEUE_SIZE];  // Очередь для хранения чисел
int count = 0;          // Текущее количество элементов в очереди
int sum = 0;            // Общая сумма чисел, обработанных потребителями

HANDLE mutex;           // Мьютекс для доступа к очереди
HANDLE dataAvailable;   // Событие: данные доступны для потребителей
HANDLE spaceAvailable;  // Событие: есть место для производителей

// Функции производителя и потребителя
DWORD WINAPI Producer(LPVOID lpParam);
DWORD WINAPI Consumer(LPVOID lpParam);

int main() {
    // Инициализация синхронизационных объектов
    mutex = CreateMutex(NULL, FALSE, NULL);
    dataAvailable = CreateEvent(NULL, TRUE, FALSE, NULL);  // Изначально нет данных
    spaceAvailable = CreateEvent(NULL, TRUE, TRUE, NULL);  // Изначально есть место

    if (!mutex || !dataAvailable || !spaceAvailable) {
        printf("Ошибка создания синхронизационных объектов!\n");
        return 1;
    }

    // Создание потоков
    HANDLE producers[PRODUCERS];
    HANDLE consumers[CONSUMERS];

    for (int i = 0; i < PRODUCERS; i++) {
        producers[i] = CreateThread(NULL, 0, Producer, (LPVOID)i, 0, NULL);
        if (producers[i] == NULL) {
            printf("Ошибка создания потока производителя %d!\n", i);
            return 1;
        }
    }

    for (int i = 0; i < CONSUMERS; i++) {
        consumers[i] = CreateThread(NULL, 0, Consumer, (LPVOID)i, 0, NULL);
        if (consumers[i] == NULL) {
            printf("Ошибка создания потока потребителя %d!\n", i);
            return 1;
        }
    }

    // Ожидание завершения работы потоков
    WaitForMultipleObjects(PRODUCERS, producers, TRUE, INFINITE);
    WaitForMultipleObjects(CONSUMERS, consumers, TRUE, INFINITE);

    // Освобождение ресурсов
    CloseHandle(mutex);
    CloseHandle(dataAvailable);
    CloseHandle(spaceAvailable);

    printf("Total sum: %d\n", sum);
    return 0;
}

// Функция производителя
DWORD WINAPI Producer(LPVOID lpParam) {
    int id = (int)lpParam;
    while (sum <= TARGET_SUM) {
        // Генерация случайного числа от 1 до 100
        int number = rand() % 100 + 1;

        // Ожидание свободного места в очереди
        WaitForSingleObject(spaceAvailable, INFINITE);

        // Блокировка мьютекса для безопасного доступа к очереди
        WaitForSingleObject(mutex, INFINITE);

        // Добавление элемента в очередь
        if (count < QUEUE_SIZE) {
            queue[count++] = number;
            printf("Producer [%d]: added %d (current queue size: %d)\n", id, number, count);
        }

        // Освобождение мьютекса
        ReleaseMutex(mutex);

        // Сигнализируем, что данные доступны для потребителей
        SetEvent(dataAvailable);

        // Задержка, чтобы симулировать время работы
        Sleep(rand() % 100);
    }
    return 0;
}

// Функция потребителя
DWORD WINAPI Consumer(LPVOID lpParam) {
    int id = (int)lpParam;
    while (sum <= TARGET_SUM) {
        // Ожидание данных в очереди
        WaitForSingleObject(dataAvailable, INFINITE);

        // Блокировка мьютекса для безопасного доступа к очереди
        WaitForSingleObject(mutex, INFINITE);

        // Извлечение элемента из очереди
        if (count > 0) {
            int number = queue[--count];
            sum += number;
            printf("Consumer [%d]: took %d (current sum: %d)\n", id, number, sum);
        }

        // Освобождение мьютекса
        ReleaseMutex(mutex);

        // Если очередь пуста, сбрасываем событие
        if (count == 0) {
            ResetEvent(dataAvailable);
        }

        // Освобождение места для производителей
        SetEvent(spaceAvailable);

        // Задержка, чтобы симулировать время обработки
        Sleep(rand() % 100);
    }
    return 0;
}
