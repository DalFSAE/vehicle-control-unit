#include "can_term.h"
#include "dio.h"
#include "stm32f4xx_hal.h"
#include <string.h>

// Reserve the last sector for config storage — confirm this against your
// linker script / flash layout so it doesn't collide with program flash.
// STM32F407: sector 11 starts at 0x080E0000 (128KB sector).
#define CAN_TERM_FLASH_SECTOR   FLASH_SECTOR_11
#define CAN_TERM_FLASH_ADDR     0x080E0000U
#define CAN_TERM_MAGIC          0x43414E31U // "CAN1"

typedef struct {
    uint32_t magic;
    uint8_t  can1_terminated;
    uint8_t  can2_terminated;
    uint8_t  _pad[2];
} CanTermFlashRecord_t;

static CanTermConfig_t s_cfg;

static void apply_gpio(const CanTermConfig_t *cfg) {
    dio_write(CAN1_TERMINATION, cfg->can1_terminated);
    dio_write(CAN2_TERMINATION, cfg->can2_terminated);
}

void can_term_init(void) {
    const CanTermFlashRecord_t *rec = (const CanTermFlashRecord_t *)CAN_TERM_FLASH_ADDR;

    if (rec->magic == CAN_TERM_MAGIC) {
        s_cfg.can1_terminated = rec->can1_terminated != 0u;
        s_cfg.can2_terminated = rec->can2_terminated != 0u;
    } else {
        // Flash blank/uninitialized — pick a safe default.
        s_cfg.can1_terminated = false;
        s_cfg.can2_terminated = false;
    }

    apply_gpio(&s_cfg);
}

CanTermConfig_t can_term_get(void) {
    return s_cfg;
}

bool can_term_set(const CanTermConfig_t *cfg) {
    if (cfg == NULL) return false;

    CanTermFlashRecord_t rec = {
        .magic            = CAN_TERM_MAGIC,
        .can1_terminated  = cfg->can1_terminated ? 1u : 0u,
        .can2_terminated  = cfg->can2_terminated ? 1u : 0u,
    };

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = CAN_TERM_FLASH_SECTOR,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
    };
    uint32_t sector_error;
    bool ok = (HAL_FLASHEx_Erase(&erase, &sector_error) == HAL_OK);

    if (ok) {
        const uint32_t *src = (const uint32_t *)&rec;
        for (size_t i = 0; i < sizeof(rec) / sizeof(uint32_t); i++) {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                   CAN_TERM_FLASH_ADDR + (i * sizeof(uint32_t)),
                                   src[i]) != HAL_OK) {
                ok = false;
                break;
            }
        }
    }

    HAL_FLASH_Lock();

    if (ok) {
        s_cfg = *cfg;
        apply_gpio(&s_cfg);
    }

    return ok;
}