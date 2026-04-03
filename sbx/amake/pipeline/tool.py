import AssetForge

from .atlas import AtlasTool
from .cubemap import CubemapTool
from .linking import RelativeLinkingTool
from .pbr import PBRTexture
from .primitives import PrimitivesTool
from .shader import ShaderTool

if __name__ == "__main__":
    tools = [AtlasTool(), CubemapTool(), PBRTexture(), PrimitivesTool(), ShaderTool(), RelativeLinkingTool()]
    AssetForge.common.main_func(tools)