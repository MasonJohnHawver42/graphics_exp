import AssetForge

from pathlib import Path

from amake import atlas, cubemap, primitives, shader

AssetForge.RegisterTool(AssetForge.common.CompressionTool(),                  priority=5) 
AssetForge.RegisterTool(atlas.AtlasTool(),                                    priority=3)
AssetForge.RegisterTool(cubemap.CubemapTool(),                                priority=4)  
AssetForge.RegisterTool(primitives.PrimitivesTool(),                          priority=4)  
AssetForge.RegisterTool(shader.ShaderTool(),                                  priority=5)
AssetForge.RegisterTool(AssetForge.common.IgnoreItToolDecorator(AssetForge.common.LinkingTool(), "linkignore"),                      priority=0)  

AssetForge.Build(Path("assets"), Path("runtime"), recursive=True, parallel=False, debug=False, quiet=True)