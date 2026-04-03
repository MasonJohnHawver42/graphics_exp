import AssetForge

from pathlib import Path

from amake.pipeline import atlas, cubemap, primitives, shader, linking

AssetForge.RegisterTool(AssetForge.common.CompressionTool(),                  priority=5) 
AssetForge.RegisterTool(atlas.AtlasTool(),                                    priority=3)
AssetForge.RegisterTool(cubemap.CubemapTool(),                                priority=4)  
AssetForge.RegisterTool(primitives.PrimitivesTool(),                          priority=4)  
AssetForge.RegisterTool(shader.ShaderTool(),                                  priority=5)
# AssetForge.RegisterTool(pbr.PBRTexture(),                                     priority=5)
AssetForge.RegisterTool(linking.RelativeLinkingTool(),                        priority=0)  

repo_root  = Path(__file__).parent.parent
assets_in  = (Path(__file__).parent / "assets").resolve()
assets_out = (repo_root / "build" / "BUILD_AM").resolve()

AssetForge.Build(assets_in, assets_out, Path(__file__).parent / "requirements.txt", debug=False, quiet=True)