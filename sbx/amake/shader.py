import AssetForge

import sys
from pathlib import Path
from typing import List, Dict, Tuple

import flatbuffers

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amake.data.gl import ShaderFile, ShaderEntry, ShaderStage

stage_map = {
    "vertex": ShaderStage.ShaderStage.Vertex,
    "fragment": ShaderStage.ShaderStage.Fragment,
    "compute": ShaderStage.ShaderStage.Compute,
    "geometry": ShaderStage.ShaderStage.Geometry,
    "tesscontrol": ShaderStage.ShaderStage.TessControl,
    "tesseval": ShaderStage.ShaderStage.TessEval,
}

class ShaderTool(AssetForge.AssetTool):
    def tool_name(self):
        return "ShaderTool"
    
    def check_match(self, file_path: Path) -> bool:
        # Accept files with the .atals extension.
        return ("".join(file_path.suffixes) == ".shader") and file_path.is_relative_to(self.input_folder)

    def define_dependencies(self, file_path: Path) -> List[Path]:
        return []

    def define_outputs(self, file_path: Path) -> List[Path]:
        return [self.output_folder / file_path.relative_to(self.input_folder).with_suffix(".shader.bin")]
    
    @staticmethod
    def build(file_path: Path, input_folder: Path, output_folder: Path) -> None:
        with open(file_path, 'r') as f:
            lines = f.readlines()

            current_stage = None
            shader_map = {}
            buffer = []

            for line in lines:
                if line.startswith("#type"):
                    if current_stage and buffer:
                        shader_map[current_stage] = ''.join(buffer)
                    current_stage = line.strip().split()[1].lower()
                    buffer = []
                else:
                    buffer.append(line)

            if current_stage and buffer:
                shader_map[current_stage] = ''.join(buffer)
        
        builder = flatbuffers.Builder(1024)
        shader_entries = []

        for stage_str, source in shader_map.items():
            stage = stage_map.get(stage_str)
            src_offset = builder.CreateString(source)

            ShaderEntry.Start(builder)
            ShaderEntry.AddStage(builder, stage)
            ShaderEntry.AddSource(builder, src_offset)
            entry = ShaderEntry.End(builder)
            shader_entries.append(entry)

        ShaderFile.StartShadersVector(builder, len(shader_entries))
        for entry in reversed(shader_entries):
            builder.PrependUOffsetTRelative(entry)
        shaders = builder.EndVector()

        ShaderFile.Start(builder)
        ShaderFile.AddShaders(builder, shaders)
        root = ShaderFile.End(builder)

        builder.Finish(root)

        out_path = output_folder / file_path.relative_to(input_folder).with_suffix(".shader.bin")
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(out_path, 'wb') as f:
            f.write(builder.Output())
        
        print("Successfully Built Shader to " + str(out_path))

if __name__ == "__main__":
    tools = [ShaderTool()]
    AssetForge.common.main_func(tools)