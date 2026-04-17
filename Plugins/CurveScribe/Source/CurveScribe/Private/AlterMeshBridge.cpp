#include "AlterMeshBridge.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

FAlterMeshBridge::FAlterMeshBridge()
{
}

FAlterMeshBridge::~FAlterMeshBridge()
{
    Shutdown();
}

bool FAlterMeshBridge::Initialize(const FString& Guid1, const FString& Guid2)
{
    // 1. 找到 DLL 路径
    FString DllPath = FPaths::Combine(
        IPluginManager::Get().FindPlugin(TEXT("CurveScribe"))->GetBaseDir(),
        TEXT("Binaries/Win64/AlterMesh.dll")
    );

    // 2. 加载 DLL
    DllHandle = FPlatformProcess::GetDllHandle(*DllPath);
    if (!DllHandle)
    {
        UE_LOG(LogTemp, Error, TEXT("AlterMeshBridge: 无法加载 DLL: %s"), *DllPath);
        return false;
    }

    // 3. 获取所有函数指针
    Func_Init         = (FAlterMeshInit)        FPlatformProcess::GetDllExport(DllHandle, TEXT("Init"));
    Func_Free         = (FAlterMeshFree)        FPlatformProcess::GetDllExport(DllHandle, TEXT("Free"));
    Func_Read         = (FAlterMeshRead)        FPlatformProcess::GetDllExport(DllHandle, TEXT("Read"));
    Func_Write        = (FAlterMeshWrite)       FPlatformProcess::GetDllExport(DllHandle, TEXT("Write"));
    Func_ReadLock     = (FAlterMeshReadLock)    FPlatformProcess::GetDllExport(DllHandle, TEXT("ReadLock"));
    Func_WriteLock    = (FAlterMeshWriteLock)   FPlatformProcess::GetDllExport(DllHandle, TEXT("WriteLock"));
    Func_ReadUnlock   = (FAlterMeshReadUnlock)  FPlatformProcess::GetDllExport(DllHandle, TEXT("ReadUnlock"));
    Func_WriteUnlock  = (FAlterMeshWriteUnlock) FPlatformProcess::GetDllExport(DllHandle, TEXT("WriteUnlock"));

    // 4. 检查是否全部找到
    if (!Func_Init || !Func_Free || !Func_Read || !Func_Write ||
        !Func_ReadLock || !Func_WriteLock || !Func_ReadUnlock || !Func_WriteUnlock)
    {
        UE_LOG(LogTemp, Error, TEXT("AlterMeshBridge: DLL 函数指针获取失败"));
        Shutdown();
        return false;
    }

    // 5. 初始化共享内存通道
    // Guid1 是 UE→外部 方向，Guid2 是 外部→UE 方向
    Handle = Func_Init(*Guid1, *Guid2);
    if (!Handle)
    {
        UE_LOG(LogTemp, Error, TEXT("AlterMeshBridge: Init 失败，无法创建共享内存"));
        Shutdown();
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("AlterMeshBridge: 初始化成功"));
    return true;
}

void FAlterMeshBridge::Shutdown()
{
    if (Handle && Func_Free)
    {
        Func_Free(Handle);
        Handle = nullptr;
    }

    if (DllHandle)
    {
        FPlatformProcess::FreeDllHandle(DllHandle);
        DllHandle = nullptr;
    }
}

// ---- 写入接口 ----

bool FAlterMeshBridge::WriteLock()
{
    if (!Handle || !Func_WriteLock) return false;
    return Func_WriteLock(Handle);
}

void FAlterMeshBridge::Write(const void* Data, int64 Length)
{
    if (!Handle || !Func_Write) return;
    Func_Write(Handle, (const char*)Data, (size_t)Length);
}

void FAlterMeshBridge::WriteString(const FString& Str)
{
    FTCHARToUTF8 Converter(*Str);
    Write(Converter.Get(), Converter.Length());
}

void FAlterMeshBridge::WriteUnlock()
{
    if (!Handle || !Func_WriteUnlock) return;
    Func_WriteUnlock(Handle);
}

// ---- 读取接口 ----

bool FAlterMeshBridge::ReadLock()
{
    if (!Handle || !Func_ReadLock) return false;
    return Func_ReadLock(Handle);
}

bool FAlterMeshBridge::Read(void** OutAddress, size_t* OutLength)
{
    if (!Handle || !Func_Read) return false;
    return Func_Read(Handle, OutAddress, OutLength);
}

bool FAlterMeshBridge::ReadBytes(TArray<uint8>& OutBytes)
{
    void* Address = nullptr;
    size_t Length = 0;

    if (!Read(&Address, &Length) || Length == 0)
        return false;

    OutBytes.SetNumUninitialized(Length);
    FMemory::Memcpy(OutBytes.GetData(), Address, Length);
    return true;
}

void FAlterMeshBridge::ReadUnlock()
{
    if (!Handle || !Func_ReadUnlock) return;
    Func_ReadUnlock(Handle);
}
