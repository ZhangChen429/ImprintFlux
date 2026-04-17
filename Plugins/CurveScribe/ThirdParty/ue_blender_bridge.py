import bpy
import sys
import json
import ctypes
import time

# 从命令行获取 GUID
args = sys.argv[sys.argv.index("--") + 1:]
guid1 = args[0]  # UE → Blender
guid2 = args[1]  # Blender → UE

# 加载 AlterMesh DLL
dll_path = "F:/Data/UE_Project/FlowSolo/FlowAndEditorTool/FlowSolo/Plugins/UEBlender/Binaries/Win64/AlterMesh.dll"
alter_mesh = ctypes.cdll.LoadLibrary(dll_path)

# 定义函数签名
alter_mesh.Init.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p)
alter_mesh.Init.restype = ctypes.c_void_p
alter_mesh.ReadLock.argtypes = (ctypes.c_void_p,)
alter_mesh.ReadLock.restype = ctypes.c_bool
alter_mesh.Read.argtypes = (ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_size_t))
alter_mesh.Read.restype = ctypes.c_bool
alter_mesh.ReadUnlock.argtypes = (ctypes.c_void_p,)
alter_mesh.WriteLock.argtypes = (ctypes.c_void_p,)
alter_mesh.WriteLock.restype = ctypes.c_bool
alter_mesh.Write.argtypes = (ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t)
alter_mesh.WriteUnlock.argtypes = (ctypes.c_void_p,)

# 初始化（交换 GUID，因为方向相反）
handle = alter_mesh.Init(guid2, guid1)


def read_params():
    """从 UE 读取参数"""
    if not alter_mesh.ReadLock(handle):
        return None

    addr = ctypes.c_void_p()
    length = ctypes.c_size_t()

    if alter_mesh.Read(handle, ctypes.byref(addr), ctypes.byref(length)) and length.value > 0:
        data = ctypes.string_at(addr, length.value)
        alter_mesh.ReadUnlock(handle)
        return json.loads(data.decode('utf-8'))

    alter_mesh.ReadUnlock(handle)
    return None


def write_mesh_data(obj):
    """将网格数据写回 UE"""
    # 确保对象已更新
    depsgraph = bpy.context.evaluated_depsgraph_get()
    eval_obj = obj.evaluated_get(depsgraph)
    mesh = eval_obj.to_mesh()

    # 计算三角形
    mesh.calc_loop_triangles()

    # 收集顶点
    vertices = []
    for v in mesh.vertices:
        vertices.extend([v.co.x, v.co.y, v.co.z])

    # 收集索引
    indices = []
    for tri in mesh.loop_triangles:
        indices.extend([tri.vertices[0], tri.vertices[1], tri.vertices[2]])

    # 构建 JSON
    data = {
        "vertices": vertices,
        "indices": indices,
        "vertex_count": len(mesh.vertices),
        "triangle_count": len(mesh.loop_triangles)
    }

    json_bytes = json.dumps(data).encode('utf-8')

    # 写入共享内存
    if alter_mesh.WriteLock(handle):
        alter_mesh.Write(handle, json_bytes, len(json_bytes))
        alter_mesh.WriteUnlock(handle)
        print(f"发送网格: {len(vertices) // 3} 顶点, {len(indices) // 3} 三角形")


def main_loop():
    """主循环 - 持续监听 UE 的请求"""
    print("Blender 桥接已启动，等待 UE 指令...")

    while True:
        try:
            # 读取 UE 发送的参数
            params = read_params()

            if params:
                print(f"收到参数: {params}")

                # 获取目标对象
                obj_name = params.get("ObjectName", "Cube")
                obj = bpy.data.objects.get(obj_name)

                if obj and obj.modifiers:
                    # 更新几何节点参数
                    for modifier in obj.modifiers:
                        if modifier.type == 'NODES':
                            inputs = params.get("Inputs", {})
                            for name, value in inputs.items():
                                if name in modifier:
                                    modifier[name] = value

                    # 刷新场景
                    bpy.context.view_layer.update()

                    # 将结果发回 UE
                    write_mesh_data(obj)

            time.sleep(0.016)  # 60 FPS

        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"错误: {e}")
            time.sleep(0.1)


if __name__ == "__main__":
    main_loop()