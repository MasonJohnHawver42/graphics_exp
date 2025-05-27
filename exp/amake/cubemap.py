import AssetForge

import json
import struct
from pathlib import Path
from typing import List, Dict, Tuple
import numpy as np
import cv2
import os

dirs = ["right", "left", "top", "bottom", "front", "back"]

class CubemapTool(AssetForge.AssetTool):
    """
    example json meta data:

    {
        "type": "equirectangular_cubemap",
        "input": "rogland_clear_night_4k.hdr",
        "face_size": "lambda input_width : int(input_width / 4)",
        "output": {
            "right": "rogland_clear_night_4k/right.hdr",
            "left": "rogland_clear_night_4k/left.hdr",
            "top": "rogland_clear_night_4k/top.hdr",
            "bottom": "rogland_clear_night_4k/bottom.hdr",
            "front": "rogland_clear_night_4k/front.hdr",
            "back": "rogland_clear_night_4k/back.hdr"
        }
    }
    """
    def tool_name(self):
        return "CubemapTool"
    
    def check_match(self, file_path: Path) -> bool:
        # Accept files with the .atals extension.
        return ("".join(file_path.suffixes).count(".cubemap.json") == 1) and file_path.is_relative_to(self.input_folder)

    def define_dependencies(self, file_path: Path) -> List[Path]:
        # The JSON cubemap file references an image.
        try:
            with file_path.open("r", encoding="utf-8") as f:
                data = json.load(f)
            if data["type"] == "equirectangular_cubemap":
                return [file_path.parent / Path(data["input"])]
            else:
                raise NotImplementedError("Haven't implemented that type of cubemap yet")
        except Exception as e:
            print(f"Cubemap Error reading {file_path}: {e}")
        
        return []

    def define_outputs(self, file_path: Path) -> List[Path]:
        outputs = [self.output_folder / self.relative_path(file_path.with_suffix(".bin"))]

        try:
            with file_path.open("r", encoding="utf-8") as f:
                data = json.load(f)
            if data["type"] == "equirectangular_cubemap":
                return outputs + [self.output_folder / self.relative_path(file_path.parent / Path(data["output"][d])) for d in dirs]
            else:
                raise NotImplementedError("Haven't implemented that type of cubemap yet")
        except Exception as e:
            print(f"Cubemap Error reading {file_path}: {e}")
        
        return outputs
    
    def build(self, file_path: Path) -> None:
        # Read the cubemap JSON data.
        json_path = file_path

        try:
            with json_path.open("r", encoding="utf-8") as f:
                cubemap_data = json.load(f)
        except Exception as e:
            print(f"Error reading cubemap JSON file {file_path}: {e}")
            return
        
        bin_path = self.output_folder / self.relative_path(file_path.with_suffix(".bin"))
        input_path = file_path.parent / Path(cubemap_data["input"])

        eq_img = cv2.imread(str(input_path), cv2.IMREAD_UNCHANGED)
        if eq_img is None:
            print(f"Failed to load equirectangular image: {input_path}")
            return
        
        print(f"Loaded equirectangular image: {input_path} (shape: {eq_img.shape})")

        input_width = eq_img.shape[1]
        face_size = eval(cubemap_data["face_size"])(input_width)

        # --- Helper functions for cubemap generation ---
        def get_direction(face: str, u: np.ndarray, v: np.ndarray) -> (np.ndarray, np.ndarray, np.ndarray):
            """
            Compute direction vectors for a given face based on normalized coordinates u and v.
            """
            if face == 'right':
                x = np.ones_like(u)
                y = -v
                z = -u
            elif face == 'left':
                x = -np.ones_like(u)
                y = -v
                z = u
            elif face == 'top':
                x = u
                y = np.ones_like(u)
                z = v
            elif face == 'bottom':
                x = u
                y = -np.ones_like(u)
                z = -v
            elif face == 'front':
                x = u
                y = -v
                z = np.ones_like(u)
            elif face == 'back':
                x = -u
                y = -v
                z = -np.ones_like(u)
            else:
                raise ValueError(f"Invalid face name: {face}")
            return x, y, z
        
        def generate_face_map(face: str, face_size: int, eq_width: int, eq_height: int) -> (np.ndarray, np.ndarray):
            """
            Generates mapping arrays for cv2.remap for a given cubemap face.
            """
            # Create a grid of pixel centers [0, face_size)
            indices = np.linspace(0, face_size - 1, face_size) + 0.5
            a, b = np.meshgrid(indices, indices)
            # Map to normalized coordinates in [-1, 1]
            u = 2.0 * (a / face_size) - 1.0
            v = 2.0 * (b / face_size) - 1.0

            # Get direction vectors for the face.
            x, y, z = get_direction(face, u, v)
            norm = np.sqrt(x**2 + y**2 + z**2)
            x /= norm
            y /= norm
            z /= norm

            # Convert the direction to spherical coordinates.
            theta = np.arctan2(z, x)  # angle in [-pi, pi]
            phi = np.arccos(y)        # angle in [0, pi]

            # Map spherical coordinates to equirectangular texture coordinates.
            map_x = ((theta + np.pi) / (2 * np.pi)) * (eq_width - 1)
            map_y = (phi / np.pi) * (eq_height - 1)
            return map_x.astype(np.float32), map_y.astype(np.float32)

        def generate_cubemap_face(eq_img: np.ndarray, face: str, face_size: int) -> np.ndarray:
            """
            Generates a single cubemap face image from the equirectangular image.
            """
            eq_height, eq_width = eq_img.shape[:2]
            map_x, map_y = generate_face_map(face, face_size, eq_width, eq_height)
            # Remap using linear interpolation; use BORDER_WRAP for horizontal seam handling.
            face_img = cv2.remap(eq_img, map_x, map_y, interpolation=cv2.INTER_LINEAR, borderMode=cv2.BORDER_WRAP)
            return face_img
        
        # --- Generate and save each cubemap face ---
        # This dict will map each face to its output path (as a string relative to some base).
        output_paths = {}
        for face in dirs:
            print(f"Processing face: {face}")
            face_img = generate_cubemap_face(eq_img, face, face_size)
            # Compute the absolute output path from the JSON metadata.
            rel_out = cubemap_data["output"][face]
            out_path = self.output_folder / self.relative_path(file_path.parent / Path(rel_out))
            # Ensure the output directory exists.
            out_path.parent.mkdir(parents=True, exist_ok=True)
            # Save the face image. The file extension (e.g., .hdr or .exr) controls the format.
            if not cv2.imwrite(str(out_path), face_img):
                print(f"Failed to save {face} face to: {out_path}")
            else:
                print(f"Saved {face} face to: {out_path}")
                # Store the relative path (as a string) for the binary file.
                print(str(os.path.relpath(out_path, start=bin_path)))
                output_paths[face] = str(os.path.relpath(out_path, start=bin_path.parent))
        
        try:
            bin_path.parent.mkdir(parents=True, exist_ok=True)
            with bin_path.open("wb") as bin_file:

                text_blob = bytearray()

                for face in dirs:
                    path_str = output_paths.get(face, "")
                    text_blob += path_str.encode("utf-8") + b'\0'
                
                text_blob_size = len(text_blob)
                header = struct.pack("I", text_blob_size)
                output_bytes = header + text_blob
                
                bin_file.write(output_bytes)

            print(f"Successfully wrote binary cubemap meta to: {bin_path}")
        except Exception as e:
            print(f"Error writing binary file {bin_path}: {e}")
       