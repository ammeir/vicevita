
#ifndef PSV_DEBUG_H
#define PSV_DEBUG_H

#include <stdio.h>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */

// Comment this line to turn debug off
//#define PSV_DEBUG_CODE

#ifdef PSV_DEBUG_CODE
static void PSV_DEBUG(const char* str, ...) {
	va_list list;
	char buf[512];

	va_start(list, str);
	vsprintf(buf, str, list);
	va_end(list);

	FILE *fp = fopen("ux0:data/vicevita/view.log", "a+");
	
	if (fp != NULL){
		fprintf(fp, buf);
		fprintf(fp, "\x0D\x0A"); // new line
		fclose(fp);
	}
}
#else
static void PSV_DEBUG(const char* str, ...) {}
#endif



#endif