#include<stdio.h>
#include <stdlib.h>
#include<string.h>

#define  replace_char_len  3
#define Image_ext_len 4

static int is_image_ext(char *filename, char **ext, int len);
char *image_ext[Image_ext_len] = {".gif", ".jpeg", ".jpg", ".png"};
static int is_image_ext(char *filename, char **ext, int len)
{
	int i;
	int rc = 1;
	if(filename == NULL) return 0;
	if(strlen(filename) < 22) return 0;
	char *src = NULL;
	for(i = 0; i < len; i ++)
	{
		src = filename + strlen(filename) - strlen(ext[i]);
		if(0 == (rc = strcasecmp(src, ext[i])))
		{
			break;
		}	
	}
	return rc == 0 ? 1:0;
}


char need_replace[replace_char_len] = {'(', ')', '_'};
char dest_replace[replace_char_len] = {'<', '>', '^'};

void replace_string(char str[], int len)
{
	int i, j;
	char *p = NULL;
	for(j = 0; j < len; j++)
	{
		for(i = 0; i < replace_char_len; i++)
		{
			if( str[j] == need_replace[i])
			{
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
int get_thumbnail(char *filename, char thumbnail_str[], int len)
{
	int i;
	if(NULL == filename)
		return 0;
	if(!is_image_ext(filename, image_ext, Image_ext_len))
		return 0;
	memset(thumbnail_str, 0, len);
	char *thum_start= NULL;
	char *thum_end = NULL;
 	if (NULL== (thum_start =  strrchr(filename, '=')))
	{
		return 0;
	}
	thum_start ++;
	if(NULL == (thum_end = strrchr(thum_start, '.')))
	{
		return 0;
	}		
	thum_end;
	if( thum_end - thum_start + 1 >= len )
		return 0;
	len = thum_end - thum_start + 1;
	snprintf(thumbnail_str, len , "%s", thum_start);
	int trim_str_len = strlen(thum_end);
	thum_start --;
	for(i = 0 ; i < trim_str_len ; i ++)
	{
		*(thum_start ++) = *(thum_end++);
	}
	*thum_start = '\0';	
	replace_string(thumbnail_str, len);
	return 1;		
}



int main(int argc, char *argv[])
{
	if(is_image_ext(argv[1], image_ext, 4))
		printf("is image\n");
	else
		printf("not image\n");
	char str1[] = "group1/M00/0D/F3/3MIv8U0JrcoAAAAAAABNcGtWhC0409=320X100)!.jpg";
	char thumb[20];
	int ret = get_thumbnail(str1, thumb, 20);
	printf("%d, %s, %s\n", ret, thumb, str1);
	return 1;
}	