#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

#include <stdint.h>

struct multiboot_module
{
    uint32_t mod_start; // void *
    uint32_t mod_end;   // void *
    uint32_t string;    // const char *
    uint8_t reserved[4];
} __attribute__((packed));

struct apm_table
{
    uint16_t version;
    uint16_t cseg;
    uint32_t offset; // void *
    uint16_t cseg16;
    uint16_t flags;
    uint16_t cseg_len;
    uint16_t cseg16_len;
    uint16_t dseg_len;
} __attribute__((packed));

struct memory_map
{
    uint32_t size;
    uint64_t base;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

struct multiboot_info
{
    uint32_t mi_flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;          // const char *
    uint32_t mods_count;
    uint32_t mods_addr;        // struct multiboot_module *
    uint32_t elfshdr_num;
    uint32_t elfshdr_size;
    uint32_t elfshdr_addr;
    uint32_t elfshdr_shndx;
    uint32_t mmap_length;
    uint32_t mmap_addr;        // struct memory_map *
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;     // void *
    uint32_t boot_loader_name; // const char *
    uint32_t apm_table;        // struct apm_table *
    uint32_t vbe_control_info; // void *
    uint32_t vbe_mode_info;    // void *
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} __attribute__((packed));

#endif
