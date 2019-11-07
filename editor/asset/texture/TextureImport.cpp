#include "core/Core.hpp"

#include "app/GfxTest.hpp"

#include "Compressonator.h"

// TODO: use IO callbacks
//#define STBI_NO_STDIO

// TODO: use a thread-local allocator
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STBI_ASSERT(x)         ASSERT(x)
#define STBI_MALLOC(sz)        malloc(sz)
#define STBI_REALLOC(p, newsz) realloc(p, newsz)
#define STBI_FREE(p)           free(p)
#include "stb_image.h"

#if 0
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBIR_ASSERT(x)       STBI_ASSERT(x)
#define STBIR_MALLOC(size, c) (static_cast<void>(c), STBI_MALLOC(size))
#define STBIR_FREE(ptr, c)    (static_cast<void>(c), STBI_FREE(ptr))
#include "stb_image_resize.h"
#endif

DECL_LOG_SOURCE(ImportImage, Info);

static bool compressCallback(f32 progress,
                             CMP_DWORD_PTR user1 UNUSED,
                             CMP_DWORD_PTR user2 UNUSED)
{
  LOG(ImportImage, Trace, "Progress: %.2f%%", progress);
  return false;
}

u32 loadImage(char const* filename, CMP_Texture const& tex, TextureType type);

static u32 getSrcPixelSize(TextureType type) {
  switch (type) {
  case TextureType::Base: return 4; // TODO RGB
  case TextureType::Normal: return 3;
  default: return 1;
  }
}

static CMP_FORMAT getSrcFormat(TextureType type) {
  switch (type) {
  case TextureType::Base: return CMP_FORMAT_RGBA_8888; // TODO RGB
  case TextureType::Normal: return CMP_FORMAT_RGB_888;
  default: return CMP_FORMAT_R_8;
  }
}

u32 importTextureImpl(char const* filename, stbi_uc* image, i32 w, i32 h, TextureType type) {
  if (!image) {
    LOG(ImportImage, Error, "Failed to load image: %s", stbi_failure_reason());
    return 0;
  }

  CMP_Texture src{};
  src.dwSize = sizeof(src);
  src.dwWidth = w;
  src.dwHeight = h;
  src.format = getSrcFormat(type);
  src.dwDataSize = w * h * getSrcPixelSize(type);
  src.pData = image;

  CMP_Texture dst{};
  dst.dwSize = sizeof(dst);
  dst.dwWidth = w;
  dst.dwHeight = h;
  dst.format = CMP_FORMAT_DXT5; // TODO more than base
  dst.dwDataSize = CMP_CalculateBufferSize(&dst);
  dst.pData = new CMP_BYTE[dst.dwDataSize];

  CMP_CompressOptions opts{};
  opts.dwSize = sizeof(opts);
  opts.fquality = 1;// .05f;
  //opts.bDisableMultiThreading = true;
  opts.dwnumThreads = 4;

  u32 id = 0;

  if (type == TextureType::Base) {
    auto status{ CMP_ConvertTexture(&src, &dst, &opts, compressCallback, 0, 0) };
    if (status != CMP_OK) {
      char const* err;
      switch (status) {
      #define E(x) case CMP_ERR_##x: err = #x; break
        E(INVALID_SOURCE_TEXTURE);
        E(INVALID_DEST_TEXTURE);
        E(UNSUPPORTED_SOURCE_FORMAT);
        E(UNSUPPORTED_DEST_FORMAT);
        E(UNSUPPORTED_GPU_ASTC_DECODE);
        E(SIZE_MISMATCH);
        E(UNABLE_TO_INIT_CODEC);
        E(UNABLE_TO_INIT_DECOMPRESSLIB);
        E(UNABLE_TO_INIT_COMPUTELIB);
        E(GENERIC);
      #undef E
      default: err = "Unknown";
      }
      LOG(ImportImage, Error, "Image compression error: %s", err);
    }
    else {
      // TODO save file
      id = loadImage(filename, dst, type);
    }
  }
  else {
    id = loadImage(filename, src, type);
  }

  delete[] dst.pData;
  stbi_image_free(image);

  return id;
}

static u32 getStbFormat(TextureType type) {
  switch (type) {
  case TextureType::Base: return STBI_rgb_alpha; // TODO RGB
  case TextureType::Normal: return STBI_rgb;
  default: return STBI_grey;
  }
}

u32 importTextureData(char const* filename, void const* data, usize size, TextureType type) {
  i32 w, h, channels;
  auto image{ stbi_load_from_memory(static_cast<stbi_uc const*>(data), size,
                                    &w, &h, &channels, getStbFormat(type)) };

  return importTextureImpl(filename, image, w, h, type);
}

u32 importTexture(char const* filename, TextureType type) {
  stbi_convert_iphone_png_to_rgb(true);

  i32 w, h, channels;
  auto image{ stbi_load(filename, &w, &h, &channels, getStbFormat(type)) };
  return importTextureImpl(filename, image, w, h, type);
}

u32 loadImageHDR(char const* filename, void const* data, u32 w, u32 h);
u32 importTextureHDR(char const* filename) {
  i32 w, h, channels;
  auto image{ stbi_loadf(filename, &w, &h, &channels, 0) };
  auto id = loadImageHDR(filename, image, w, h);
  stbi_image_free(image);
  return id;
}
