// ASEFile.h

enum 
{
	TK_ASE_COLON = TK_USERDEF,
	TK_ASE_OBRACE,
	TK_ASE_CBRACE,
	TK_ASE_ASTERISK,
	TK_MESH,
	TK_TIMEVALUE,
	TK_MESH_NUMVERTEX,
	TK_MESH_NUMFACES,
	TK_MESH_VERTEX_LIST,
	TK_MESH_VERTEX,
	TK_MESH_FACE_LIST,
	TK_MESH_FACE,
	TK_MESH_NUMTVERTEX,
	TK_MESH_TVERTLIST,
	TK_MESH_TVERT,
	TK_MESH_NUMTVFACES,
	TK_MESH_TFACELIST,
	TK_MESH_TFACE,
	TK_MESH_ANIMATION,
	TK_WIREFRAME_COLOR,
	TK_CONTROL_BEZIER_SCALE_KEY,
	TK_CONTROL_SCALE_BEZIER,
	TK_CONTROL_ROT_TCB,
	TK_CONTROL_TCB_ROT_KEY,
	TK_CONTROL_POS_BEZIER,
	TK_CONTROL_BEZIER_POS_KEY,
	TK_NODE_NAME,
	TK_TM_ANIMATION,
	TK_PROP_RECVSHADOW,
	TK_PROP_CASTSHADOW,
	TK_PROP_MOTIONBLUR,
	TK_GEOMOBJECT,
	TK_NODE_PARENT,
	TK_NODE_TM,
	TK_INHERIT_POS,
	TK_INHERIT_ROT,
	TK_INHERIT_SCL,
	TK_TM_ROW0,
	TK_TM_ROW1,
	TK_TM_ROW2,
	TK_TM_ROW3,
	TK_TM_POS,
	TK_TM_ROTAXIS,
	TK_TM_ROTANGLE,
	TK_TM_SCALE,
	TK_TM_SCALEAXIS,
	TK_TM_SCALEAXISANG,
	TK_3DSMAX_ASCIIEXPORT,
	TK_ASE_COMMENT,
	TK_SCENE,
	TK_SCENE_FILENAME,
	TK_SCENE_FIRSTFRAME,
	TK_SCENE_LASTFRAME,
	TK_SCENE_FRAMESPEED,
	TK_SCENE_TICKSPERFRAME,
	TK_SCENE_BACKGROUND_STATIC,
	TK_SCENE_AMBIENT_STATIC,
	TK_MATERIAL_LIST,
	TK_MATERIAL_COUNT,
	TK_MATERIAL,
	TK_MATERIAL_NAME,
	TK_MATERIAL_CLASS,
	TK_MATERIAL_AMBIENT,
	TK_MATERIAL_DIFFUSE,
	TK_MATREIAL_SPECULAR,
	TK_MATERIAL_SHINE,
	TK_MATERIAL_SHINESTRENGTH,
	TK_MATERIAL_TRANSPARENCY,
	TK_MATERIAL_WIRESIZE,
	TK_MATERIAL_SHADING,
	TK_MATERIAL_XP_FALLOFF,
	TK_MATERIAL_SELFILLUM,
	TK_MATERIAL_FALLOFF,
	TK_MATERIAL_SOFTEN,
	TK_MATERIAL_XP_TYPE,
	TK_MAP_DIFFUSE,
	TK_MAP_NAME,
	TK_MAP_CLASS,
	TK_MAP_SUBNO,
	TK_MAP_AMOUNT,
	TK_BITMAP,
	TK_MAP_TYPE,
	TK_UVW_U_OFFSET,
	TK_UVW_V_OFFSET,
	TK_UVW_U_TILING,
	TK_UVW_V_TILING,
	TK_UVW_ANGLE,
	TK_UVW_BLUR,
	TK_UVW_BLUR_OFFSET,
	TK_UVW_NOUSE_AMT,
	TK_UVW_NOISE_SIZE,
	TK_UVW_NOISE_LEVEL,
	TK_UVW_NOISE_PHASE,
	TK_BITMAP_FILTER,
	TK_MESH_SMOOTHING,	
	TK_MESH_MTLID,
	TK_LABEL_A,
	TK_LABEL_B,
	TK_LABEL_C,
	TK_LABEL_AB,
	TK_LABEL_BC,
	TK_LABEL_CA,
};

class CASEFile 
{
public:
	CASEFile();
	~CASEFile();
	static CASEFile* Create(LPCTSTR filename);
	void Delete();

	void Parse();

protected:
	void Init(LPCTSTR filename);

	void ParseAsciiExport(CTokenizer* tokenizer);
	void ParseComment(CTokenizer* tokenizer);
	void ParseMaterialList(CTokenizer* tokenizer);
	void ParseScene(CTokenizer* tokenizer);
	void ParseGeomObject(CTokenizer* tokenizer);

	char*			m_file;
	int				m_export;
	char*			m_comment;

	static keywordArray_t	s_symbols[];
	static keywordArray_t	s_keywords[];
	static keywordArray_t	s_sceneKeywords[];
	static keywordArray_t   s_materialKeywords[];
	static keywordArray_t	s_mapKeywords[];
	static keywordArray_t	s_objectKeywords[];
	static keywordArray_t	s_nodeKeywords[];
	static keywordArray_t	s_meshKeywords[];
	static keywordArray_t	s_vertexKeywords[];
	static keywordArray_t	s_faceKeywords[];
	static keywordArray_t	s_faceOptionKeywords[];
	static keywordArray_t	s_tvertKeywords[];
	static keywordArray_t	s_tfaceKeywords[];
	static keywordArray_t	s_animationKeywords[];
	static keywordArray_t	s_contorlPosBezierKeywords[];
	static keywordArray_t	s_controlRotTCBKeywords[];
	static keywordArray_t	s_controlScaleBezierKeywords[];
	static keywordArray_t	s_labelKeywords[];
};
