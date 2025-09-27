import AssetForge

import os
import io
import sys
import json
from pathlib import Path
from typing import List, Dict, Tuple

from PIL import Image
from pygltflib import GLTF2, TextureInfo

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

# from amake.data.gl import ShaderFile, ShaderEntry, ShaderStage

import flatbuffers

from amake.data.gl.pbr.data import (
    PBRCache   as FBCache,
    Texture    as FBTexture,
    TexturePool as FBTexturePool,
    Material   as FBMaterial,
    Model      as FBModel,
    TextureType as FBTT,
    TextureRes  as FBTR
)


# class PBRDefualt 

_TEX_SIZE_POOLS = [(512, 512, 4), (1024, 1024, 4), (2048, 2048, 4)]

def _ClipImage(img: Image.Image, allowed_sizes: list[tuple[int, int, int]]) -> Image.Image:
    width, height = img.size
    img_channels = len(img.getbands())

    # Filter sizes that are smaller than or equal to the image
    valid_sizes = [
        (w, h, c) for (w, h, c) in allowed_sizes
        if w <= width and h <= height
    ]

    if not valid_sizes:
        raise ValueError(f"No valid size found for image of size ({width}, {height}, {img_channels})")

    # Choose the largest valid size by area
    best_w, best_h, best_c = max(valid_sizes, key=lambda s: s[0] * s[1])

    index = allowed_sizes.index((best_w, best_h, best_c))

    # Convert image to match the required number of channels
    mode_map = {1: "L", 3: "RGB", 4: "RGBA"}
    if best_c != img_channels:
        if best_c not in mode_map:
            raise ValueError(f"Unsupported channel count {best_c}")
        img = img.convert(mode_map[best_c])

    # Crop the top-left region
    return img.crop((0, 0, best_w, best_h)), index

def _DefaultTextures(size=(512, 512, 4)):
    width, height, c = size
    mode_map = {1: "L", 3: "RGB", 4: "RGBA"}

    def _CheckerboardImage(mode, size: tuple[int, int], square_size=64, color1=(0, 0, 0, 255), color2=(255, 0, 0, 255)) -> Image.Image:
        width, height = size

        img = Image.new(mode, size)
        pixels = img.load()

        for y in range(height):
            for x in range(width):
                if ((x // square_size) + (y // square_size)) % 2 == 0:
                    pixels[x, y] = color1
                else:
                    pixels[x, y] = color2

        return img

    # Albedo: white (1.0, 1.0, 1.0)
    albedo = _CheckerboardImage(mode_map[c], (width, height))

    # MRAO (Metallic-Roughness-AO): default = (0, 255, 255)
    # Metallic = 0 (non-metal), Roughness = 1 (255), AO = 1 (255)
    mrao = Image.new(mode_map[c], (width, height), (0, 255, 255, 255))

    # Normal Map: default = (128, 128, 255) — normal pointing straight up in tangent space
    normals = Image.new(mode_map[c], (width, height), (128, 128, 255, 255))

    # Emissive: default = black (0, 0, 0)
    emissive = Image.new(mode_map[c], (width, height), (0, 0, 0, 255))

    return {
        "albedo": albedo,
        "mrao": mrao,
        "normals": normals,
        "emissive": emissive,
    }

def _ExtractSaveEmbeddedImages(glb_path: Path, output_dir: Path):
    output_dir.mkdir(parents=True, exist_ok=True)

    gltf = GLTF2().load(glb_path)
    binary_blob = open(glb_path, "rb").read()

    saved_images = {}  # bufferView_index → saved file path

    def save_embedded_image(img_index):
        img = gltf.images[img_index]
        if img.uri: return None  # Already external

        bv = gltf.bufferViews[img.bufferView]
        buf = gltf.buffers[bv.buffer]

        byte_offset = bv.byteOffset or 0
        byte_length = bv.byteLength
        img_bytes = binary_blob[bv.byteOffset : byte_offset + byte_length]

        mime = img.mimeType or "image/png"
        ext = ".jpg" if "jpeg" in mime else ".png"
        file_name = f"image_{img_index}{ext}"
        file_path = output_dir / file_name

        if img.bufferView in saved_images:
            return saved_images[img.bufferView]

        with Image.open(io.BytesIO(img_bytes)) as pil_img:
            pil_img.save(file_path)
            saved_images[img.bufferView] = file_path
            print(f"✅ Saved: {file_path}")
            return file_path

    # Walk through materials and extract any referenced textures
    for i, mat in enumerate(gltf.materials):
        print(f"\n🎨 Material {i}: {mat.name or 'Unnamed'}")

        def try_texture(tex_info, label):
            if tex_info and isinstance(tex_info, TextureInfo):
                tex_index = tex_info.index
                if tex_index is not None and tex_index < len(gltf.textures):
                    img_index = gltf.textures[tex_index].source
                    saved_path = save_embedded_image(img_index)
                    if saved_path:
                        print(f"   - {label}: {saved_path.name}")

        try_texture(getattr(mat.pbrMetallicRoughness, 'baseColorTexture', None), "Albedo")
        try_texture(getattr(mat.pbrMetallicRoughness, 'metallicRoughnessTexture', None), "M/R")
        try_texture(mat.normalTexture, "Normal")
        try_texture(mat.occlusionTexture, "AO")
        try_texture(mat.emissiveTexture, "Emissive")

def _PackAlbedo(gltf, model_path, texture_dir):
    images = gltf.images
    textures = gltf.textures
    materials = gltf.materials

    albedo = []

    for i, mat in enumerate(materials):
        print(f"\n🎨 Material {i}: {mat.name or 'Unnamed'}")
        
        albedo_img = None

        if mat.pbrMetallicRoughness and isinstance(mat.pbrMetallicRoughness.baseColorTexture, TextureInfo):
            tex_index = mat.pbrMetallicRoughness.baseColorTexture.index
            img_index = textures[tex_index].source
            img_path = model_path.parent / images[img_index].uri

            with Image.open(img_path) as img:
                width, height = img.size
                img = img.convert("RGB")
                albedo_img = img

            print(f"   - ALbedo: {images[img_index].uri} - {width} x {height}")
        
        combined_path = texture_dir / f"{model_path.stem}_mat_{mat.name or 'Unnamed'}_{i}_albedo.png"
        combined_path.parent.mkdir(parents=True, exist_ok=True)

        if albedo_img:
            albedo_img, index = _ClipImage(albedo_img, _TEX_SIZE_POOLS)

            albedo_img.save(combined_path)
            albedo.append((combined_path, index))
            print(f"   ✅ Saved ALBEDO: {combined_path}")
        else:
            albedo.append((None, None))
    
    return albedo


def _PackMRandAO(gltf, model_path, texture_dir):
    images = gltf.images
    textures = gltf.textures
    materials = gltf.materials

    mrao = []

    for i, mat in enumerate(materials):
        print(f"\n🎨 Material {i}: {mat.name or 'Unnamed'}")
        
        roughness_img = None
        metallic_img = None
        ao_img = None

        # -- Metallic-Roughness (often packed in 2 channels)
        if mat.pbrMetallicRoughness and isinstance(mat.pbrMetallicRoughness.metallicRoughnessTexture, TextureInfo):
            tex_index = mat.pbrMetallicRoughness.metallicRoughnessTexture.index
            img_index = textures[tex_index].source
            img_path = model_path.parent / images[img_index].uri

            with Image.open(img_path) as img:
                width, height = img.size
                img = img.convert("RGB")
                roughness_img = img.getchannel("G")  # usually G = roughness
                metallic_img = img.getchannel("B")   # usually B = metallic

            print(f"   - MR: {images[img_index].uri} - {width} x {height}")

        # -- AO (single channel)
        if mat.occlusionTexture:
            tex_index = mat.occlusionTexture.index
            img_index = textures[tex_index].source
            img_path = model_path.parent / images[img_index].uri

            with Image.open(img_path) as img:
                img = img.convert("L")
                ao_img = img
                print(f"   - AO: {images[img_index].uri} - {img.size[0]} x {img.size[1]}")

        combined_path = texture_dir / f"{model_path.stem}_mat_{mat.name or 'Unnamed'}_{i}_mrao.png"
        combined_path.parent.mkdir(parents=True, exist_ok=True)

        # -- If both MR and AO exist, combine
        if roughness_img and metallic_img and ao_img:
            # Match AO size to MR size
            if ao_img.size != roughness_img.size:
                ao_img = ao_img.resize(roughness_img.size, Image.BICUBIC)

            # Create RGBA: R = Roughness, G = Metallic, B = AO, A = 255
            combined = Image.merge("RGBA", (
                roughness_img,
                metallic_img,
                ao_img,
                Image.new("L", roughness_img.size, 255)
            ))

            combined, index = _ClipImage(combined, _TEX_SIZE_POOLS)

            combined.save(combined_path)
            mrao.append((combined_path, i))
            print(f"   ✅ Saved combined MRAO: {combined_path}")
        
        elif roughness_img and metallic_img:
            combined = Image.merge("RGBA", (
                roughness_img,
                metallic_img,
                Image.new("L", roughness_img.size, 255),
                Image.new("L", roughness_img.size, 255)
            ))

            combined, index = _ClipImage(combined, _TEX_SIZE_POOLS)

            combined.save(combined_path)
            mrao.append((combined_path, index))
            print(f"   ✅ Saved combined MRAO: {combined_path}")
        
        else:
            mrao.append((None, None))
    
    return mrao


def _PackNormal(gltf, model_path, texture_dir):
    images = gltf.images
    textures = gltf.textures
    materials = gltf.materials

    normals = []

    for i, mat in enumerate(materials):
        print(f"\n🧭 Material {i}: {mat.name or 'Unnamed'}")
        normal_img = None

        if mat.normalTexture:
            tex_index = mat.normalTexture.index
            img_index = textures[tex_index].source
            img_path = model_path.parent / images[img_index].uri

            with Image.open(img_path) as img:
                width, height = img.size
                img = img.convert("RGB")  # normals are usually RGB
                normal_img = img

            print(f"   - Normal: {images[img_index].uri} - {width} x {height}")

        combined_path = texture_dir / f"{model_path.stem}_mat_{mat.name or 'Unnamed'}_{i}_normal.png"
        combined_path.parent.mkdir(parents=True, exist_ok=True)

        if normal_img:
            normal_img, index = _ClipImage(normal_img, _TEX_SIZE_POOLS)

            # Optionally: Invert green channel if Y-axis needs flipping
            # r, g, b = normal_img.split()
            # g = ImageChops.invert(g)
            # normal_img = Image.merge("RGB", (r, g, b))

            normal_img.save(combined_path)
            normals.append((combined_path, index))
            print(f"   ✅ Saved NORMAL: {combined_path}")
        else:
            normals.append((None, None))

    return normals


def _PackEmissive(gltf, model_path, texture_dir):
    images = gltf.images
    textures = gltf.textures
    materials = gltf.materials

    emissives = []

    for i, mat in enumerate(materials):
        print(f"\n💡 Material {i}: {mat.name or 'Unnamed'}")
        emissive_img = None

        if mat.emissiveTexture:
            tex_index = mat.emissiveTexture.index
            img_index = textures[tex_index].source
            img_path = model_path.parent / images[img_index].uri

            with Image.open(img_path) as img:
                width, height = img.size
                img = img.convert("RGB")
                emissive_img = img

            print(f"   - Emissive: {images[img_index].uri} - {width} x {height}")

        combined_path = texture_dir / f"{model_path.stem}_mat_{mat.name or 'Unnamed'}_{i}_emissive.png"
        combined_path.parent.mkdir(parents=True, exist_ok=True)

        if emissive_img:
            emissive_img, index = _ClipImage(emissive_img, _TEX_SIZE_POOLS)

            emissive_img.save(combined_path)
            emissives.append((combined_path, index))
            print(f"   ✅ Saved EMISSIVE: {combined_path}")
        else:
            emissives.append((None, None))

    return emissives

def build_cache(gltf_paths: list[Path], out_bin: Path, texture_dir: Path):
    builder = flatbuffers.Builder(0)

    tex_entries = []
    mat_entries = []
    model_entries = []

    # 1) Ensure default textures in directory
    defaults = _DefaultTextures()
    default_uuids = {}
    for ttype, img in defaults.items():
        buf = io.BytesIO()
        img.save(buf, format='PNG')
        path = texture_dir / f"default_{ttype}.png"
        path.parent.mkdir(exist_ok=True)
        path.write_bytes(buf.getvalue())
        # record uuid and FB offsets...

    # 2) Process each glTF
    for mp in gltf_paths:
        gltf = GLTF2().load(str(mp))
        # Pack each texture type
        albedos = _PackAlbedo(gltf, mp, texture_dir)
        mraos   = _PackMRandAO(gltf, mp, texture_dir)
        normals = _PackNormal(gltf, mp, texture_dir)
        emissives = _PackEmissive(gltf, mp, texture_dir)

        # Create FB Texture entries
        for tex_list, ttype in [(albedos, FBTT.TextureType.Albedo),
                                 (mraos, FBTT.TextureType.MRAO),
                                 (normals, FBTT.TextureType.Normal),
                                 (emissives, FBTT.TextureType.Emissive)]:
            for path, pool_idx in tex_list:
                uuid = generate_unique_uuid(path)
                res = FBTR.TextureRes(pool_idx)
                txt_offset = FBTexture.CreateTexture(builder,
                                  builder.CreateString(str(path)),
                                  uuid,
                                  ttype,
                                  res)
                tex_entries.append(txt_offset)

        # Material FB entries
        for mat in gltf.materials:
            m_uuid = generate_unique_uuid(mat.name)
            # lookup texture UUIDs by mat index
            mat_off = FBMaterial.CreateMaterial(builder,
                          m_uuid,
                          albedo_uuid,
                          mrao_uuid,
                          normal_uuid,
                          emissive_uuid)
            mat_entries.append(mat_off)

        # Model FB entry
        model_off = FBModel.CreateModel(builder,
                            generate_unique_uuid(mp.stem),
                            FBModel.CreateMaterialsVector(builder, mat_entries[-len(gltf.materials):]))
        model_entries.append(model_off)

    # 3) Create vectors
    tex_vec = FBCache.CreateTexturesVector(builder, tex_entries)
    mat_vec = FBCache.CreateMaterialsVector(builder, mat_entries)
    model_vec = FBCache.CreateModelsVector(builder, model_entries)

    FBCache.Start(builder)
    FBCache.AddTextures(builder, tex_vec)
    FBCache.AddMaterials(builder, mat_vec)
    FBCache.AddModels(builder, model_vec)
    root = FBCache.End(builder)
    builder.Finish(root)

    out_bin.write_bytes(builder.Output())


def _WriteTexturePoolBin(output_path, pools_data, default_paths):
    """
    Write texture pools binary to output_path using FlatBuffers.

    pools_data: list of entries, each:
      [pool_name(str), width(int), height(int), list_of_image_paths(str)]
    """
    builder = flatbuffers.Builder(1024)

    def _get_texture_pool_type(name):
        """
        Map pool name string to TexturePoolType enum value.
        Adjust these mappings if your enum differs.
        """
        if name.lower() == "albedo":
            return TexturePoolType.TexturePoolType.Albedo
        elif name.lower() == "mrao":
            return TexturePoolType.TexturePoolType.MRAO
        elif name.lower() == "normal":
            return TexturePoolType.TexturePoolType.Normal
        elif name.lower() == "emissive":
            return TexturePoolType.TexturePoolType.Emissive
        else:
            return TexturePoolType.TexturePoolType.Albedo

    # Build each TexturePool and collect their offsets
    pool_offsets = []

    for pool_entry in pools_data:
        # Unpack
        pool_name, width, height, channels, image_paths = pool_entry

        # Create strings in builder for each image path
        image_offsets = []
        for img_path in image_paths:
            image_offsets.append(builder.CreateString(str(img_path)))

        # Create vector of image strings
        TexturePool.TexturePoolStartImagePathsVector(builder, len(image_offsets))
        for offset in reversed(image_offsets):
            builder.PrependUOffsetTRelative(offset)
        textures_vector = builder.EndVector(len(image_offsets))

        # Create name string
        # name_offset = builder.CreateString(pool_name)

        # Start TexturePool object
        TexturePool.TexturePoolStart(builder)
        TexturePool.TexturePoolAddPoolType(builder, _get_texture_pool_type(pool_name))
        TexturePool.TexturePoolAddWidth(builder, width)
        TexturePool.TexturePoolAddHeight(builder, height)
        TexturePool.TexturePoolAddChannels(builder, channels)
        TexturePool.TexturePoolAddImagePaths(builder, textures_vector)
        pool_offset = TexturePool.TexturePoolEnd(builder)
        pool_offsets.append(pool_offset)

    # Create vector of TexturePool offsets
    TexturePoolList.TexturePoolListStartPoolsVector(builder, len(pool_offsets))
    for po in reversed(pool_offsets):
        builder.PrependUOffsetTRelative(po)
    pools_vector = builder.EndVector(len(pool_offsets))

    # Build strings in reverse order and collect their offsets
    str_offsets = [builder.CreateString(s) for s in reversed(default_paths)]

    TexturePoolList.TexturePoolListStartDefaultsVector(builder, len(str_offsets))
    for offset in str_offsets:
        builder.PrependUOffsetTRelative(offset)
    defaults_vector = builder.EndVector()

    # Build root object
    TexturePoolList.TexturePoolListStart(builder)
    TexturePoolList.TexturePoolListAddPools(builder, pools_vector)
    TexturePoolList.TexturePoolListAddDefaults(builder, defaults_vector)
    root = TexturePoolList.TexturePoolListEnd(builder)

    builder.Finish(root)

    # Write to file
    with open(output_path, "wb") as f:
        f.write(builder.Output())

class PBRTexture(AssetForge.AssetTool):
    def tool_name(self):
        return "PBRTextures"
    
    def check_match(self, file_path: Path) -> bool:
        # Accept files with the .atals extension.
        return ("".join(file_path.suffixes) == ".json") and file_path.is_relative_to(self.input_folder) and file_path.name == "pbr_texture_preproc.json"

    def define_dependencies(self, file_path: Path) -> List[Path]:
        try:
            with file_path.open("r", encoding="utf-8") as f:
                data = json.load(f)

            if data.get("version", "") != "1.0":
                print(f"Unsupported PBR Texture Preproc version in {file_path}")
                return [] #[Path(__file__).resolve()]

            return [file_path.parent / Path(f) for f in data.get("models", [])] #+ [Path(__file__).resolve()]
        except Exception as e:
            print(f"PBR Texture Preproc error reading {file_path}: {e}")
        return [] #[Path(__file__).resolve()]

    def define_outputs(self, file_path: Path) -> List[Path]:
        bin_path = self.output_folder / file_path.parent.relative_to(self.input_folder) / "base.texture_pool.bin"
        return [bin_path]
    
    @staticmethod
    def build(file_path: Path, input_folder: Path, output_folder: Path) -> None:
        with open(file_path, 'r') as f:
            data = json.load(f)
        
        if data.get("version", "") != "1.0":
            print(f"Unsupported PBR Texture Preproc version in {file_path}")
            return
                
        models = data.get("models", [])
        texture_dir = output_folder / file_path.parent.relative_to(input_folder) / data.get("texture_dir", "textures")

        defaults = _DefaultTextures()

        texture_dir.mkdir(parents=True, exist_ok=True)

        defaults["albedo"].save(texture_dir / "default_albedo.png")
        defaults["mrao"].save(texture_dir / "default_mrao.png")
        defaults["normals"].save(texture_dir / "default_normal.png")
        defaults["emissive"].save(texture_dir / "default_emissive.png")

        all_albedo = []
        all_mrao = []
        all_normal = []
        all_emissive = []

        for model in models:
            model_path = input_folder / file_path.parent.relative_to(input_folder) / Path(model)
            
            if not model_path.exists():
                print(f"Model file {model_path} does not exist.")
                continue
            
            # _ExtractSaveEmbeddedImages(model_path, model_path.parent)
            
            gltf = GLTF2().load(model_path)

            all_albedo += _PackAlbedo(gltf, model_path, texture_dir)
            all_mrao += _PackMRandAO(gltf, model_path, texture_dir)
            all_normal += _PackNormal(gltf, model_path, texture_dir)
            all_emissive += _PackEmissive(gltf, model_path, texture_dir)
        
        bin_path = output_folder / file_path.parent.relative_to(input_folder) / "base.texture_pool.bin"
        
        # print([("Test", *size, [os.path.relpath(img, bin_path.parent) for img, index in all_albedo if index and index == i]) for i, size in enumerate(_TEX_SIZE_POOLS)])

        # # print([img for img, index in all_albedo if index and index == 1])

        # print(
        #     [("Albedo", *size, [os.path.relpath(img, bin_path.parent) for img, index in all_albedo if index and index == i] + ([os.path.relpath(texture_dir / "default_albedo.png", bin_path.parent)] if i == 0 else [])) for i, size in enumerate(_TEX_SIZE_POOLS)] +
        #     [("MRAO", *size, [os.path.relpath(img, bin_path.parent) for img, index in all_mrao if index and index == i] + ([os.path.relpath(texture_dir / "default_mrao.png", bin_path.parent)] if i == 0 else [])) for i, size in enumerate(_TEX_SIZE_POOLS)] +
        #     [("Normal", *size, [os.path.relpath(img, bin_path.parent) for img, index in all_normal if index and index == i] + ([os.path.relpath(texture_dir / "default_normal.png", bin_path.parent)] if i == 0 else [])) for i, size in enumerate(_TEX_SIZE_POOLS)] +
        #     [("Emissive", *size, [os.path.relpath(img, bin_path.parent) for img, index in all_emissive if index and index == i] + ([os.path.relpath(texture_dir / "default_emissive.png", bin_path.parent)] if i == 0 else [])) for i, size in enumerate(_TEX_SIZE_POOLS)]
        # )

        _WriteTexturePoolBin(
            bin_path,
            [("Albedo", *size, [os.path.relpath(img, output_folder) for img, index in all_albedo if index and index == i] + ([os.path.relpath(texture_dir / "default_albedo.png", output_folder)] if i == 0 else [])) for i, size in enumerate(_TEX_SIZE_POOLS)] +
            [("MRAO", *size, [os.path.relpath(img, output_folder) for img, index in all_mrao if index and index == i] + ([os.path.relpath(texture_dir / "default_mrao.png", output_folder)] if i == 0 else [])) for i, size in enumerate(_TEX_SIZE_POOLS)] +
            [("Normal", *size, [os.path.relpath(img, output_folder) for img, index in all_normal if index and index == i] + ([os.path.relpath(texture_dir / "default_normal.png", output_folder)] if i == 0 else [])) for i, size in enumerate(_TEX_SIZE_POOLS)] +
            [("Emissive", *size, [os.path.relpath(img, output_folder) for img, index in all_emissive if index and index == i] + ([os.path.relpath(texture_dir / "default_emissive.png", output_folder)] if i == 0 else [])) for i, size in enumerate(_TEX_SIZE_POOLS)],
            [
                os.path.relpath(texture_dir / "default_albedo.png", output_folder),
                os.path.relpath(texture_dir / "default_mrao.png", output_folder),
                os.path.relpath(texture_dir / "default_normal.png", output_folder),
                os.path.relpath(texture_dir / "default_emissive.png", output_folder)
            ]
        )




if __name__ == "__main__":
    tools = [PBRTexture()]
    AssetForge.common.main_func(tools)