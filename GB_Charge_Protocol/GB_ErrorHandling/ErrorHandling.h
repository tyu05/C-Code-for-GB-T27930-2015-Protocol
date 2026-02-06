/*
 * ErrorHandling.h
 *
 *  Created on: 2025年7月24日
 *  Author: 杨燚帆
 *
 */

#ifndef __ERROR_HANDLING_H_
#define __ERROR_HANDLING_H_

#include <stdint.h>
#include <stdio.h>
#include "Hardware.h"
#include <string.h>

// 错误类型定义
typedef enum {
    ERROR_TYPE_HARDWARE,    // 硬件错误（包括传输层错误）
    ERROR_TYPE_COMMUNICATION // 通信错误（包括用户层错误）
} ErrorType_t;

// 硬件错误码定义（与传输层状态码保持一致）
typedef enum {
    ERROR_CODE_HW_TRANSPORT_ERROR = 1,        // 传输层错误
    ERROR_CODE_HW_TRANSPORT_BUSY = 2,          // 传输层忙
    ERROR_CODE_HW_TRANSPORT_TIMEOUT = 3,       // 传输层超时
    ERROR_CODE_HW_TRANSPORT_INVALID_PARAM = 4, // 传输层无效参数
} HardwareErrorCode_t;

// 通信错误码定义
typedef enum {
    /* 用户层错误码 */
    ERROR_CODE_COMM_CHM_TIMEOUT = 0x200,            // CHM超时
    ERROR_CODE_COMM_CRM_TIMEOUT,                    // CRM超时
    ERROR_CODE_COMM_CML_TIMEOUT,                    // CML超时
    ERROR_CODE_COMM_CRO_TIMEOUT,                    // CRO超时
    ERROR_CODE_COMM_CCS_TIMEOUT,                    // CCS超时
	ERROR_CODE_COMM_CST_TIMEOUT,                    // CST超时
	ERROR_CODE_COMM_CSD_TIMEOUT						// CSD超时
} CommunicationErrorCode_t;

// 错误记录结构体
typedef struct {
    ErrorType_t errorType;         // 错误类型
    uint16_t errorCode;            // 错误码
    uint32_t timestamp;            // 时间戳
} ErrorRecord_t;

// 错误处理接口
void ErrorHandler_HandleError(ErrorType_t errorType, uint16_t errorCode);

#endif /* __ERROR_HANDLING_H */
