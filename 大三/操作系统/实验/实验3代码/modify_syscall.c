#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>

// 定义要替换的系统调用编号
#define TARGET_SYSCALL 96

// 保存旧的系统调用地址和相关信息
static unsigned long original_syscall_func;
static unsigned long syscall_table_address; 
static unsigned long original_cr0;

// 清除写保护位，允许修改系统调用表
static unsigned int disable_wp(void)
{
    unsigned int cr0_value = 0;
    unsigned int result;

    // 读取当前的 CR0 寄存器值
    asm volatile ("movq %%cr0, %%rax" : "=a"(cr0_value));  
    result = cr0_value;

    // 清除 CR0 中的写保护位
    cr0_value &= ~0x10000;
    asm volatile ("movq %%rax, %%cr0" :: "a"(cr0_value));  

    return result;
}

// 恢复 CR0 寄存器中的原始值
static void restore_wp(unsigned int value)
{
    asm volatile ("movq %%rax, %%cr0" :: "a"(value));  
}

// 新的系统调用函数，替代原有的 gettimeofday
asmlinkage int hello_syscall(struct pt_regs *regs)
{
    int param1 = regs->di;  // 第一个参数
    int param2 = regs->si;  // 第二个参数
    printk(KERN_INFO "System call 96 replaced with custom. Result of a + b: %d\n", param1 + param2);
    return param1 + param2;
}

// 修改系统调用表，将 syscall 96 指向新的函数
static void replace_syscall(void)
{
    unsigned long *syscall_entry;

    // 查找 sys_call_table 的地址
    syscall_table_address = (unsigned long *)kallsyms_lookup_name("sys_call_table");
    if (!syscall_table_address) {
        pr_err("Unable to find sys_call_table\n");
        return;
    }

    // 获取 syscall 96 的原始地址
    syscall_entry = (unsigned long *)(syscall_table_address + TARGET_SYSCALL * sizeof(void *));
    original_syscall_func = *(syscall_entry);

    // 禁用写保护，修改 sys_call_table
    original_cr0 = disable_wp();
    *(syscall_entry) = (unsigned long)&custom_syscall;  
    restore_wp(original_cr0);
}

// 恢复原始的系统调用，取消对 syscall 96 的替换
static void restore_original_syscall(void)
{
    unsigned long *syscall_entry;

    syscall_entry = (unsigned long *)(syscall_table_address + TARGET_SYSCALL * sizeof(void *));
    original_cr0 = disable_wp();
    *(syscall_entry) = original_syscall_func;  
    restore_wp(original_cr0);
}

// 模块初始化函数
static int __init syscall_modify_init(void)
{
    replace_syscall();  // 替换系统调用
    return 0;
}

// 模块退出函数，恢复原始系统调用
static void __exit syscall_modify_exit(void)
{
    restore_original_syscall();  
}

module_init(syscall_modify_init);
module_exit(syscall_modify_exit);
MODULE_LICENSE("GPL");
