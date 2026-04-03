import AssetForge

from pathlib import Path
from typing import List

from amake.proto import shader_pb2

from .common import CommonAssetTool

stage_map = {
    "vertex":      shader_pb2.SHADER_STAGE_VERTEX,
    "fragment":    shader_pb2.SHADER_STAGE_FRAGMENT,
    "compute":     shader_pb2.SHADER_STAGE_COMPUTE,
    "geometry":    shader_pb2.SHADER_STAGE_GEOMETRY,
    "tesscontrol": shader_pb2.SHADER_STAGE_TESS_CONTROL,
    "tesseval":    shader_pb2.SHADER_STAGE_TESS_EVAL,
}

class ShaderTool(CommonAssetTool):
    def tool_name(self):
        return "ShaderTool"

    def check_match(self, file_path: Path) -> bool:
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

        shader_file = shader_pb2.ShaderFile()
        for stage_str, source in shader_map.items():
            entry = shader_file.shaders.add()
            entry.stage = stage_map[stage_str]
            entry.source = source

        out_path = output_folder / file_path.relative_to(input_folder).with_suffix(".shader.bin")
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(out_path, 'wb') as f:
            f.write(shader_file.SerializeToString())

        print("Successfully built shader to " + str(out_path))

if __name__ == "__main__":
    tools = [ShaderTool()]
    AssetForge.common.main_func(tools)
