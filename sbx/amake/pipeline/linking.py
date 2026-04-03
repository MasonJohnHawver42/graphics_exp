import os
import re
from pathlib import Path
from typing import List

import AssetForge

from .common import CommonAssetTool


def in_folder(file_path: Path, folder: Path) -> bool:
    try:
        file_path.resolve().relative_to(folder.resolve())
        return True
    except ValueError:
        return False


class RelativeLinkingTool(CommonAssetTool):
    """
    Takes every file in an input folder and creates an output symlink that
    points to the input file using a relative path.

    Relative symlinks work correctly inside containers where the absolute host
    path is not valid.

    The output file preserves the relative path from the input folder.
    For example, if the input file is `imgs/penguin.png` relative to the
    input folder, the output will be `<output_folder>/imgs/penguin.png` -> a
    relative symlink pointing back to the input file.
    """

    def __init__(self, pattern=r".*"):
        super().__init__()
        self.pattern = pattern

    def tool_name(self):
        return "RelativeLinkingTool"

    def check_match(self, file_path: Path) -> bool:
        return in_folder(file_path, self.input_folder) and bool(
            re.match(self.pattern, str(file_path), re.IGNORECASE)
        )

    def define_dependencies(self, file_path: Path) -> List[Path]:
        return []

    def define_outputs(self, file_path: Path) -> List[Path]:
        return [
            self.output_folder
            / file_path.resolve().relative_to(self.input_folder.resolve())
        ]

    @staticmethod
    def build(file_path: Path, input_folder: Path, output_folder: Path) -> None:
        """
        Creates a relative symbolic link in the output folder pointing to the
        input file.

        The symlink target is expressed as a path relative to the symlink's
        own parent directory so that it remains valid inside a container where
        the repo is mounted at a different absolute path.
        """
        input_file = file_path.resolve()
        output_file = output_folder / input_file.relative_to(input_folder.resolve())

        output_file.parent.mkdir(parents=True, exist_ok=True)

        if output_file.exists() or output_file.is_symlink():
            os.remove(output_file)

        relative_target = Path(
            os.path.relpath(input_file, start=output_file.parent)
        )

        try:
            output_file.symlink_to(relative_target)
            print(f"Created relative symlink: {output_file} -> {relative_target}")
        except Exception as e:
            print(f"Error creating symlink for {input_file} at {output_file}: {e}")
