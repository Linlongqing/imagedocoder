#include <opencv2/opencv.hpp>

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jpeglib.h"


int main()
{

    FILE *input_file;

    input_file=fopen("test.jpg","rb");


    struct jpeg_decompress_struct cinfo;//JPEG图像在解码过程中，使用jpeg_decompress_struct类型的结构体来表示，图像的所有信息都存储在结构体中
    struct jpeg_error_mgr jerr;//定义一个标准的错误结构体，一旦程序出现错误就会调用exit()函数退出进程



    cinfo.err = jpeg_std_error(&jerr);//绑定错误处理结构对象

    jpeg_create_decompress(&cinfo);//初始化cinfo结构
    jpeg_stdio_src(&cinfo,input_file);//指定解压缩数据源
    jpeg_read_header(&cinfo,TRUE);//获取文件信息
    jpeg_start_decompress(&cinfo);//开始解压缩

    unsigned long width = cinfo.output_width;//图像宽度
    unsigned long height = cinfo.output_height;//图像高度
    unsigned short depth = cinfo.output_components;//图像深度

    unsigned char *src_buff;//用于存取解码之后的位图数据(RGB格式)
    src_buff = (unsigned char *)malloc(width * height * depth);//分配位图数据空间
    memset(src_buff, 0, sizeof(unsigned char) * width * height * depth);//清0

    JSAMPARRAY buffer;//用于存取一行数据
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, width*depth, 1);//分配一行数据空间

    unsigned char *point = src_buff;
    while(cinfo.output_scanline < height)//逐行读取位图数据
    {
        jpeg_read_scanlines(&cinfo, buffer, 1);    //读取一行jpg图像数据到buffer
        memcpy(point, *buffer, width*depth);    //将buffer中的数据逐行给src_buff
        point += width * depth;            //指针偏移一行
    }

    cv::Mat img(height, width, CV_8UC3);
    memcpy(img.data, src_buff, height * width * sizeof(char) * 3);
    cv::imshow("image", img);
    cv::waitKey(0);
    jpeg_finish_decompress(&cinfo);//解压缩完毕
    jpeg_destroy_decompress(&cinfo);// 释放资源
    free(src_buff);//释放资源

    fclose(input_file);
}
#endif

#if 1
#include <stdio.h>
#include <stdlib.h>
#include "png.h"
#include <jpeglib.h>
#include "zlib.h"

typedef struct
{
    unsigned char* data;
    int size;
    int offset;
}ImageSource;
//从内存读取PNG图片的回调函数
static void pngReadCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
    ImageSource* isource = (ImageSource*)png_get_io_ptr(png_ptr);
    if(isource->offset + length <= isource->size)
    {
        memcpy(data, isource->data+isource->offset, length);
        isource->offset += length;
    }
    else
        png_error(png_ptr, "pngReaderCallback failed");
}

int decodejpeg(unsigned char* data, const unsigned int dataSize, cv::Mat& image)
{

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    //从内存读取
    jpeg_mem_src(&cinfo, data, dataSize);

    jpeg_read_header(&cinfo, TRUE);
    int height = cinfo.image_height;
    int width = cinfo.image_width;
    image = cv::Mat(height, width, CV_8UC3);

    jpeg_start_decompress(&cinfo);

    JSAMPROW row_pointer[1];

    while(cinfo.output_scanline < cinfo.output_height)
    {
        row_pointer[0] = &image.data[cinfo.output_scanline * cinfo.image_width * cinfo.num_components];
        jpeg_read_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);


    return 0;
}
//从内存读取
int loadFromStream(unsigned char* data, const unsigned int dataSize, cv::Mat& image)
{

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(png_ptr == 0) {
        return -1;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == 0)
    {
        png_destroy_read_struct(&png_ptr, 0, 0);
        return -1;
    }
    if(setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr,0);
    }
    ImageSource imgsource;
    imgsource.data = data;
    imgsource.size = dataSize;
    imgsource.offset = 0;
    png_set_read_fn(png_ptr, &imgsource, pngReadCallback);
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_STRIP_ALPHA, 0);
    int width = info_ptr->width;
    int height = info_ptr->height;
    int color_type = info_ptr->color_type;
    png_bytep* row_pointers = png_get_rows(png_ptr,info_ptr);

    int pos=0;
    if(color_type == PNG_COLOR_TYPE_GRAY)
    {
        image = cv::Mat(height, width, CV_8UC1);
        for(int i = 0; i < height; i++)
        {
            for(int j = 0; j < width; j+=1)
            {
                image.data[pos++] = row_pointers[i][j];
            }
        }
    }
    else
    {
        image = cv::Mat(height, width, CV_8UC3);
        for(int i=0;i<height;i++)
        {
            for(int j=0;j<3*width;j+=3)
            {
                image.data[pos++] = row_pointers[i][j+2];//BLUE
                image.data[pos++] = row_pointers[i][j+1];//GREEN
                image.data[pos++] = row_pointers[i][j];//RED
            }
        }
    }

    // free memory
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);

    return 0;
}

int main()
{
    FILE *fp;
//    if ((fp = fopen("test.png", "rb")) == NULL)
//    {
//        return EXIT_FAILURE;
//    }
//
//    uchar* buffer = new uchar[1920 * 1080];
//    int dataSize = fread(buffer, 1, 1920 * 1080, fp);
//    cv::Mat image;
//
//    loadFromStream(buffer, dataSize, image);
//    cv::imshow("lin", image);
//    cv::waitKey(0);
    if ((fp = fopen("test.jpg", "rb")) == NULL)
    {
        return EXIT_FAILURE;
    }

    uchar* buffer = new uchar[1920 * 1080];
    int dataSize = fread(buffer, 1, 1920 * 1080, fp);
    cv::Mat image;

    decodejpeg(buffer, dataSize, image);
    cv::imshow("lin", image);
    cv::waitKey(0);
}
#endif

#if 0
unsigned char* buffer = NULL;
png_uint_32 width, height, color_type;

//获取每一行所用的字节数，需要凑足4的倍数
int getRowBytes(int width){
    //刚好是4的倍数
    if((width * 3) % 4 == 0){
        return width * 3;
    }else{
        return ((width * 3) / 4 + 1) * 4;
    }
}

int main(int c, char** v) {
    png_structp png_ptr;
    png_infop info_ptr;
    int bit_depth;
    FILE *fp;

    printf("lpng[%s], zlib[%s]\n", PNG_LIBPNG_VER_STRING, ZLIB_VERSION);

    if ((fp = fopen("test.png", "rb")) == NULL) {
        return EXIT_FAILURE;
    }
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        fclose(fp);
        return EXIT_FAILURE;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return EXIT_FAILURE;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        /* Free all of the memory associated with the png_ptr and info_ptr */
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        /* If we get here, we had a problem reading the file */
        return EXIT_FAILURE;
    }
    /* Set up the input control if you are using standard C streams */
    png_init_io(png_ptr, fp);
    //读取png文件
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);
    //获取png图片相关信息
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, (int *)&color_type,
                 NULL, NULL, NULL);
    printf("width[%d], height[%d], bit_depth[%d], color_type[%d]\n",
           width, height, bit_depth, color_type);

    //获得所有png数据
    png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);
    //计算buffer大小
    unsigned int bufSize = 0;
    if (color_type == PNG_COLOR_TYPE_RGB) {
        bufSize = getRowBytes(width) * height;
    } else if (color_type == PNG_COLOR_TYPE_RGBA) {
        bufSize = width * height * 4;
    } else{
        return EXIT_FAILURE;
    }
    //申请堆空间
    buffer = (unsigned char*) malloc(bufSize);
    int i;
    for (i = 0; i < height; i++) {
        //拷贝每行的数据到buffer，
        //opengl原点在下方，拷贝时要倒置一下
        if(color_type == PNG_COLOR_TYPE_RGB){
            memcpy(buffer + getRowBytes(width) * i, row_pointers[i], width * 3);
        }else if(color_type == PNG_COLOR_TYPE_RGBA){
            memcpy(buffer + i * width * 4, row_pointers[height - i - 1], width * 4);
        }
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    fclose(fp);

    cv::Mat img(height, width, CV_8UC3);
    memcpy(img.data, buffer, height * width * sizeof(char) * 3);
    cv::imshow("image", img);
    cv::waitKey(0);
    return 0;
}
#endif