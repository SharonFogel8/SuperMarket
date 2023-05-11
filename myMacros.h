#pragma once

#define CHECK_RETURN_0(ptr) {if(!ptr) return 0;}
#define CHECK_MSG_RETURN_0(ptr,msg) { if(!ptr){ printf("%s",msg); return 0;}}


#define	FREE_CLOSE_FILE_RETURN_0(ptr,fp) {\
free(ptr);\
fclose(fp); \
return 0;}

#define CLOSE_RETURN_0(fp) fclose(fp); return 0;

