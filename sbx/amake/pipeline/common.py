import os
from pathlib import Path

import AssetForge

class CommonAssetTool(AssetForge.AssetTool):
    def __init__(self):
        super().__init__()

    def build_cmd(self, input_file: Path) -> str:
        return f"@$(PYTHON) -m amake.pipeline.tool {self.tool_name()} {str(os.path.relpath(input_file, self.build_folder))} {str(os.path.relpath(self.input_folder, self.build_folder))} {str(self.output_folder.relative_to(self.build_folder))} >> log.txt 2>&1"
