import AssetForge

import json
from pathlib import Path
from typing import List

import trimesh
import numpy as np

from amake.proto import debug_primitives_pb2

from .common import CommonAssetTool

class PrimitivesTool(CommonAssetTool):
    def tool_name(self):
        return "PrimitivesTool"

    def check_match(self, file_path: Path) -> bool:
        return ("".join(file_path.suffixes) == ".primitives.json") and file_path.is_relative_to(self.input_folder)

    def define_dependencies(self, file_path: Path) -> List[Path]:
        try:
            with file_path.open("r", encoding="utf-8") as f:
                data = json.load(f)
            return [file_path.parent / Path(f) for _, f in data["models"].items()]
        except Exception as e:
            print(f"PrimitivesTool Error reading {file_path}: {e}")
        return []

    def define_outputs(self, file_path: Path) -> List[Path]:
        return [self.output_folder / file_path.relative_to(self.input_folder).with_suffix(".bin")]

    @staticmethod
    def build(file_path: Path, input_folder: Path, output_folder: Path) -> None:
        try:
            with file_path.open("r", encoding="utf-8") as f:
                prim_data = json.load(f)
        except Exception as e:
            print(f"Error reading JSON file {file_path}: {e}")
            return

        models = prim_data["models"]

        all_vert_data = []
        all_ebo_data = []
        model_offset = {}

        for name, obj_file in models.items():
            try:
                mesh = trimesh.load(file_path.parent / Path(obj_file), force='mesh')
                if mesh.faces.shape[1] != 3:
                    mesh = mesh.subdivide_to_triangles()
            except Exception as e:
                print(f"Error loading OBJ file {obj_file}: {e}")
                continue

            if not hasattr(mesh, 'vertex_normals') or len(mesh.vertex_normals) == 0:
                face_normals = mesh.face_normals
                new_verts = []
                new_ebo = []
                for i, face in enumerate(mesh.faces):
                    base_index = len(new_verts) // 6
                    face_normal = face_normals[i]
                    for vertex_index in face:
                        pos = mesh.vertices[vertex_index]
                        new_verts.extend([pos[0], pos[1], pos[2],
                                          face_normal[0], face_normal[1], face_normal[2]])
                    new_ebo.extend([base_index, base_index + 1, base_index + 2])
                verts = new_verts
                ebo = [x + (len(all_vert_data) // 6) for x in new_ebo]
            else:
                new_verts = []
                for pos, norm in zip(mesh.vertices, mesh.vertex_normals):
                    new_verts.extend([pos[0], pos[1], pos[2],
                                      norm[0], norm[1], norm[2]])
                verts = new_verts
                ebo = (mesh.faces.flatten() + (len(all_vert_data) // 6)).tolist()

            model_offset[name] = (len(all_vert_data) // 6, len(all_ebo_data), len(verts) // 6, len(ebo))
            all_vert_data += verts
            all_ebo_data += ebo

        prims = debug_primitives_pb2.DebugPrimitives()

        for name in model_offset:
            prims.model_names.append(name)

        for name in model_offset:
            vo, eo, vl, el = model_offset[name]
            m = prims.model_offsets.add()
            m.vert_offset = vo
            m.ebo_offset  = eo
            m.vert_length = vl
            m.ebo_length  = el

        for i in range(0, len(all_vert_data), 6):
            v = prims.vertices.add()
            v.position.extend(all_vert_data[i:i+3])
            v.normal.extend(all_vert_data[i+3:i+6])

        prims.indices.extend(all_ebo_data)

        output_file = output_folder / file_path.relative_to(input_folder).with_suffix(".bin")
        output_file.parent.mkdir(parents=True, exist_ok=True)
        with open(output_file, 'wb') as f:
            f.write(prims.SerializeToString())

        print(f"Successfully built primitives to {output_file}")

if __name__ == "__main__":
    tools = [PrimitivesTool()]
    AssetForge.common.main_func(tools)
