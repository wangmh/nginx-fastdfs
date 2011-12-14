#include<stdio.h>
#include <stdlib.h>
#include<string.h>
#include <wand/MagickWand.h>
#include "fdfs_define.h"
#include "logger.h"
#include "shared_func.h"
#include "fdfs_global.h"
#include "sockopt.h"
#include "http_func.h"
#include "fdfs_http_shared.h"
#include "fdfs_client.h"
#include "local_ip_func.h"
#include "fdfs_thumbnail.h"
#define  replace_char_len  4
#define CROP_ITEM_COUNT 4

#define IMAGE_FOR_MAX(X, Y)  ((X) > ( Y ) ? (X):(Y))
#define IMAGE_FOR_MIN(X, Y)  ((X) < ( Y ) ? (X):(Y))

#define ThrowWandException(wand) \
{ \
  char \
    *description; \
 \
  ExceptionType \
    severity; \
 \
  description=MagickGetException(wand,&severity); \
  (void) logError("%s %s %lu %s\n",GetMagickModule(),description); \
  description=(char *) MagickRelinquishMemory(description); \
}

unsigned char *covert_image(MagickWand *magick_wand,
		img_transition_info *image_transition_info, size_t * thumbnail_size);

static int is_image_ext(char *filename, char **ext, int len);

typedef enum {
	GIFEXT, JPEGEXT, JPGEXT, PNGEXT, IMAGE_EXT_LEN
} IMG_TYPE;

char *image_ext[IMAGE_EXT_LEN] = { ".gif", ".jpeg", ".jpg", ".png" };

char *image_format[IMAGE_EXT_LEN] = { "GIF", "JPEG", "JPG", "PNG" };

static int is_image_ext(char *filename, char **ext, int len) {
	int i;
	int rc = 1;
	if (filename == NULL)
		return 0;
	if (strlen(filename) < 22)
		return 0;
	char *src = NULL;
	for (i = 0; i < len; i++) {
		src = filename + strlen(filename) - strlen(ext[i]);
		if (0 == (rc = strcasecmp(src, ext[i]))) {
			break;
		}
	}
	return rc == 0 ? 1 : 0;
}

/*
 static int is_gif(char *filename) {
 int rc = 1;
 if (filename == NULL)
 return 0;
 if (strlen(filename) < 22)
 return 0;
 char *src = NULL;
 src = filename + strlen(filename) - strlen(image_ext[GIFEXT]);
 rc = strcasecmp(src, image_ext[GIFEXT]);
 return rc == 0 ? 1 : 0;
 }
 */

char need_replace[replace_char_len] = { '(', ')', '_', '-' };
char dest_replace[replace_char_len] = { '<', '>', '^', '%' };

void replace_string(char str[], int len) {
	int i, j;
	for (j = 0; j < len; j++) {
		for (i = 0; i < replace_char_len; i++) {
			if (str[j] == need_replace[i]) {
				str[j] = dest_replace[i];
				break;
			}
		}
	}
	return;
}

/*
 *	group1/M00/0D/F3/3MIv8U0JrcoAAAAAAABNcGtWhC0409=320X100)!.jpg
 * 	return 320X100
 */
int filter_thumbnail(char *filename, char thumbnail_str[], int len) {
	int i;
	if (NULL == filename)
		return 0;
	if (!is_image_ext(filename, image_ext, IMAGE_EXT_LEN))
		return 0;
	memset(thumbnail_str, 0, len);
	char *thum_start = NULL;
	char *thum_end = NULL;
	if (NULL == (thum_start = strrchr(filename, '='))) {
		return 0;
	}
	thum_start++;
	//modify for .jpg.jpg
	if (NULL == (thum_end = strchr(thum_start, '.'))) {
		return 0;
	}
	if (thum_end - thum_start + 1 >= len)
		return 0;
	len = thum_end - thum_start + 1;
	snprintf(thumbnail_str, len, "%s", thum_start);
	int trim_str_len = strlen(thum_end);
	thum_start--;
	for (i = 0; i < trim_str_len; i++) {
		*(thum_start++) = *(thum_end++);
	}
	*thum_start = '\0';
	replace_string(thumbnail_str, len);
	return 1;
}

#if 0
unsigned char *get_thumbnail(char *full_filename, char *thumbnail_str,
		size_t *thumbnail_size, int is_rotate, int rotate_degree) {

	unsigned char *image_data = NULL;

	if (full_filename == NULL || thumbnail_str == NULL)
	return NULL;
	MagickBooleanType status;
	MagickWand *tmp_magick_wand = NULL;
	MagickWand *magick_wand = NULL;
	magick_wand = NewMagickWand();
	status = MagickReadImage(magick_wand, full_filename);
	if (status == MagickFalse) {
		ThrowWandException(magick_wand);
		return NULL;
	}
	PixelWand *background = NULL;

	size_t height = MagickGetImageHeight(magick_wand);
	size_t old_height = height;
	size_t width = MagickGetImageWidth(magick_wand);
	size_t old_width = width;

	ssize_t i = 0, j = 0;
	int is_crop = 0;
	char is_gif_flag = 0;
	char is_jpeg_flag = 0;
	int do_quality = 0;
	char *fileformat = NULL;

	fileformat = MagickGetImageFormat(magick_wand);
	if (fileformat == NULL) {
		return NULL;
	}

	if (0 == strcasecmp(fileformat, image_format[GIFEXT])) {
		is_gif_flag = 1;
	} else if (0 == strcasecmp(fileformat, image_format[JPEGEXT])) {
		is_jpeg_flag = 1;
	} else if (0 == strcasecmp(fileformat, image_format[JPGEXT])) {
		is_jpeg_flag = 1;
	}
	fileformat = (char *)MagickRelinquishMemory(fileformat); //free();
	if( 'C' == *thumbnail_str ||'c' == *thumbnail_str ) {
		is_crop = 1;
	}
	if(is_crop) {
		ParseMetaGeometry(thumbnail_str + 1, &i, &j, &width, &height);
	} else {
		ParseMetaGeometry(thumbnail_str, &i, &j, &width, &height);
	}

	if (old_width == width && height == old_height) {
		image_data = MagickGetImagesBlob(magick_wand, thumbnail_size);
	} else if (width <= 0 || height <= 0) {
		logError("%s%s:Geometry  %s error\n", __FILE__, __func__, thumbnail_str);
	} else {
		/*
		 * if type of the image is GIF, maybe have more than one frame, so do this different
		 *  from others
		 */
		if (is_gif_flag) {
			tmp_magick_wand = magick_wand;
			magick_wand = MagickCoalesceImages(tmp_magick_wand);
			tmp_magick_wand = DestroyMagickWand(tmp_magick_wand);
		}
		/*
		 * if size of the image less than 800 * 600 and that's type is JPEG, then do
		 * quality 100 OP
		 */
		if ((old_width < 800) && (old_height < 600) && is_jpeg_flag && is_crop != 1) {
			do_quality = 1;
		}

		MagickResetIterator(magick_wand);
		while (MagickNextImage(magick_wand) != MagickFalse) {
			if(do_quality) {
				MagickSetImageCompressionQuality(magick_wand, 100);
				MagickStripImage(magick_wand);
			}
			if(is_crop == 0)
			MagickThumbnailImage(magick_wand, width, height);
			else {
				logInfo("crop Image %ld, %ld", i, j);
				MagickCropImage(magick_wand, width, height, i, j);
			}
			if(is_rotate == 1) {
				background=NewPixelWand();
				status=PixelSetColor(background,"#000000");
				MagickRotateImage(magick_wand, background,(double)rotate_degree);
				background=DestroyPixelWand(background);
			}
		}
		image_data = MagickGetImagesBlob(magick_wand, thumbnail_size);
	}
	magick_wand = DestroyMagickWand(magick_wand);
	return image_data;
}
#endif

//200x100+100
int get_Crop_width_height(char *transition_str, size_t* cw, size_t *ch,
		size_t *x_offset, size_t *y_offset) {
	if (transition_str == NULL || cw == NULL || ch == NULL || x_offset == NULL
			|| y_offset == NULL)
		return 0;
	int j;
	*x_offset = 0;
	*y_offset = 0;
	*ch = 0;
	*cw = 0;
	char *str = strdup(transition_str);
	char delim[] = "x+ ";
	char *tmp = NULL;
	char *saveptr = NULL;
	char *tokens[CROP_ITEM_COUNT];
	char *token;
	for (j = 0, tmp = str;; j++, tmp = NULL) {
		token = strtok_r(tmp, delim, &saveptr);
		if (token == NULL || j >= CROP_ITEM_COUNT)
			break;
		tokens[j] = token;
	}
	if (j < 2 || j > 4) {
		free(str);
		return 0;
	}
	*cw = (size_t) atol(tokens[0]);
	*ch = (size_t) atol(tokens[1]);
	if (j == 3) {
		*x_offset = (size_t) atol(tokens[2]);
	}
	if (j >= 4) {
		*x_offset = (size_t) atol(tokens[2]);
		*y_offset = (size_t) atol(tokens[3]);
	}
	free(str);
	return 1;
}

static size_t get_pixels_by_rate(size_t a, size_t b, size_t c) {
	if (c == 0)
		return 0;
	if (a * b % c) {
		return a * b / c + 1;
	} else {
		return a * b / c;
	}
}

static void get_Crop_offset_and_wh(size_t cw, size_t ch, size_t *width,
		size_t *height, size_t *x_offset, size_t *y_offset) {
	size_t old_width = *width;
	size_t old_height = *height;
	size_t x = *x_offset;
	size_t y = *y_offset;
	if (cw * old_height >= ch * old_width) {
		*width = cw;
		*x_offset = 0;
		*height = get_pixels_by_rate(cw, old_height, old_width);
		if (0 == x && 0 == y) {
			*y_offset = 0;
		} else if (0 == x) {
			*y_offset = *height * (100 - y) / 100;
			*y_offset = *y_offset - IMAGE_FOR_MIN(*y_offset, ch);
		} else if (0 == y) {
			*y_offset = *height * x / 100;
			*y_offset = IMAGE_FOR_MIN(*y_offset, (*height - ch));
		} else {
			*y_offset = (*height - ch) * x / (x + y);
		}

	} else {
		*height = ch;
		*y_offset = 0;

		*width = get_pixels_by_rate(ch, old_width, old_height);
		if (0 == x && 0 == y) {
			*x_offset = 0;
		} else if (0 == x) {
			*x_offset = *width * (100 - y) / 100;
			*x_offset = *x_offset - IMAGE_FOR_MIN(*x_offset, cw);
		} else if (0 == y) {
			*x_offset = *width * x / 100;
			*x_offset = IMAGE_FOR_MIN(*x_offset, (*width - cw));
		} else {
			*x_offset = (*width - cw) * x / (x + y);
		}

	}

}

unsigned char *covert_image(MagickWand *magick_wand,
		img_transition_info *image_transition_info, size_t *thumbnail_size) {
	unsigned char *image_data = NULL;
	MagickBooleanType status;
	MagickWand *tmp_magick_wand = NULL;
	PixelWand *background = NULL;
	size_t height = MagickGetImageHeight(magick_wand);
	size_t old_height = height;
	size_t width = MagickGetImageWidth(magick_wand);
	size_t old_width = width;
	int is_crop = 0;
	int is_Crop = 0;
	int is_thumbnail = 0;
	ssize_t i = 0, j = 0;
	char is_gif_flag = 0;
	char is_jpeg_flag = 0;
	int do_quality = 0;
	char *fileformat = NULL;
	size_t cw = 0; //crop weight
	size_t ch = 0; //crop height

	size_t x_offset = 0;
	size_t y_offset = 0;

	fileformat = MagickGetImageFormat(magick_wand);
	if (fileformat == NULL) {
		return NULL;
	}

	if (0 == strcasecmp(fileformat, image_format[GIFEXT])) {
		is_gif_flag = 1;
	} else if (0 == strcasecmp(fileformat, image_format[JPEGEXT])) {
		is_jpeg_flag = 1;
	} else if (0 == strcasecmp(fileformat, image_format[JPGEXT])) {
		is_jpeg_flag = 1;
	}
	fileformat = (char *) MagickRelinquishMemory(fileformat); //free();

	if ('c' == image_transition_info->transition_str[0]) {
		is_crop = 1;
	} else if ('C' == image_transition_info->transition_str[0]) {
		is_Crop = 1;
	} else {
		is_thumbnail = 1;
	}
	if (is_crop) {
		ParseMetaGeometry(image_transition_info->transition_str + 1, &i, &j,
				&width, &height);
	} else if (is_thumbnail) {
		ParseMetaGeometry(image_transition_info->transition_str, &i, &j, &width,
				&height);
		if (old_width == width && height == old_height) //���ߴ���ͬ����������
			is_thumbnail = 0;
	} else if (is_Crop) {
		if (0
				>= get_Crop_width_height(
						image_transition_info->transition_str + 1, &cw, &ch,
						&x_offset, &y_offset)) {
			logError("%s%s:Crop  %s error\n", __FILE__, __func__,
					image_transition_info->transition_str + 1);

			return NULL;
		}
#if 0
		if(cw > width || ch > height) {
			image_data = MagickGetImagesBlob(magick_wand, thumbnail_size);
			magick_wand = DestroyMagickWand(magick_wand);
			return image_data;
		}
#endif
		//�õ�height��width,�����Ӧ��x_offset,yoffset;
		get_Crop_offset_and_wh(cw, ch, &width, &height, &x_offset, &y_offset);
	}

	if (old_width == width && height == old_height && (is_Crop == 0)
			&& (image_transition_info->is_rotate == 0)
			&& (image_transition_info->is_quality == 0)) {
		image_data = MagickGetImagesBlob(magick_wand, thumbnail_size);
	} else if (width <= 0 || height <= 0) {
		logError("%s%s:Geometry  %s error\n", __FILE__, __func__,
				image_transition_info->transition_str);
	} else {
		/*
		 * if type of the image is GIF, maybe have more than one frame, so do this different
		 *  from others
		 */
		if (is_gif_flag) {
			tmp_magick_wand = magick_wand;
			magick_wand = MagickCoalesceImages(tmp_magick_wand);
			tmp_magick_wand = magick_wand;
			magick_wand = MagickOptimizeImageLayers(tmp_magick_wand);
			tmp_magick_wand = DestroyMagickWand(tmp_magick_wand);
		}
		/*
		 * if size of the image less than 800 * 600 and that's type is JPEG, then do
		 * quality 100 OP
		 */
		if ((old_width < 800) && (old_height < 600) && is_jpeg_flag
				&& is_crop != 1 && (image_transition_info->is_quality == 0)) {
			do_quality = 1;
		}
		background = NewPixelWand();
		status = PixelSetColor(background, "#000000");

		MagickResetIterator(magick_wand);
		while (MagickNextImage(magick_wand) != MagickFalse) {
			if (do_quality) {
				MagickSetImageCompressionQuality(magick_wand, 100);
				MagickStripImage(magick_wand);
			}
			if (is_thumbnail == 1) {
				MagickThumbnailImage(magick_wand, width, height);
			} else if (is_crop == 1) {
				MagickCropImage(magick_wand, width, height, i, j);
			} else if (is_Crop == 1) {
				MagickThumbnailImage(magick_wand, width, height);
				MagickCropImage(magick_wand, cw, ch, x_offset, y_offset);
				if(is_gif_flag){// gif should thumbnail again
					MagickThumbnailImage(magick_wand, cw, ch);
				}
			}
			if (image_transition_info->is_rotate == 1) {
				MagickRotateImage(magick_wand, background,
						(double) (image_transition_info->degree));
			}
			if (image_transition_info->is_quality) {
				MagickSetImageCompressionQuality(magick_wand,
						image_transition_info->quality);
			}
			MagickStripImage(magick_wand);
		}
		background = DestroyPixelWand(background);
		image_data = MagickGetImagesBlob(magick_wand, thumbnail_size);

		if (is_gif_flag) {
			magick_wand = DestroyMagickWand(magick_wand);
		}
	}
	return image_data;
}

unsigned char * get_transition_image(char *full_filename,
		size_t * thumbnail_size, img_transition_info *image_transition_info) {
	unsigned char *image_data = NULL;

	if (full_filename == NULL)
		return NULL;
	if (image_transition_info == NULL)
		return NULL;
	if ((0 == image_transition_info->is_rotate)
			&& ('\0' == image_transition_info->transition_str[0])
			&& (0 == image_transition_info->is_quality))
		return NULL;
	MagickBooleanType status;
	MagickWand *magick_wand = NULL;
	magick_wand = NewMagickWand();
	status = MagickReadImage(magick_wand, full_filename);
	if (status == MagickFalse) {
		ThrowWandException(magick_wand);
		return NULL;
	}
	image_data = covert_image(magick_wand, image_transition_info,
			thumbnail_size);
	magick_wand = DestroyMagickWand(magick_wand);
	return image_data;
}

unsigned char *get_transition_image_blob(char *file_buf, int buf_size,
		size_t * thumbnail_size, img_transition_info *image_transition_info) {

	unsigned char *image_data = NULL;

	if (file_buf == NULL)
		return NULL;
	if (image_transition_info == NULL)
		return NULL;
	if ((0 == image_transition_info->is_rotate)
			&& ('\0' == image_transition_info->transition_str[0])
			&& (0 == image_transition_info->is_quality))
		return NULL;
	MagickBooleanType status;
	MagickWand *magick_wand = NULL;
	magick_wand = NewMagickWand();
	status = MagickReadImageBlob(magick_wand, file_buf, buf_size);
	if (status == MagickFalse) {
		ThrowWandException(magick_wand);
		return NULL;
	}
	image_data = covert_image(magick_wand, image_transition_info,
			thumbnail_size);
	magick_wand = DestroyMagickWand(magick_wand);
	return image_data;

}
