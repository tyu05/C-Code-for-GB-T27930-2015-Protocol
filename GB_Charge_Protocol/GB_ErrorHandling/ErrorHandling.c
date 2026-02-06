/*
 * ErrorHandling.c
 *
 *  Created on: 2025年7月24日
 *  Author: 83923
 *
 */


#include "ErrorHandling.h"

ErrorRecord_t errorRecord;


// 硬件错误信息字符串数组
static const char* hardwareErrorStrings[] = {
    "传输层错误",
    "传输层忙",
    "传输层超时",
    "传输层无效参数",
};

// 通信错误信息字符串数组
static const char* communicationErrorStrings[] = {
    /* 用户层错误信息 */
    "CHM超时",
    "CRM超时",
    "CML超时",
    "CRO超时",
    "CCS超时",
    "CST超时",
    "CSD超时"
};

// 获取错误信息字符串
static const char* GetErrorString(ErrorType_t errorType, uint16_t errorCode) {
    switch (errorType) {
        case ERROR_TYPE_HARDWARE:
            if (errorCode >= ERROR_CODE_HW_TRANSPORT_ERROR && errorCode <= ERROR_CODE_HW_TRANSPORT_INVALID_PARAM) {
                return hardwareErrorStrings[errorCode - ERROR_CODE_HW_TRANSPORT_ERROR];
            }
            break;
            
        case ERROR_TYPE_COMMUNICATION:
             if (errorCode >= ERROR_CODE_COMM_CHM_TIMEOUT && errorCode <= ERROR_CODE_COMM_CSD_TIMEOUT) {
                 return communicationErrorStrings[errorCode - ERROR_CODE_COMM_CHM_TIMEOUT];
             }
             break;
            
        default:
            break;
    }
    
    return "未知错误";
}

/**
 * @brief  错误处理接口函数
 * @param  errorType: 错误类型
 * @param  errorCode: 错误码
 * @retval None
 */
void ErrorHandler_HandleError(ErrorType_t errorType, uint16_t errorCode)
{
    const char* errorStr = GetErrorString(errorType, errorCode);
	
    // 调用硬件层函数获取时间戳
    uint32_t timestamp = Hardware_Get_Timestamp();
    
    // 填充错误记录
    errorRecord.errorType = errorType;
    errorRecord.errorCode = errorCode;
    errorRecord.timestamp = timestamp;
    
    // 输出错误信息
    printf("[ERROR] Time: %d, Description: %s\n", timestamp, errorStr);
}
