#ifdef __cplusplus
extern "C"
{
#endif

int XSI_LoadFile(const char *filename);
int XSI_FindBone(char *bonename);
int XSI_GetNumBones();
void XSI_GetBoneName(int bone,char *bonename);
void XSI_GetBoneMatrix(int bone,int frame,float mat[3][4]);
int XSI_GetNumFrames();

void XSI_Cleanup();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
void XSI_GetBoneMatrix4(int bone,int frame,Matrix4 &m);
#endif




