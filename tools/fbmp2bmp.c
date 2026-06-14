// fbmp2bmp.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t type;      // 0x4D42 = "BM"
    uint32_t size;      // 文件大小
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;    // 像素数据偏移 (54)
} BMPHeader;

typedef struct {
    uint32_t size;          // 40
    int32_t width;
    int32_t height;
    uint16_t planes;        // 1
    uint16_t bits_per_pixel;// 24
    uint32_t compression;   // 0
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} BMPInfoHeader;
#pragma pack(pop)

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "用法: %s <input.fbmp> <output.bmp>\n", argv[0]);
        return 1;
    }

    FILE* fin = fopen(argv[1], "rb");
    if (!fin) {
        perror("打开输入文件失败");
        return 1;
    }

    // 读取 fbmp 头
    uint32_t magic, width, height;
    fread(&magic, 4, 1, fin);
    fread(&width, 4, 1, fin);
    fread(&height, 4, 1, fin);

    if (magic != 0x706D6266) { // "fbmp"
        fprintf(stderr, "错误: 不是有效的 fbmp 文件\n");
        fclose(fin);
        return 1;
    }

    printf("尺寸: %u x %u\n", width, height);

    // 读取像素数据 (BGRA)
    size_t pixel_count = width * height;
    uint8_t* pixels = malloc(pixel_count * 4);
    fread(pixels, 1, pixel_count * 4, fin);
    fclose(fin);

    // 创建 BMP 文件
    FILE* fout = fopen(argv[2], "wb");
    if (!fout) {
        perror("创建输出文件失败");
        free(pixels);
        return 1;
    }

    // BMP 行大小（每行字节数，对齐到4字节）
    int row_size = ((width * 3 + 3) & ~3);
    int image_size = row_size * height;

    BMPHeader header = {
        .type = 0x4D42,
        .size = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + image_size,
        .offset = sizeof(BMPHeader) + sizeof(BMPInfoHeader)
    };
    BMPInfoHeader info = {
        .size = 40,
        .width = width,
        .height = height,
        .planes = 1,
        .bits_per_pixel = 24,
        .compression = 0,
        .image_size = image_size
    };

    fwrite(&header, sizeof(header), 1, fout);
    fwrite(&info, sizeof(info), 1, fout);

    // 转换 BGRA -> BGR，并翻转上下（BMP 是从下到上）
    uint8_t* row_buffer = malloc(row_size);
    for (int y = height - 1; y >= 0; y--) {
        uint8_t* src = pixels + y * width * 4;
        uint8_t* dst = row_buffer;
        for (uint32_t x = 0; x < width; x++) {
            dst[0] = src[0];  // B
            dst[1] = src[1];  // G
            dst[2] = src[2];  // R
            src += 4;
            dst += 3;
        }
        fwrite(row_buffer, row_size, 1, fout);
    }

    free(row_buffer);
    free(pixels);
    fclose(fout);

    printf("转换成功: %s\n", argv[2]);
    return 0;
}

/*
# 编译
gcc -o ./tools/fbmp2bmp ./tools/fbmp2bmp.c

# 转换
./tools/fbmp2bmp root/base/data/wallpaper/bluewhite.fbmp wallpaper.bmp
*/