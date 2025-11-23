#include "bl-flash.h"
#include "core/system.h"

#define __IO volatile /*!< Defines 'read / write' permissions */

typedef enum {
    FLASH_PROC_NONE       = 0,
    FLASH_PROC_PAGEERASE  = 1,
    FLASH_PROC_PROGRAM    = 2,
} FLASH_ProcedureTypeDef;

typedef enum {
    HAL_UNLOCKED = 0x00U,
    HAL_LOCKED   = 0x01U
} HAL_LockTypeDef;

typedef struct {
    __IO FLASH_ProcedureTypeDef ProcedureOnGoing; /*!< Internal variable to indicate which procedure is ongoing or not in IT context */
    __IO uint32_t               NbPagesToErase;   /*!< Internal variable to save the remaining sectors to erase in IT context*/
    __IO uint32_t               Address;          /*!< Internal variable to save address selected for program or erase */
    __IO uint32_t               Page;             /*!< Internal variable to define the current page which is erasing */
    HAL_LockTypeDef             Lock;             /*!< FLASH locking object */
    __IO uint32_t               ErrorCode;        /*!< FLASH error code. This parameter can be a value of @ref FLASH_Error_Codes  */
} FLASH_ProcessTypeDef;

FLASH_ProcessTypeDef pFlash;

#define FLASH_TIMEOUT_VALUE (50000U) /* 50 s */

#define FLASH_PAGE_SIZE (128U) /*!< FLASH Page Size in bytes */

#define PERIPH_BASE           (0x40000000UL) /*!< Peripheral base address in the alias region */
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x00020000UL)
#define FLASH_R_BASE          (AHBPERIPH_BASE + 0x00002000UL) /*!< FLASH registers base address */

typedef struct {
    __IO uint32_t ACR;           /*!< Access control register,                     Address offset: 0x00 */
    __IO uint32_t PECR;          /*!< Program/erase control register,              Address offset: 0x04 */
    __IO uint32_t PDKEYR;        /*!< Power down key register,                     Address offset: 0x08 */
    __IO uint32_t PEKEYR;        /*!< Program/erase key register,                  Address offset: 0x0c */
    __IO uint32_t PRGKEYR;       /*!< Program memory key register,                 Address offset: 0x10 */
    __IO uint32_t OPTKEYR;       /*!< Option byte key register,                    Address offset: 0x14 */
    __IO uint32_t SR;            /*!< Status register,                             Address offset: 0x18 */
    __IO uint32_t OPTR;          /*!< Option byte register,                        Address offset: 0x1c */
    __IO uint32_t WRPR;          /*!< Write protection register,                   Address offset: 0x20 */
} FLASH_TypeDef;
#define FLASH ((FLASH_TypeDef *) FLASH_R_BASE)

#define FLASH_PECR_PROG_Pos          (3U)     
#define FLASH_PECR_PROG_Msk          (0x1UL << FLASH_PECR_PROG_Pos) /*!< 0x00000008 */
#define FLASH_PECR_PROG FLASH_PECR_PROG_Msk /*!< Program matrix selection */

#define FLASH_PECR_ERASE_Pos         (9U)     
#define FLASH_PECR_ERASE_Msk         (0x1UL << FLASH_PECR_ERASE_Pos) /*!< 0x00000200 */
#define FLASH_PECR_ERASE             FLASH_PECR_ERASE_Msk /*!< Page erasing mode */

/******************  Bit definition for FLASH_SR register  *******************/
#define FLASH_SR_BSY_Pos             (0U)     
#define FLASH_SR_BSY_Msk             (0x1UL << FLASH_SR_BSY_Pos)                /*!< 0x00000001 */
#define FLASH_SR_BSY                 FLASH_SR_BSY_Msk                           /*!< Busy */
#define FLASH_SR_EOP_Pos             (1U)     
#define FLASH_SR_EOP_Msk             (0x1UL << FLASH_SR_EOP_Pos)                /*!< 0x00000002 */
#define FLASH_SR_EOP                 FLASH_SR_EOP_Msk                           /*!< End Of Programming*/
#define FLASH_SR_HVOFF_Pos           (2U)     
#define FLASH_SR_HVOFF_Msk           (0x1UL << FLASH_SR_HVOFF_Pos)              /*!< 0x00000004 */
#define FLASH_SR_HVOFF               FLASH_SR_HVOFF_Msk                         /*!< End of high voltage */
#define FLASH_SR_READY_Pos           (3U)     
#define FLASH_SR_READY_Msk           (0x1UL << FLASH_SR_READY_Pos)              /*!< 0x00000008 */
#define FLASH_SR_READY               FLASH_SR_READY_Msk                         /*!< Flash ready after low power mode */

/******************  Bit definition for FLASH_SR register  *******************/
#define FLASH_SR_BSY_Pos             (0U)     
#define FLASH_SR_BSY_Msk             (0x1UL << FLASH_SR_BSY_Pos)                /*!< 0x00000001 */
#define FLASH_SR_BSY                 FLASH_SR_BSY_Msk                           /*!< Busy */
#define FLASH_SR_EOP_Pos             (1U)     
#define FLASH_SR_EOP_Msk             (0x1UL << FLASH_SR_EOP_Pos)                /*!< 0x00000002 */
#define FLASH_SR_EOP                 FLASH_SR_EOP_Msk                           /*!< End Of Programming*/
#define FLASH_SR_HVOFF_Pos           (2U)     
#define FLASH_SR_HVOFF_Msk           (0x1UL << FLASH_SR_HVOFF_Pos)              /*!< 0x00000004 */
#define FLASH_SR_HVOFF               FLASH_SR_HVOFF_Msk                         /*!< End of high voltage */
#define FLASH_SR_READY_Pos           (3U)     
#define FLASH_SR_READY_Msk           (0x1UL << FLASH_SR_READY_Pos)              /*!< 0x00000008 */
#define FLASH_SR_READY               FLASH_SR_READY_Msk                         /*!< Flash ready after low power mode */

#define FLASH_SR_WRPERR_Pos          (8U)     
#define FLASH_SR_WRPERR_Msk          (0x1UL << FLASH_SR_WRPERR_Pos)             /*!< 0x00000100 */
#define FLASH_SR_WRPERR              FLASH_SR_WRPERR_Msk                        /*!< Write protection error */
#define FLASH_SR_PGAERR_Pos          (9U)     
#define FLASH_SR_PGAERR_Msk          (0x1UL << FLASH_SR_PGAERR_Pos)             /*!< 0x00000200 */
#define FLASH_SR_PGAERR              FLASH_SR_PGAERR_Msk                        /*!< Programming Alignment Error */
#define FLASH_SR_SIZERR_Pos          (10U)    
#define FLASH_SR_SIZERR_Msk          (0x1UL << FLASH_SR_SIZERR_Pos)             /*!< 0x00000400 */
#define FLASH_SR_SIZERR              FLASH_SR_SIZERR_Msk                        /*!< Size error */
#define FLASH_SR_OPTVERR_Pos         (11U)    
#define FLASH_SR_OPTVERR_Msk         (0x1UL << FLASH_SR_OPTVERR_Pos)            /*!< 0x00000800 */
#define FLASH_SR_OPTVERR             FLASH_SR_OPTVERR_Msk                       /*!< Option Valid error */
#define FLASH_SR_RDERR_Pos           (13U)    
#define FLASH_SR_RDERR_Msk           (0x1UL << FLASH_SR_RDERR_Pos)              /*!< 0x00002000 */
#define FLASH_SR_RDERR               FLASH_SR_RDERR_Msk                         /*!< Read protected error */
#define FLASH_SR_NOTZEROERR_Pos      (16U)    
#define FLASH_SR_NOTZEROERR_Msk      (0x1UL << FLASH_SR_NOTZEROERR_Pos)         /*!< 0x00010000 */
#define FLASH_SR_NOTZEROERR          FLASH_SR_NOTZEROERR_Msk                    /*!< Not Zero error */
#define FLASH_SR_FWWERR_Pos          (17U)    
#define FLASH_SR_FWWERR_Msk          (0x1UL << FLASH_SR_FWWERR_Pos)             /*!< 0x00020000 */
#define FLASH_SR_FWWERR              FLASH_SR_FWWERR_Msk                        /*!< Write/Errase operation aborted */

#define FLASH_FLAG_BSY             FLASH_SR_BSY        /*!< FLASH Busy flag */
#define FLASH_FLAG_EOP             FLASH_SR_EOP        /*!< FLASH End of Programming flag */
#define FLASH_FLAG_ENDHV           FLASH_SR_HVOFF      /*!< FLASH End of High Voltage flag */
#define FLASH_FLAG_READY           FLASH_SR_READY      /*!< FLASH Ready flag after low power mode */
#define FLASH_FLAG_WRPERR          FLASH_SR_WRPERR     /*!< FLASH Write protected error flag */
#define FLASH_FLAG_PGAERR          FLASH_SR_PGAERR     /*!< FLASH Programming Alignment error flag */
#define FLASH_FLAG_SIZERR          FLASH_SR_SIZERR     /*!< FLASH Size error flag  */
#define FLASH_FLAG_OPTVERR         FLASH_SR_OPTVERR    /*!< FLASH Option Validity error flag  */
#define FLASH_FLAG_RDERR           FLASH_SR_RDERR      /*!< FLASH Read protected error flag */
#define FLASH_FLAG_FWWERR          FLASH_SR_FWWERR     /*!< FLASH Write or Errase operation aborted */
#define FLASH_FLAG_NOTZEROERR      FLASH_SR_NOTZEROERR /*!< FLASH Read protected error flag */

#define HAL_MAX_DELAY 0xFFFFFFFFU

#define __HAL_FLASH_GET_FLAG(__FLAG__)      (((FLASH->SR) & (__FLAG__)) == (__FLAG__))
#define __HAL_FLASH_CLEAR_FLAG(__FLAG__)    ((FLASH->SR) = (__FLAG__))

#define HAL_FLASH_ERROR_NONE      0x00U  /*!< No error */
#define HAL_FLASH_ERROR_PGA       0x01U  /*!< Programming alignment error */
#define HAL_FLASH_ERROR_WRP       0x02U  /*!< Write protection error */
#define HAL_FLASH_ERROR_OPTV      0x04U  /*!< Option validity error */
#define HAL_FLASH_ERROR_SIZE      0x08U  /*!<  */
#define HAL_FLASH_ERROR_RD        0x10U  /*!< Read protected error */
#define HAL_FLASH_ERROR_FWWERR    0x20U  /*!< FLASH Write or Erase operation aborted */
#define HAL_FLASH_ERROR_NOTZERO   0x40U  /*!< FLASH Write operation is done in a not-erased region */

#define FLASHSIZE_BASE        (0x1FF8007CUL)        /*!< FLASH Size register base address */

#define FLASH_SIZE            (uint32_t)((*((uint32_t *)FLASHSIZE_BASE)&0xFFFF) * 1024U)
#define FLASH_PAGE_SIZE       (128U)  /*!< FLASH Page Size in bytes */
#define FLASH_NBPAGES_MAX     (FLASH_SIZE / FLASH_PAGE_SIZE)
#define IS_NBPAGES(__PAGES__) (((__PAGES__) >= 1) && ((__PAGES__) <= FLASH_NBPAGES_MAX))

#define FLASH_TYPEERASE_PAGES           (0x00U)  /*!<Page erase only*/
#define IS_FLASH_TYPEERASE(__VALUE__)   (((__VALUE__) == FLASH_TYPEERASE_PAGES))

#define FLASH_SIZE                                  (uint32_t)((*((uint32_t *)FLASHSIZE_BASE)&0xFFFF) * 1024U)
#define FLASH_BASE                                  (0x08000000UL) /*!< FLASH base address in the alias region */
#define IS_FLASH_PROGRAM_ADDRESS(__ADDRESS__)       (((__ADDRESS__) >= FLASH_BASE)       && ((__ADDRESS__) <  (FLASH_BASE + FLASH_SIZE)))

#define assert_param(expr) ((void)0U)

#define SET_BIT(REG, BIT) ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))

#define __HAL_LOCK(__HANDLE__)                                                 \
                                do{                                            \
                                    if((__HANDLE__)->Lock == HAL_LOCKED)       \
                                    {                                          \
                                       return HAL_BUSY;                        \
                                    }                                          \
                                    else                                       \
                                    {                                          \
                                       (__HANDLE__)->Lock = HAL_LOCKED;        \
                                    }                                          \
                                }while (0)

#define __HAL_UNLOCK(__HANDLE__)                                             \
                                do{                                          \
                                    (__HANDLE__)->Lock = HAL_UNLOCKED;       \
                                }while (0)

#define FLASH_TYPEERASE_PAGES (0x00U)  /*!<Page erase only*/

/*******************  Bit definition for FLASH_PECR register  ******************/
#define FLASH_PECR_PELOCK_Pos        (0U)     
#define FLASH_PECR_PELOCK_Msk        (0x1UL << FLASH_PECR_PELOCK_Pos)           /*!< 0x00000001 */
#define FLASH_PECR_PELOCK            FLASH_PECR_PELOCK_Msk                      /*!< FLASH_PECR and Flash data Lock */
#define FLASH_PECR_PRGLOCK_Pos       (1U)     
#define FLASH_PECR_PRGLOCK_Msk       (0x1UL << FLASH_PECR_PRGLOCK_Pos)          /*!< 0x00000002 */
#define FLASH_PECR_PRGLOCK           FLASH_PECR_PRGLOCK_Msk                     /*!< Program matrix Lock */
#define FLASH_PECR_OPTLOCK_Pos       (2U)     
#define FLASH_PECR_OPTLOCK_Msk       (0x1UL << FLASH_PECR_OPTLOCK_Pos)          /*!< 0x00000004 */
#define FLASH_PECR_OPTLOCK           FLASH_PECR_OPTLOCK_Msk                     /*!< Option byte matrix Lock */
#define FLASH_PECR_PROG_Pos          (3U)     
#define FLASH_PECR_PROG_Msk          (0x1UL << FLASH_PECR_PROG_Pos)             /*!< 0x00000008 */
#define FLASH_PECR_PROG              FLASH_PECR_PROG_Msk                        /*!< Program matrix selection */
#define FLASH_PECR_DATA_Pos          (4U)     
#define FLASH_PECR_DATA_Msk          (0x1UL << FLASH_PECR_DATA_Pos)             /*!< 0x00000010 */
#define FLASH_PECR_DATA              FLASH_PECR_DATA_Msk                        /*!< Data matrix selection */
#define FLASH_PECR_FIX_Pos           (8U)     
#define FLASH_PECR_FIX_Msk           (0x1UL << FLASH_PECR_FIX_Pos)              /*!< 0x00000100 */
#define FLASH_PECR_FIX               FLASH_PECR_FIX_Msk                         /*!< Fixed Time Data write for Word/Half Word/Byte programming */
#define FLASH_PECR_ERASE_Pos         (9U)     
#define FLASH_PECR_ERASE_Msk         (0x1UL << FLASH_PECR_ERASE_Pos)            /*!< 0x00000200 */
#define FLASH_PECR_ERASE             FLASH_PECR_ERASE_Msk                       /*!< Page erasing mode */
#define FLASH_PECR_FPRG_Pos          (10U)    
#define FLASH_PECR_FPRG_Msk          (0x1UL << FLASH_PECR_FPRG_Pos)             /*!< 0x00000400 */
#define FLASH_PECR_FPRG              FLASH_PECR_FPRG_Msk                        /*!< Fast Page/Half Page programming mode */
#define FLASH_PECR_EOPIE_Pos         (16U)    
#define FLASH_PECR_EOPIE_Msk         (0x1UL << FLASH_PECR_EOPIE_Pos)            /*!< 0x00010000 */
#define FLASH_PECR_EOPIE             FLASH_PECR_EOPIE_Msk                       /*!< End of programming interrupt */ 
#define FLASH_PECR_ERRIE_Pos         (17U)    
#define FLASH_PECR_ERRIE_Msk         (0x1UL << FLASH_PECR_ERRIE_Pos)            /*!< 0x00020000 */
#define FLASH_PECR_ERRIE             FLASH_PECR_ERRIE_Msk                       /*!< Error interrupt */ 
#define FLASH_PECR_OBL_LAUNCH_Pos    (18U)    
#define FLASH_PECR_OBL_LAUNCH_Msk    (0x1UL << FLASH_PECR_OBL_LAUNCH_Pos)       /*!< 0x00040000 */
#define FLASH_PECR_OBL_LAUNCH        FLASH_PECR_OBL_LAUNCH_Msk                  /*!< Launch the option byte loading */
#define FLASH_PECR_HALF_ARRAY_Pos    (19U)    
#define FLASH_PECR_HALF_ARRAY_Msk    (0x1UL << FLASH_PECR_HALF_ARRAY_Pos)       /*!< 0x00080000 */
#define FLASH_PECR_HALF_ARRAY        FLASH_PECR_HALF_ARRAY_Msk  

#define HAL_IS_BIT_SET(REG, BIT) (((REG) & (BIT)) == (BIT))
#define HAL_IS_BIT_CLR(REG, BIT) (((REG) & (BIT)) == 0U)

#define BOOTLOADER_SIZE (0x8000U) // 32 KByte (32768 Byte)
#define MAIN_APPLICATION_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE) // 0x08000000 + 0x8000 (0x08008000)

#define FLASH_PEKEY1 (0x89ABCDEFU) /*!< Flash program erase key1 */
#define FLASH_PEKEY2 (0x02030405U) /*!< Flash program erase key: used with FLASH_PEKEY2 to unlock the write access to the FLASH_PECR register and data EEPROM */

#define FLASH_PRGKEY1 (0x8C9DAEBFU) /*!< Flash program memory key1 */
#define FLASH_PRGKEY2 (0x13141516U) /*!< Flash program memory key2: used with FLASH_PRGKEY2 to unlock the program memory */

#define WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define __STATIC_FORCEINLINE __attribute__((always_inline)) static inline
#define __ASM __asm

#define UNUSED(X) (void)X /* To avoid gcc/g++ warnings */

#define IS_FLASH_TYPEPROGRAM(_VALUE_) ((_VALUE_) == FLASH_TYPEPROGRAM_WORD)

uint32_t HAL_FLASH_GetError(void) {
   return pFlash.ErrorCode;
}

static void FLASH_SetErrorCode(void) {
    uint32_t flags = 0;
    
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR)) {
        pFlash.ErrorCode |= HAL_FLASH_ERROR_WRP;
        flags |= FLASH_FLAG_WRPERR;
    }
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_PGAERR)) {
        pFlash.ErrorCode |= HAL_FLASH_ERROR_PGA;
        flags |= FLASH_FLAG_PGAERR;
    }
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_SIZERR)) { 
        pFlash.ErrorCode |= HAL_FLASH_ERROR_SIZE;
        flags |= FLASH_FLAG_SIZERR;
    }
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR)) {
        /* WARNING : On the first cut of STM32L031xx and STM32L041xx devices,
        *           (RefID = 0x1000) the FLASH_FLAG_OPTVERR bit was not behaving
        *           as expected. If the user run an application using the first
        *           cut of the STM32L031xx device or the first cut of the STM32L041xx
        *           device, this error should be ignored. The revId of the device
        *           can be retrieved via the HAL_GetREVID() function. */
        pFlash.ErrorCode |= HAL_FLASH_ERROR_OPTV;
        flags |= FLASH_FLAG_OPTVERR;
    }

    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_RDERR)) {
        pFlash.ErrorCode |= HAL_FLASH_ERROR_RD;
        flags |= FLASH_FLAG_RDERR;
    }
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_FWWERR)) { 
        pFlash.ErrorCode |= HAL_FLASH_ERROR_FWWERR;
        flags |= HAL_FLASH_ERROR_FWWERR;
    }
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_NOTZEROERR)) { 
        pFlash.ErrorCode |= HAL_FLASH_ERROR_NOTZERO;
        flags |= FLASH_FLAG_NOTZEROERR;
    }

    /* Clear FLASH error pending bits */
    __HAL_FLASH_CLEAR_FLAG(flags);
} 

HAL_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout) {
    /* Wait for the FLASH operation to complete by polling on BUSY flag to be reset. Even if the FLASH operation fails, the BUSY flag will be reset and an error flag will be set */
     
    uint32_t tickstart = system_get_ticks();
     
    while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) { 
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((system_get_ticks()-tickstart) > Timeout)) {
                return HAL_TIMEOUT;
            }
        }
    }
  
    /* Check FLASH End of Operation flag  */
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP)) {
        /* Clear FLASH End of Operation pending bit */
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);
    }
  
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR)     || 
       __HAL_FLASH_GET_FLAG(FLASH_FLAG_PGAERR)     || 
       __HAL_FLASH_GET_FLAG(FLASH_FLAG_SIZERR)     || 
       __HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR)    || 
       __HAL_FLASH_GET_FLAG(FLASH_FLAG_RDERR)      || 
       __HAL_FLASH_GET_FLAG(FLASH_FLAG_FWWERR)     || 
       __HAL_FLASH_GET_FLAG(FLASH_FLAG_NOTZEROERR) ) {
        /*Save the error code*/

        /* WARNING : On the first cut of STM32L031xx and STM32L041xx devices,
        *           (RefID = 0x1000) the FLASH_FLAG_OPTVERR bit was not behaving
        *           as expected. If the user run an application using the first
        *           cut of the STM32L031xx device or the first cut of the STM32L041xx
        *           device, this error should be ignored. The revId of the device
        *           can be retrieved via the HAL_GetREVID() function. */
        
        FLASH_SetErrorCode();
        return HAL_ERROR;
    }

    /* There is no error flag set */
    return HAL_OK;
}

void FLASH_PageErase(uint32_t PageAddress) {
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /* Set the ERASE bit */
    SET_BIT(FLASH->PECR, FLASH_PECR_ERASE);

    /* Set PROG bit */
    SET_BIT(FLASH->PECR, FLASH_PECR_PROG);

    /* Write 00000000h to the first word of the program page to erase */
    *(__IO uint32_t *)(uint32_t)(PageAddress & ~(FLASH_PAGE_SIZE - 1)) = 0x00000000;
}

__STATIC_FORCEINLINE uint32_t __get_PRIMASK(void) {
    uint32_t result;
    __ASM volatile ("MRS %0, primask" : "=r" (result) :: "memory");
    return(result);
}

__STATIC_FORCEINLINE void __disable_irq(void) {
  __ASM volatile ("cpsid i" : : : "memory");
}

__STATIC_FORCEINLINE void __set_PRIMASK(uint32_t priMask) {
  __ASM volatile ("MSR primask, %0" : : "r" (priMask) : "memory");
}

HAL_StatusTypeDef HAL_FLASH_Unlock(void) {
    uint32_t primask_bit;

    /* Unlocking FLASH_PECR register access*/
    if (HAL_IS_BIT_SET(FLASH->PECR, FLASH_PECR_PELOCK)) {
        /* Disable interrupts to avoid any interruption during unlock sequence */
        primask_bit = __get_PRIMASK();
        __disable_irq();

        WRITE_REG(FLASH->PEKEYR, FLASH_PEKEY1);
        WRITE_REG(FLASH->PEKEYR, FLASH_PEKEY2);

        /* Re-enable the interrupts: restore previous priority mask */
        __set_PRIMASK(primask_bit);

        if (HAL_IS_BIT_SET(FLASH->PECR, FLASH_PECR_PELOCK)) {
            return HAL_ERROR;
        }
    }

    if (HAL_IS_BIT_SET(FLASH->PECR, FLASH_PECR_PRGLOCK)) {
        /* Disable interrupts to avoid any interruption during unlock sequence */
        primask_bit = __get_PRIMASK();
        __disable_irq();

        /* Unlocking the program memory access */
        WRITE_REG(FLASH->PRGKEYR, FLASH_PRGKEY1);
        WRITE_REG(FLASH->PRGKEYR, FLASH_PRGKEY2);  

        /* Re-enable the interrupts: restore previous priority mask */
        __set_PRIMASK(primask_bit);

        if (HAL_IS_BIT_SET(FLASH->PECR, FLASH_PECR_PRGLOCK)) {
            return HAL_ERROR;
        }
    } 

    return HAL_OK; 
}

HAL_StatusTypeDef HAL_FLASH_Lock(void) {
    /* Set the PRGLOCK Bit to lock the FLASH Registers access */
    SET_BIT(FLASH->PECR, FLASH_PECR_PRGLOCK);
  
    /* Set the PELOCK Bit to lock the PECR Register access */
    SET_BIT(FLASH->PECR, FLASH_PECR_PELOCK);

    return HAL_OK;  
}

HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *PageError) {
    HAL_StatusTypeDef status;
    uint32_t address = 0U;
  
    /* Process Locked */
    __HAL_LOCK(&pFlash);

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    if (status == HAL_OK) {
        /*Initialization of PageError variable*/
        *PageError = 0xFFFFFFFFU;

        /* Check the parameters */
        assert_param(IS_NBPAGES(pEraseInit->NbPages));
        assert_param(IS_FLASH_TYPEERASE(pEraseInit->TypeErase));
        assert_param(IS_FLASH_PROGRAM_ADDRESS(pEraseInit->PageAddress));
        assert_param(IS_FLASH_PROGRAM_ADDRESS((pEraseInit->PageAddress & ~(FLASH_PAGE_SIZE - 1U)) + pEraseInit->NbPages * FLASH_PAGE_SIZE - 1U));

        /* Erase page by page to be done*/
        for (address = pEraseInit->PageAddress; address < ((pEraseInit->NbPages * FLASH_PAGE_SIZE) + pEraseInit->PageAddress); address += FLASH_PAGE_SIZE) {
            FLASH_PageErase(address);

            /* Wait for last operation to be completed */
            status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

            /* If the erase operation is completed, disable the ERASE Bit */
            CLEAR_BIT(FLASH->PECR, FLASH_PECR_PROG);
            CLEAR_BIT(FLASH->PECR, FLASH_PECR_ERASE);

            if (status != HAL_OK) {
                /* In case of error, stop erase procedure and return the faulty address */
                *PageError = address;
                break;
            }
        }
    }

    /* Process Unlocked */
    __HAL_UNLOCK(&pFlash);

    return status;
}

HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t Data) {
    HAL_StatusTypeDef status;
  
    /* Process Locked */
    __HAL_LOCK(&pFlash);

    /* Check the parameters */
    assert_param(IS_FLASH_TYPEPROGRAM(TypeProgram));
    assert_param(IS_FLASH_PROGRAM_ADDRESS(Address));

    /* Prevent unused argument(s) compilation warning */
    UNUSED(TypeProgram);

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
    if (status == HAL_OK) {
        /* Clean the error context */
        pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

        /*Program word (32-bit) at a specified address.*/
        *(__IO uint32_t *)Address = Data;

        /* Wait for last operation to be completed */
        status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    }

    /* Process Unlocked */
    __HAL_UNLOCK(&pFlash);

    return status;
}

uint32_t BL_FLASH_ERASE_Main_Application(void) {
    static FLASH_EraseInitTypeDef EraseInit;
    uint32_t PageError;

    EraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInit.PageAddress = MAIN_APPLICATION_START_ADDRESS;
    EraseInit.NbPages = 256;

    HAL_FLASH_Unlock();
    HAL_FLASHEx_Erase(&EraseInit, &PageError);
    HAL_FLASH_Lock();

    return 0;
}
