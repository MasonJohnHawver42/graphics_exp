import AssetForge

from pathlib import Path

from amake import atlas, cubemap, primitives, shader, pbr

AssetForge.RegisterTool(AssetForge.common.CompressionTool(),                  priority=5) 
AssetForge.RegisterTool(atlas.AtlasTool(),                                    priority=3)
AssetForge.RegisterTool(cubemap.CubemapTool(),                                priority=4)  
AssetForge.RegisterTool(primitives.PrimitivesTool(),                          priority=4)  
AssetForge.RegisterTool(shader.ShaderTool(),                                  priority=5)
AssetForge.RegisterTool(pbr.PBRTexture(),                                     priority=5)
AssetForge.RegisterTool(AssetForge.common.LinkingTool(),                      priority=0)  

AssetForge.Build(Path("assets").resolve(), Path("build").resolve(), Path("/home/mjh/Programming/fun/graphics_exp/sbx/amake/requirements.txt"), debug=True, quiet=True)