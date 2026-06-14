#ifndef _KERNEL_POWER_H
#define _KERNEL_POWER_H 1

#include <stdint.h>

// 电源状态
#define POWER_STATE_S0 0   // 正常工作
#define POWER_STATE_S3 3   // 挂起到内存
#define POWER_STATE_S4 4   // 挂起到磁盘
#define POWER_STATE_S5 5   // 软关机

// 函数声明
void power_off(void);
void reboot(void);
void power_reset(void);

// ACPI 相关函数
void acpi_power_off(void);
void acpi_reboot(void);

#endif
