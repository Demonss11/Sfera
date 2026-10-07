/*
 * pthread_win.c — минимальный слой совместимости POSIX pthreads поверх
 * Win32, необходимый программам на «Сфере» под Windows.
 *
 * Сгенерированный компилятором код ВСЕГДА создаёт глобальный mutex и
 * condition variable (см. sfera_детеризм_мьютекс / sfera_детеризм_cond в
 * .ll) и вызывает классические pthread_* функции. На Windows их нет,
 * поэтому нужен этот слой — он переводит вызовы на нативные
 * CRITICAL_SECTION / CONDITION_VARIABLE / CreateThread.
 *
 * Компилятор подключает этот файл к линковке автоматически (см.
 * main.rs, найти_шим_совместимости и слинковать_исполняемый_файл) —
 * clang умеет скомпилировать .c и слинковать его за один вызов, отдельный
 * объектный файл заранее не нужен.
 *
 * ⚠️ Проверено: буферы под mutex/cond, которые выделяет компилятор, имеют
 * размер 128 байт — CRITICAL_SECTION (40) и CONDITION_VARIABLE (8) в них
 * помещаются с запасом.
 */
#include <windows.h>
#include <stdint.h>

/* ---- mutex ---- */

int pthread_mutex_init(void *мьютекс, const void *атрибуты) {
    (void)атрибуты;
    InitializeCriticalSection((CRITICAL_SECTION *)мьютекс);
    return 0;
}

int pthread_mutex_lock(void *мьютекс) {
    EnterCriticalSection((CRITICAL_SECTION *)мьютекс);
    return 0;
}

int pthread_mutex_unlock(void *мьютекс) {
    LeaveCriticalSection((CRITICAL_SECTION *)мьютекс);
    return 0;
}

int pthread_mutex_destroy(void *мьютекс) {
    DeleteCriticalSection((CRITICAL_SECTION *)мьютекс);
    return 0;
}

/* ---- condition variable ---- */

int pthread_cond_init(void *условие, const void *атрибуты) {
    (void)атрибуты;
    InitializeConditionVariable((CONDITION_VARIABLE *)условие);
    return 0;
}

int pthread_cond_destroy(void *условие) {
    (void)условие;
    return 0;
}

int pthread_cond_signal(void *условие) {
    WakeConditionVariable((CONDITION_VARIABLE *)условие);
    return 0;
}

int pthread_cond_broadcast(void *условие) {
    WakeAllConditionVariable((CONDITION_VARIABLE *)условие);
    return 0;
}

int pthread_cond_wait(void *условие, void *мьютекс) {
    return SleepConditionVariableCS((CONDITION_VARIABLE *)условие,
                                    (CRITICAL_SECTION *)мьютекс, INFINITE)
               ? 0
               : 1;
}

/* ---- потоки (для «параллельно { }») ---- */

int pthread_create(void **поток, const void *атрибуты, void *(*функция)(void *),
                   void *аргумент) {
    (void)атрибуты;
    /* На x64 соглашения о вызовах start-функции потока и
     * LPTHREAD_START_ROUTINE совпадают — прямой каст корректен. */
    HANDLE h = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)функция, аргумент,
                            0, NULL);
    if (h == NULL) {
        return 1;
    }
    *поток = (void *)h;
    return 0;
}

int pthread_join(void *поток, void **результат) {
    if (результат != NULL) {
        *результат = NULL;
    }
    if (поток == NULL) {
        return 0;
    }
    WaitForSingleObject((HANDLE)поток, INFINITE);
    CloseHandle((HANDLE)поток);
    return 0;
}

/* ---- прочие POSIX-функции, которых нет в MSVCRT ---- */

void sleep(unsigned int секунды) {
    Sleep(секунды * 1000u);
}
