#ifndef FDFS_THUMBNAIL_H_
#define FDFS_THUMBNAIL_H_



typedef struct _img_transition_info
{
    char transition_str[50];
    int is_rotate;
    int degree;
    int quality;
    int is_quality;
}img_transition_info;

int filter_thumbnail(char * filename, char thumbnail_str [ ], int len);

unsigned char * get_transition_image(char *full_filename,  size_t * thumbnail_size,
        img_transition_info *image_transition_info);

#endif /*FDFS_THUMBNAIL_H_*/
