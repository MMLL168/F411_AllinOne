/*
 * Syscall.c
 *
 *  Created on: Aug 8, 2025
 *      Author: marlonwu
 */

    /* USER CODE BEGIN 0 */


#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

// #warning "syscalls.c is being compiled" // 可選：取消註解這行，如果在 Console 看到警告，代表檔案有被編譯

#include "main.h"
extern UART_HandleTypeDef huart6;

/*
 * 這些是未實作的函式存根。
 * 當應用程式連結到 newlib-nano 函式庫但沒有提供這些底層實作時，
 * 就需要這些存根來避免連結器錯誤。
 */

int _close(int file) {
    return -1;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

int _read(int file, char *ptr, int len) {
    return 0;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _write(int file, char *ptr, int len)
{
  // 只處理標準輸出 (STDOUT) 和標準錯誤 (STDERR)
  if (file == 1 || file == 2)
  {
    // 將數據透過 huart6 傳送出去
    // 注意，我們傳遞的是 huart6 的位址: &huart6
    // 最後一個參數是超時時間，設定 100ms 即可
    HAL_UART_Transmit(&huart6, (uint8_t*)ptr, len, 100);
  }
  return len;
}

// 其他可能需要的存根
int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

void _exit(int status) {
    _kill(status, -1);
    while (1) {} // 無限迴圈
}
/* USER CODE END 3 */
