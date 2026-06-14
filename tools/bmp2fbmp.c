// tools/bmp2fbmp.c - 将 BMP 转换为 PatchworkOS 的 fbmp 格式

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
    uint32_t offset;    // 像素数据偏移
} BMPHeader;

typedef struct {
    uint32_t size;          // 信息头大小 (40)
    int32_t  width;
    int32_t  height;
    uint16_t planes;        // 1
    uint16_t bits_per_pixel; // 24 或 32
    uint32_t compression;   // 0 = 无压缩
    uint32_t image_size;
    int32_t  x_pixels_per_meter;
    int32_t  y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} BMPInfoHeader;
#pragma pack(pop)

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "用法: %s <input.bmp> <output.fbmp>\n", argv[0]);
        fprintf(stderr, "示例: %s wallpaper.bmp wallpaper.fbmp\n", argv[0]);
        return 1;
    }

    // 打开 BMP 文件
    FILE* fin = fopen(argv[1], "rb");
    if (!fin) {
        perror("打开输入文件失败");
        return 1;
    }

    // 读取 BMP 头
    BMPHeader header;
    BMPInfoHeader info;
    
    fread(&header, sizeof(header), 1, fin);
    fread(&info, sizeof(info), 1, fin);

    // 验证 BMP 格式
    if (header.type != 0x4D42) {
        fprintf(stderr, "错误: 不是有效的 BMP 文件\n");
        fclose(fin);
        return 1;
    }

    if (info.bits_per_pixel != 24 && info.bits_per_pixel != 32) {
        fprintf(stderr, "错误: 只支持 24-bit 或 32-bit BMP (当前: %d-bit)\n", info.bits_per_pixel);
        fclose(fin);
        return 1;
    }

    int width = info.width;
    int height = info.height > 0 ? info.height : -info.height;
    int is_top_down = info.height < 0;  // BMP 从上到下存储的标志

    printf("尺寸: %d x %d, %d-bit\n", width, height, info.bits_per_pixel);

    // 计算 BMP 每行字节数（对齐到 4 字节）
    int bmp_row_size = ((width * (info.bits_per_pixel / 8) + 3) & ~3);
    
    // 跳转到像素数据
    fseek(fin, header.offset, SEEK_SET);

    // 读取所有像素数据
    uint8_t* bmp_data = malloc(bmp_row_size * height);
    if (!bmp_data) {
        fprintf(stderr, "错误: 内存不足\n");
        fclose(fin);
        return 1;
    }
    fread(bmp_data, 1, bmp_row_size * height, fin);
    fclose(fin);

    // 打开 fbmp 输出文件
    FILE* fout = fopen(argv[2], "wb");
    if (!fout) {
        perror("创建输出文件失败");
        free(bmp_data);
        return 1;
    }

    // 写入 fbmp 头
    uint32_t magic = 0x706D6266;  // "fbmp"
    uint32_t fbmp_width = width;
    uint32_t fbmp_height = height;
    
    fwrite(&magic, 4, 1, fout);
    fwrite(&fbmp_width, 4, 1, fout);
    fwrite(&fbmp_height, 4, 1, fout);

    // 转换并写入像素数据 (BGRA 格式)
    // fbmp 存储顺序：从上到下，每个像素 4 字节 (B, G, R, A)
    // Alpha 通道固定为 0xFF（不透明）
    
    for (int y = 0; y < height; y++) {
        // BMP 的行索引（处理正/反存储）
        int src_y;
        if (is_top_down) {
            src_y = y;  // 从上到下
        } else {
            src_y = height - 1 - y;  // 从下到上（标准 BMP）
        }
        
        uint8_t* src_row = bmp_data + src_y * bmp_row_size;
        
        for (int x = 0; x < width; x++) {
            uint8_t b, g, r;
            
            if (info.bits_per_pixel == 24) {
                // 24-bit BMP: BGR 顺序
                b = src_row[x * 3 + 0];
                g = src_row[x * 3 + 1];
                r = src_row[x * 3 + 2];
            } else { // 32-bit
                // 32-bit BMP: BGRA 顺序
                b = src_row[x * 4 + 0];
                g = src_row[x * 4 + 1];
                r = src_row[x * 4 + 2];
                // alpha = src_row[x * 4 + 3];
            }
            
            // 写入 BGRA 格式（Alpha = 0xFF）
            fputc(b, fout);
            fputc(g, fout);
            fputc(r, fout);
            fputc(0xFF, fout);
        }
    }

    free(bmp_data);
    fclose(fout);
    
    printf("转换成功: %s (%d x %d)\n", argv[2], width, height);
    return 0;
}

/*
# 编译
gcc -o ./tools/bmp2fbmp ./tools/bmp2fbmp.c

# 转换
./tools/bmp2fbmp wallpaper_waves.bmp root/base/data/wallpaper/wallpaper_waves.fbmp
*/