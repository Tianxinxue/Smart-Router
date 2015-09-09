#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    char src_fileName[255];
    char out_fileName[255];
    int src_width = 0 ,src_height = 0;
    int src_size = 0, out_size = 0, tem_size = 0;
    unsigned char * src_buf, *out_buf, *tem_buf;
    FILE* in_file, *out_file;
    unsigned char *Y, *U, *V;
    unsigned char * Y2, *U2, *V2;
    unsigned char * p=NULL;

    if(argc < 5)
    {
        printf("usage:%s <src_filename> <out_filename> <src_width> <src_height>\n",argv[0]);
        return -1;		
    }
    strcpy(src_fileName, argv[1]);
    strcpy(out_fileName, argv[2]);
    src_width = atoi(argv[3]);
    src_height = atoi(argv[4]);


    src_size = (src_width * src_height) << 1 ;//对于YUY2 4:2:2
    src_buf = (unsigned char *)malloc(src_size*sizeof(char));
    memset(src_buf, 0, src_size);


    tem_size = (src_width * src_height) << 1; ///对于YUV 4:2:2
    tem_buf = (unsigned char *)malloc(tem_size*sizeof(char));
    memset(tem_buf, 0, tem_size);

    out_size = src_width*src_height * 1.5;//对于YUV 4:2:0
    out_buf = (unsigned char *)malloc(out_size*sizeof(char));
    memset(out_buf, 0, out_size);

    in_file = fopen(src_fileName, "rb");
    if (!in_file)
    {
        printf("cannot open input file.");
        return 0;
    }

    out_file = fopen(out_fileName, "wb");
    if (!out_file)
    {
        printf("cannot write 264 file./n");
        return 0;
    }
    
   if (fread(src_buf, src_size, 1, in_file) <= 0)
   {
       printf("cannot read from input file\n");
       return -1;
   }
   p = src_buf;
  
   Y = tem_buf;
   U = Y + src_width*src_height;
   V = U + (src_width*src_height>>1);
  
   Y2 = out_buf;
   U2 = Y2 + src_width*src_height;
   V2 = U2 + (src_width*src_height>>2);
  
   //由打包YUYV变成平板YUV
    int k, j;
    for( k=0; k<src_height; ++k)
    { 
        for( j=0; j<(src_width>>1); ++j)
        {    
            Y[j*2] = p[4*j];    
            U[j] = p[4*j+1];
            Y[j*2+1] = p[4*j+2];
            V[j] = p[4*j+3];
        }
        p = p + src_width*2;
        Y = Y + src_width;
        U = U + (src_width>>1);
        V = V + (src_width>>1);
    }
  
   //复位
    Y = tem_buf;
    U = Y + src_width*src_height;
    V = U + (src_width*src_height>>1);
  
    int l;
    for( l=0; l<src_height/2; ++l)
    {
        memcpy(U2, U, src_width>>1);
        memcpy(V2, V, src_width>>1);
   
        U2 = U2 + (src_width>>1);
        V2 = V2 + (src_width>>1);
   
        U = U + (src_width);
        V = V + (src_width);
    }
  
    memcpy(Y2, Y, src_width*src_height);
  
    fwrite(out_buf, sizeof(char), out_size, out_file);
    printf("yuy2 to yuv420 ok!\n");
    fflush(out_file);

    free(src_buf); src_buf=NULL;
    free(tem_buf); tem_buf=NULL;
    free(out_buf); out_buf=NULL;

    fclose(in_file);
    fclose(out_file);

    return 0;
}
