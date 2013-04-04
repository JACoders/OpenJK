#ifndef __UI_SPLASH_H
#define __UI_SPLASH_H

void	SP_DisplayLogos(void);
void	SP_DoLicense(void);
void*	SP_LoadFile(const char* name);
void*	SP_LoadFileWithLanguage(const char *name);
char*	SP_GetLanguageExt();
void	SP_DrawTexture(void* pixels, float width, float height, float vShift);

#endif
