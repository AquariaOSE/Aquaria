/* Conversion utility to hackpatch glf files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    float dx, dy;
    float tx1, ty1;
    float tx2, ty2;
} GLFontChar;

typedef struct
{
    unsigned int tex; /* tex id, unused */
    unsigned int tw; /* tex width */
    unsigned int th; /* tex height */
    unsigned int sc; /* start char number */
    unsigned int ec; /* end char number */
    unsigned int chrs; /* chars cound, unused*/
} GLFontHeader;

#define CHR_SIZE_KOEFF_X 8.8f/256.0f
#define CHR_SIZE_KOEFF_Y 10.0f/256.0f

int main(int argc, char **argv)
{
    unsigned int i, num_chars;
    GLFontChar cc;
    GLFontHeader h;
    FILE *in, *out;
    
    
    in = fopen("in.glf", "rb");
    out = fopen("out.glf", "wb");
    
    if(!(in && out))
        return 1;

    fread(&h, 1, sizeof(h), in);

    /* Fix it. */
    h.tw *= 2;
    h.th *= 2;
    
    fwrite(&h, 1, sizeof(h), out);

    num_chars = h.ec - h.sc + 1;
    

    for(i = 0; i < num_chars; ++i)
    {
        fread(&cc, 1, sizeof(cc), in);
        
        /* Fix it. */
        cc.dx *= CHR_SIZE_KOEFF_X;
        cc.dy *= CHR_SIZE_KOEFF_Y;
        
        fwrite(&cc, 1, sizeof(cc), out);
    }
    
    /* We don't write actual image data. */

    fclose(in);
    fclose(out);

    return 0;
}

