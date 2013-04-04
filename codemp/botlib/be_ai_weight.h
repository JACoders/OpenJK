
/*****************************************************************************
 * name:		be_ai_weight.h
 *
 * desc:		fuzzy weights
 *
 * $Archive: /source/code/botlib/be_ai_weight.h $
 * $Author: Mrelusive $ 
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

#define WT_BALANCE			1
#define MAX_WEIGHTS			128

//fuzzy seperator
typedef struct fuzzyseperator_s
{
	int index;
	int value;
	int type;
	float weight;
	float minweight;
	float maxweight;
	struct fuzzyseperator_s *child;
	struct fuzzyseperator_s *next;
} fuzzyseperator_t;

//fuzzy weight
typedef struct weight_s
{
	char *name;
	struct fuzzyseperator_s *firstseperator;
} weight_t;

//weight configuration
typedef struct weightconfig_s
{
	int numweights;
	weight_t weights[MAX_WEIGHTS];
	char		filename[MAX_QPATH];
} weightconfig_t;

//reads a weight configuration
weightconfig_t *ReadWeightConfig(char *filename);
//free a weight configuration
void FreeWeightConfig(weightconfig_t *config);
//writes a weight configuration, returns true if successfull
qboolean WriteWeightConfig(char *filename, weightconfig_t *config);
//find the fuzzy weight with the given name
int FindFuzzyWeight(weightconfig_t *wc, char *name);
//returns the fuzzy weight for the given inventory and weight
float FuzzyWeight(int *inventory, weightconfig_t *wc, int weightnum);
float FuzzyWeightUndecided(int *inventory, weightconfig_t *wc, int weightnum);
//scales the weight with the given name
void ScaleWeight(weightconfig_t *config, char *name, float scale);
//scale the balance range
void ScaleBalanceRange(weightconfig_t *config, float scale);
//evolves the weight configuration
void EvolveWeightConfig(weightconfig_t *config);
//interbreed the weight configurations and stores the interbreeded one in configout
void InterbreedWeightConfigs(weightconfig_t *config1, weightconfig_t *config2, weightconfig_t *configout);
//frees cached weight configurations
void BotShutdownWeights(void);
