	#include <stdio.h>
  #include <wand/MagickWand.h>
  
  static void logError(const char *format,...)
  {
  	return;
  }
  
  #define ThrowWandException(wand) \
do{ \
  char \
    *description; \
 \
  ExceptionType \
    severity; \
 \
  description=MagickGetException(wand,&severity); \
  (void) fprintf(stderr, "%s %s %lu %s\n",GetMagickModule(),description); \
  description=(char *) MagickRelinquishMemory(description); \
}while(0);

  char *get_thumbnail(char *full_filename, char *thumbnail_str, size_t *thumbnail_size)
 {
	
	char *image_data = NULL;
	if(full_filename == NULL || thumbnail_str == NULL)
		return NULL;
	MagickBooleanType  status;
   	MagickWand  *magick_wand, *tmp_magick_wand;
	magick_wand=NewMagickWand();
	status=MagickReadImage(magick_wand, full_filename);
	if (status == MagickFalse)
  	  ThrowWandException(magick_wand);
	size_t height = MagickGetImageHeight(magick_wand);
	size_t width = MagickGetImageWidth(magick_wand);
	ssize_t i, j;
	ParseMetaGeometry(thumbnail_str, &i, &j, &width, &height);
	if(width <= 0 || height <= 0)
	{
		logError("%s%s:Geometry  %s error\n", __FILE__, __func__, thumbnail_str);
	}
	else
	{
		/*
		if(is_gif(full_filename))
		{
			  tmp_magick_wand = magick_wand;
			  magick_wand = MagickCoalesceImages(tmp_magick_wand);
			  tmp_magick_wand =   DestroyMagickWand(tmp_magick_wand);
		}
		*/	
		 MagickResetIterator(magick_wand);
   		 while (MagickNextImage(magick_wand) != MagickFalse)
     		 {
     		 	MagickThumbnailImage(magick_wand,height,width);
   		 }
		image_data =  MagickGetImagesBlob(magick_wand, thumbnail_size);
	}
	
	magick_wand=DestroyMagickWand(magick_wand);
	return image_data;
}



  
  int main(int argc,char **argv)
  {
  
  
    MagickBooleanType
      status;
  
    MagickWand
      *magick_wand;
  
  
    /*
      Read an image.
    */
    MagickWandGenesis();
    magick_wand=NewMagickWand();
    size_t len;
    char *data;
    
		data = get_thumbnail(argv[1], argv[2], &len);
		status = MagickReadImageBlob(magick_wand, data, len);  
		if (status == MagickFalse)
      ThrowWandException(magick_wand);
    status=MagickWriteImages(magick_wand,argv[3],MagickTrue);
    if (status == MagickFalse)
      ThrowWandException(magick_wand);
    magick_wand=DestroyMagickWand(magick_wand);
    MagickWandTerminus();
    return(0);
  }
  
