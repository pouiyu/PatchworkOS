#include <kernel/power.h>
#include <kernel/log/log.h>
#include <kernel/cpu/cpu.h>

#include <stdint.h>

// 声明 I/O 端口函数
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

// 简化的 ACPI 关机实现
static void acpi_power_off_simple(void)
{
    LOG_INFO("Attempting ACPI power off...\n");
    
    // 常见的 ACPI PM1a_CNT 端口地址
    uint16_t pm1a_cnt_ports[] = {0x1800, 0x2000, 0x4000, 0x604, 0xB004};
    
    for (int i = 0; i < 5; i++) {
        uint16_t port = pm1a_cnt_ports[i];
        LOG_INFO("Trying PM1a_CNT at 0x%x\n", port);
        
        // SLP_EN = 1 (bit 13), SLP_TYP = 0 (S5, bits 10-12)
        uint16_t s5_value = (1 << 13) | (0 << 10);
        outw(port, s5_value);
        
        // 等待一下看是否关机
        for (int j = 0; j < 100000; j++) {
            __asm__ volatile("pause");
        }
    }
}

void power_off(void)
{
    LOG_INFO("Powering off system...\n");
    
    // 停止其他 CPU
    cpu_halt_others();
    LOG_INFO("Other CPUs halted\n");
    
    // 尝试 ACPI 关机
    acpi_power_off_simple();
    
    // 尝试键盘控制器关机
    LOG_WARN("Trying keyboard controller...\n");
    outb(0x64, 0xFE);
    
    // 最后手段：死循环
    LOG_WARN("Failed to power off, halting CPU\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}

void reboot(void)
{
    LOG_INFO("Rebooting system...\n");
    
    // 尝试键盘控制器重启
    LOG_INFO("Trying keyboard controller reset...\n");
    outb(0x64, 0xFE);
    
    // 使用简单的方法：跳转到 BIOS 入口点
    // 通过设置 CS:IP 为 0xFFFF:0x0000 来触发重启
    void (*reset)(void) = (void (*)(void))0xFFFF0;
    reset();
    
    // 如果上面的方法失败，无限循环
    while (1) {
        __asm__ volatile("hlt");
    }
}
