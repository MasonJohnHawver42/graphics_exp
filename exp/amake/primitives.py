import AssetForge

import json
import struct
from pathlib import Path
from typing import List, Dict, Tuple

import flatbuffers
from amake.data.gl.debug import DebugPrimitives, DebugPrimitiveModel, DebugPrimitiveVertex

import trimesh
import numpy as np

class PrimitivesTool(AssetForge.AssetTool):
    def tool_name(self):
        return "PrimitivesTool"
    
    def check_match(self, file_path: Path) -> bool:
        # Accept files with the .atals extension.
        return ("".join(file_path.suffixes) == ".primitives.json") and file_path.is_relative_to(self.input_folder)

    def define_dependencies(self, file_path: Path) -> List[Path]:
        # The JSON atlas file references an image.
        try:
            with file_path.open("r", encoding="utf-8") as f:
                data = json.load(f)

            return [file_path.parent / Path(f) for _, f in data["models"].items()] 
        except Exception as e:
            print(f"PrimitivesTool Error reading {file_path}: {e}")
        
        return []

    def define_outputs(self, file_path: Path) -> List[Path]:
        return [self.output_folder / self.relative_path(file_path.with_suffix(".bin"))]
    
    def build(self, file_path: Path) -> None:
        # Read the atlas JSON data.
        json_path = file_path

        try:
            with json_path.open("r", encoding="utf-8") as f:
                prim_data = json.load(f)
        except Exception as e:
            print(f"Error reading atlas JSON file {file_path}: {e}")
            return

        models = prim_data["models"]

        all_vert_data = []
        all_ebo_data = []
        model_offset = {}

        for name, obj_file in models.items():
            verts = [] # 3 for xyz 3 for normal = 6 total each
            ebo = []
            # load file .obj and extract the xyz and normals
            # if the normals don't exists generate them store them into the verts and ebo lists above flatley

            try:
                # Load the mesh from the OBJ file.
                mesh = trimesh.load(file_path.parent / Path(obj_file), force='mesh')
                
                # Ensure all faces are triangles.
                if mesh.faces.shape[1] != 3:
                    mesh = mesh.subdivide_to_triangles()
            except Exception as e:
                print(f"Error loading OBJ file {obj_file}: {e}")
                continue
            
            if not hasattr(mesh, 'vertex_normals') or len(mesh.vertex_normals) == 0:
                print("here noooo")
                face_normals = mesh.face_normals
                new_verts = []
                new_ebo = []
                for i, face in enumerate(mesh.faces):
                    # Record the starting index for the vertices of this face.
                    base_index = len(new_verts) // 6  # Each vertex has 6 float values.
                    face_normal = face_normals[i]
                    for vertex_index in face:
                        pos = mesh.vertices[vertex_index]
                        # Append position and the computed face normal.
                        new_verts.extend([pos[0], pos[1], pos[2],
                                        face_normal[0], face_normal[1], face_normal[2]])
                    # Each face is a triangle, so add three indices.
                    new_ebo.extend([base_index, base_index + 1, base_index + 2])
                verts = new_verts
                ebo = [x + (len(all_vert_data) // 6) for x in new_ebo]
            else:
                # Use the normals provided in the OBJ file.
                new_verts = []
                # Assuming mesh.vertices and mesh.vertex_normals align.
                for pos, norm in zip(mesh.vertices, mesh.vertex_normals):
                    new_verts.extend([pos[0], pos[1], pos[2],
                                    norm[0], norm[1], norm[2]])
                # print(len(new_verts))
                # print(len(mesh.vertices))
                # print(len(mesh.vertex_normals))
                verts = new_verts
                # Flatten the faces array to create the element buffer object.
                ebo = (mesh.faces.flatten() + (len(all_vert_data) // 6)).tolist()
                # print(ebo)
            
            model_offset[name] = (len(all_vert_data) // 6, len(all_ebo_data), len(verts) // 6, len(ebo))
            all_vert_data += verts
            all_ebo_data += ebo
        
        output_file = self.output_folder / self.relative_path(file_path.with_suffix(".bin"))

        builder = flatbuffers.Builder(0)

        # Build vertices
        vertex_offsets = []
        for i in range(0, len(all_vert_data), 6):
            pos = all_vert_data[i:i+3]
            norm = all_vert_data[i+3:i+6]

            pos_vec = builder.CreateNumpyVector(np.array(pos, dtype=np.float32))
            norm_vec = builder.CreateNumpyVector(np.array(norm, dtype=np.float32))

            DebugPrimitiveVertex.Start(builder)
            DebugPrimitiveVertex.AddPosition(builder, pos_vec)
            DebugPrimitiveVertex.AddNormal(builder, norm_vec)
            vertex_offsets.append(DebugPrimitiveVertex.End(builder))

        DebugPrimitives.StartVerticesVector(builder, len(vertex_offsets))
        for v in reversed(vertex_offsets): builder.PrependUOffsetTRelative(v)
        vertices = builder.EndVector()

        # Indices
        DebugPrimitives.StartIndicesVector(builder, len(all_ebo_data))
        for i in reversed(all_ebo_data): builder.PrependUint32(i)
        indices = builder.EndVector()

        # Model names
        name_offsets = [builder.CreateString(name) for name in model_offset]
        DebugPrimitives.StartModelNamesVector(builder, len(name_offsets))
        for name in reversed(name_offsets): builder.PrependUOffsetTRelative(name)
        model_names = builder.EndVector()

        # Model offsets
        model_offsets = []
        for name in model_offset:
            vo, eo, vl, el = model_offset[name]
            DebugPrimitiveModel.Start(builder)
            DebugPrimitiveModel.AddVertOffset(builder, vo)
            DebugPrimitiveModel.AddEboOffset(builder, eo)
            DebugPrimitiveModel.AddVertLength(builder, vl)
            DebugPrimitiveModel.AddEboLength(builder, el)
            model_offsets.append(DebugPrimitiveModel.End(builder))

        DebugPrimitives.StartModelOffsetsVector(builder, len(model_offsets))
        for o in reversed(model_offsets): builder.PrependUOffsetTRelative(o)
        offsets = builder.EndVector()

        # Root table
        DebugPrimitives.Start(builder)
        DebugPrimitives.AddModelNames(builder, model_names)
        DebugPrimitives.AddModelOffsets(builder, offsets)
        DebugPrimitives.AddVertices(builder, vertices)
        DebugPrimitives.AddIndices(builder, indices)
        root = DebugPrimitives.End(builder)

        builder.Finish(root)

        # Write binary
        output_file = self.output_folder / self.relative_path(file_path.with_suffix(".bin"))
        with open(output_file, "wb") as f:
            f.write(builder.Output())
        
        return

        # now in the output file write the model_offset data  all_vert_data all_ebo_data
        # let it start with the number of models in it as binary unisgned integer
        # then let it have a blob of text for each name of the model that will have the pattern 
        # model name\0model name 2\0 .... \0 each name is delimed by \0 and the order will be the same as the offset information
        # save the offset data all 4 as integers vert offset first then ebo offset then vert length then ebo length both as binary unisgned integer
        # then save the number of floats in the vert data as binary unsigned integer
        # save the vert data as a list of floats in binary to the file
        # then save the number of indexes in the ebo data as a binary unsigned int
        # then save all the ebo data as a list of unsigned integers 

        print(all_vert_data)

        try:
            with output_file.open("wb") as f:
                # Write the number of models as an unsigned integer.
                num_models = len(model_offset)
                f.write(struct.pack("I", num_models))

                # Write a blob of model names, each name delimited by a null byte (\0).
                # The order here is the same as the order in model_offset.
                model_names_blob = b"".join(name.encode("utf-8") + b"\0" for name in model_offset.keys())
                f.write(model_names_blob)

                # Write the offset data for each model in the same order.
                # Each model's offset data: 4 unsigned integers: vertex offset, EBO offset, vertex data length, EBO length.
                for name in model_offset.keys():
                    vert_offset, ebo_offset, vert_length, ebo_length = model_offset[name]
                    f.write(struct.pack("IIII", vert_offset, ebo_offset, vert_length, ebo_length))

                # Write the total number of floats in the vertex data as an unsigned integer.
                num_floats = len(all_vert_data)
                f.write(struct.pack("I", num_floats))

                # Write the vertex data as a list of floats.
                f.write(struct.pack(f"{num_floats}f", *all_vert_data))

                # Write the total number of indices in the EBO data as an unsigned integer.
                num_indices = len(all_ebo_data)
                f.write(struct.pack("I", num_indices))

                # Write the EBO data as a list of unsigned integers.
                f.write(struct.pack(f"{num_indices}I", *all_ebo_data))
        except Exception as e:
            print(f"Error writing to output file {output_file}: {e}")
            return

        print(f"Successfully wrote {len(model_offset)} models to {output_file}")