#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#include "img2vram.h"

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Error: Missing argument(s).\n");
        usage();

        exit(EXIT_FAILURE);
    } else if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage();
        return EXIT_SUCCESS;
    }

    int nImages = argc - 1;

    printf("static font[2] = {0x0F0F, 0x0F0F};\n\n");

    printf("static tiles[%d] = {", nImages * 32*12);
    for(int i = 1; i < argc; ++i) {
        int width = 0;
        int height = 0;
        unsigned char* data = stbi_load(argv[i], &width, &height, 0, 3);
        if(data == NULL) {
            fprintf(stderr, "Could not load file (%s)\n", stbi_failure_reason());
            exit(EXIT_FAILURE);
        }

        if(width != 32 || height != 24) {
            fprintf(stderr, "The input image \"%s\" is not 32*24.\n", argv[i]);
            exit(EXIT_FAILURE);
        }

        for(int y = 0; y < 12; ++y) {
            for(int x = 0; x < 32; ++x) {

                // We double the number of vertical pixels, totalling two pixels per character (foreground is top pixel ; background is bottom one)
                unsigned short pixel_up[] =   {data[y*2*3 * 32 + x*3],      data[y*2*3 * 32 + x*3 + 1],       (data[y*2*3 * 32 + x*3 + 2])      };
                unsigned short pixel_down[] = {data[(y*2 + 1)*3 * 32 + x*3], data[(y*2 + 1)*3 * 32 + x*3 + 1], (data[(y*2 + 1)*3 * 32 + x*3 + 2])};
                //printf("%d\n", data[y * 3 *32 + x*3] );

                int dist_up[16] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
                int dist_down[16] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
                int min_up = 0;
                int min_down = 0;
                for(int j = 0; j < 16; ++j) {
                    // As suggested by Mark Byers on Stack Overflow, to find the nearest color we need to use the euclidian distance
                    dist_up[j] =   sqrt((pixel_up[0]   - palette_rgb[j][0])*(pixel_up[0]   - palette_rgb[j][0]) + (pixel_up[1]   - palette_rgb[j][1])*(pixel_up[1]   - palette_rgb[j][1]) + (pixel_up[2]   - palette_rgb[j][2])*(pixel_up[2]   - palette_rgb[j][2]));
                    dist_down[j] = sqrt((pixel_down[0] - palette_rgb[j][0])*(pixel_down[0] - palette_rgb[j][0]) + (pixel_down[1] - palette_rgb[j][1])*(pixel_down[1] - palette_rgb[j][1]) + (pixel_down[2] - palette_rgb[j][2])*(pixel_down[2] - palette_rgb[j][2]));
                    if(dist_up[j] < dist_up[min_up])
                        min_up = j;
                    if(dist_down[j] < dist_down[min_down])
                        min_down = j;
                }
                //printf("Closest to (%d, %d, %d) is (%d, %d, %d) (%d)\n", pixel_up[0], pixel_up[1], pixel_up[2], palette_rgb[min_up][0], palette_rgb[min_up][1], palette_rgb[min_up][2], min_up);
                //printf("Closest to (%d, %d, %d) is (%d, %d, %d) (%d)\n", pixel_down[0], pixel_down[1], pixel_down[2], palette_rgb[min_down][0], palette_rgb[min_down][1], palette_rgb[min_down][2], min_down);

                if((y*32+x) % 8 == 0) {
                    printf("%s\n\t0x%hx", y == 0 && x == 0 && i == 0 ? "" : ",", (min_up << 12) | min_down << 8);
                } else {
                    printf(", 0x%hx", (min_up << 12) | min_down << 8);
                }
            }
        }

        stbi_image_free(data);
    }
    printf("\n};\n");

    return EXIT_SUCCESS;
}

void usage() {
    printf("Usage:\n\t./img2vram <file1> [file2] [file 3] [...]\n");
}
