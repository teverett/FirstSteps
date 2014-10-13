
#include "video.h"

void main( void )
{
    char *str = "my first kernel";
	clear();
	showstring(str);

  for(;;); /* Keep the OS running */
}