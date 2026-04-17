import bpy
import struct
import ctypes
import time
import sys
import os

# ====================== 日志 ======================
log_path = os.path.join(os.path.dirname(__file__), "blender_debug.log")
log_file = open(log_path, "w", encoding='utf-8')
sys.stdout = log_file
sys.stderr = log_file


def log(msg):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}", flush=True)
    log_file.flush()


log("=" * 50)
log("Blender 桥接脚本启动")

# ====================== DLL 加载 ======================
dll_path = "F:/Data/UE_Project/FlowSolo/FlowAndEditorTool/FlowSolo/Plugins/UEBlender/Binaries/Win64/AlterMesh.dll"
try:
    alter_mesh = ctypes.cdll.LoadLibrary(dll_path)
    log(f"DLL 加载成功")
except Exception as e:
    log(f"DLL 加载失败: {e}")
    sys.exit(1)

alter_mesh.Init.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p)
alter_mesh.Init.restype = ctypes.c_void_p
alter_mesh.WriteLock.argtypes = (ctypes.c_void_p,)
alter_mesh.WriteLock.restype = ctypes.c_bool
alter_mesh.Write.argtypes = (ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t)
alter_mesh.WriteUnlock.argtypes = (ctypes.c_void_p,)

# ====================== 解析参数 ======================
try:
    idx = sys.argv.index("--")
    args = sys.argv[idx + 1:]
    guid_blender_to_ue = args[0]  # Guid2 - Blender→UE 通道
    guid_ue_to_blender = args[1]  # Guid1 - UE→Blender 通道
except:
    log("错误：参数解析失败")
    sys.exit(1)

# ====================== 关键：获取 .blend 文件路径 ======================
# Blender 自动打开文件后，bpy.data.filepath 就是文件路径
blend_file_path = bpy.data.filepath
log(f"Blend 文件路径: {blend_file_path}")

if not blend_file_path:
    log("错误：没有打开 .blend 文件！")
    sys.exit(1)

# 初始化共享内存（注意 GUID 顺序：Init(UE→Blender, Blender→UE)）
handle = alter_mesh.Init(guid_ue_to_blender, guid_blender_to_ue)
if not handle:
    log("共享内存初始化失败")
    sys.exit(1)

log(f"共享内存初始化成功")

# ====================== 获取要同步的对象 ======================
# 方式1：使用文件中指定的对象名称（通过文件名或其他约定）
# 方式2：使用当前选中的对象
# 方式3：使用文件中的第一个 Mesh 对象

obj = bpy.context.active_object
if not obj or obj.type != 'MESH':
    # 找到第一个 Mesh
    for o in bpy.data.objects:
        if o.type == 'MESH':
            obj = o
            break

if not obj:
    log("错误：场景中没有 Mesh 对象")
    sys.exit(1)

log(f"同步对象: {obj.name}")

# ====================== 发送逻辑 ======================
last_hash = 0


def get_mesh_hash(mesh):
    hash_val = 0
    for v in mesh.vertices:
        hash_val ^= hash((round(v.co.x, 4), round(v.co.y, 4), round(v.co.z, 4)))
    return hash_val


def send_mesh():
    global last_hash

    if not obj or obj.type != 'MESH':
        return 0.1

    try:
        depsgraph = bpy.context.evaluated_depsgraph_get()
        eval_obj = obj.evaluated_get(depsgraph)
        mesh = eval_obj.to_mesh()

        if not mesh or len(mesh.vertices) == 0:
            eval_obj.to_mesh_clear()
            return 0.1

        current_hash = get_mesh_hash(mesh)
        if current_hash == last_hash:
            eval_obj.to_mesh_clear()
            return 0.05

        last_hash = current_hash
        mesh.calc_loop_triangles()

        # 构建二进制数据
        data = bytearray()
        data.extend(struct.pack('ii', len(mesh.vertices), len(mesh.loop_triangles) * 3))

        uv_layer = mesh.uv_layers.active if mesh.uv_layers else None

        for i, vert in enumerate(mesh.vertices):
            data.extend(struct.pack('fff', vert.co.x, vert.co.y, vert.co.z))
            data.extend(struct.pack('fff', vert.normal.x, vert.normal.y, vert.normal.z))

            if uv_layer and i < len(uv_layer.data):
                uv = uv_layer.data[i].uv
                data.extend(struct.pack('ff', uv.x, uv.y))
            else:
                data.extend(struct.pack('ff', 0.0, 0.0))

        for tri in mesh.loop_triangles:
            data.extend(struct.pack('iii', tri.vertices[0], tri.vertices[1], tri.vertices[2]))

        eval_obj.to_mesh_clear()

        # 发送
        if alter_mesh.WriteLock(handle):
            alter_mesh.Write(handle, bytes(data), len(data))
            alter_mesh.WriteUnlock(handle)
            log(f"发送: {len(mesh.vertices)} 顶点, {len(mesh.loop_triangles)} 面")

    except Exception as e:
        log(f"异常: {e}")

    return 0.05  # 50ms 后再次检查


# ====================== 启动 ======================
def main_loop():
    log("开始实时同步...")
    log("提示：在 Blender 中编辑模型，UE 将实时看到变化")

    # 后台模式用 while，GUI 模式用 timer
    if bpy.app.background:
        log("后台模式运行")
        while True:
            try:
                if obj and obj.is_updated_data:
                    send_mesh()
                time.sleep(0.05)
                bpy.context.view_layer.update()
            except Exception as e:
                log(f"循环异常: {e}")
                break
        AlterMesh.Free(handle)
    else:
        log("GUI 模式运行")
        bpy.app.timers.register(send_mesh, first_interval=0.1, persistent=True)


if __name__ == "__main__":
    main_loop()
