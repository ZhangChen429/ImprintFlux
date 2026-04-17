#pragma once

#include "CoreMinimal.h"

// DLL 导出的函数指针类型定义
typedef void* (*FAlterMeshInit)(const wchar_t* Guid, const wchar_t* Guid2);
typedef void  (*FAlterMeshFree)(void* Handle);
typedef bool  (*FAlterMeshRead)(void* Handle, void** Address, size_t* OutLength);
typedef void  (*FAlterMeshWrite)(void* Handle, const char* Source, size_t Length);
typedef bool  (*FAlterMeshReadLock)(void* Handle);
typedef bool  (*FAlterMeshWriteLock)(void* Handle);
typedef void  (*FAlterMeshReadUnlock)(void* Handle);
typedef void  (*FAlterMeshWriteUnlock)(void* Handle);

/**
 * AlterMesh.dll 的封装管理类
 * 负责：加载DLL、创建共享内存通道、提供读写接口
 */
class CURVESCRIBE_API FAlterMeshBridge
{
public:
    FAlterMeshBridge();
    ~FAlterMeshBridge();

    // 加载 DLL 并初始化共享内存通道（传入两个唯一的 GUID）
    bool Initialize(const FString& Guid1, const FString& Guid2);

    // 释放所有资源
    void Shutdown();

    bool IsInitialized() const { return Handle != nullptr; }

    // --- 写入接口（UE → 外部程序）---
    // 锁定写入，返回 false 表示失败
    bool WriteLock();
    // 写入一段二进制数据
    void Write(const void* Data, int64 Length);
    // 写入字符串（UTF-8）
    void WriteString(const FString& Str);
    // 解锁，通知外部程序可以读取了
    void WriteUnlock();

    // --- 读取接口（外部程序 → UE）---
    // 等待外部程序写入完成，返回 false 表示失败
    bool ReadLock();
    // 读取下一块数据，Address 指向共享内存，无需拷贝
    bool Read(void** OutAddress, size_t* OutLength);
    // 读取为字节数组
    bool ReadBytes(TArray<uint8>& OutBytes);
    // 解锁，通知外部程序可以继续写入
    void ReadUnlock();

private:
    // DLL 句柄
    void* DllHandle = nullptr;
    // 共享内存句柄（由 Init 返回）
    void* Handle = nullptr;

    // 函数指针
    FAlterMeshInit      Func_Init        = nullptr;
    FAlterMeshFree      Func_Free        = nullptr;
    FAlterMeshRead      Func_Read        = nullptr;
    FAlterMeshWrite     Func_Write       = nullptr;
    FAlterMeshReadLock  Func_ReadLock    = nullptr;
    FAlterMeshWriteLock Func_WriteLock   = nullptr;
    FAlterMeshReadUnlock  Func_ReadUnlock  = nullptr;
    FAlterMeshWriteUnlock Func_WriteUnlock = nullptr;
};
