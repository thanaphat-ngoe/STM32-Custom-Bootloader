#ifndef INC_BL_FLASH_H
#define INC_BL_FLASH_H
 
#include "common-defines.h"

#define FLASH_TYPEPROGRAM_WORD (0x02U)  /*!<Program a word (32-bit) at a specified address.*/

typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

typedef struct {
    uint32_t TypeErase;   /*!< TypeErase: Page Erase only. This parameter can be a value of @ref FLASHEx_Type_Erase */
    uint32_t PageAddress; /*!< PageAddress: Initial FLASH address to be erased. This parameter must be a value belonging to FLASH Program address (depending on the devices)  */
    uint32_t NbPages;     /*!< NbPages: Number of pages to be erased. This parameter must be a value between 1 and (max number of pages - value of Initial page)*/
} FLASH_EraseInitTypeDef;

uint32_t BL_FLASH_ERASE_Main_Application(void);

uint32_t HAL_FLASH_GetError(void);


HAL_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout);

HAL_StatusTypeDef HAL_FLASH_Unlock(void);
HAL_StatusTypeDef HAL_FLASH_Lock(void);

void FLASH_PageErase(uint32_t PageAddress);
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *PageError);
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t Data);

#endif
