
/*****************************************************************************
 * name:		be_ai_gen.c
 *
 * desc:		genetic selection
 *
 * $Archive: /MissionPack/code/botlib/be_ai_gen.c $
 * $Author: Zaphod $ 
 * $Revision: 3 $
 * $Modtime: 11/22/00 8:50a $
 * $Date: 11/22/00 8:55a $
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "../game/be_ai_gen.h"

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int GeneticSelection(int numranks, float *rankings)
{
	float sum, select;
	int i, index;

	sum = 0;
	for (i = 0; i < numranks; i++)
	{
		if (rankings[i] < 0) continue;
		sum += rankings[i];
	} //end for
	if (sum > 0)
	{
		//select a bot where the ones with the higest rankings have
		//the highest chance of being selected
		select = random() * sum;
		for (i = 0; i < numranks; i++)
		{
			if (rankings[i] < 0) continue;
			sum -= rankings[i];
			if (sum <= 0) return i;
		} //end for
	} //end if
	//select a bot randomly
	index = random() * numranks;
	for (i = 0; i < numranks; i++)
	{
		if (rankings[index] >= 0) return index;
		index = (index + 1) % numranks;
	} //end for
	return 0;
} //end of the function GeneticSelection
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child)
{
	float rankings[256], max;
	int i;

	if (numranks > 256)
	{
		botimport.Print(PRT_WARNING, "GeneticParentsAndChildSelection: too many bots\n");
		*parent1 = *parent2 = *child = 0;
		return qfalse;
	} //end if
	for (max = 0, i = 0; i < numranks; i++)
	{
		if (ranks[i] < 0) continue;
		max++;
	} //end for
	if (max < 3)
	{
		botimport.Print(PRT_WARNING, "GeneticParentsAndChildSelection: too few valid bots\n");
		*parent1 = *parent2 = *child = 0;
		return qfalse;
	} //end if
	Com_Memcpy(rankings, ranks, sizeof(float) * numranks);
	//select first parent
	*parent1 = GeneticSelection(numranks, rankings);
	rankings[*parent1] = -1;
	//select second parent
	*parent2 = GeneticSelection(numranks, rankings);
	rankings[*parent2] = -1;
	//reverse the rankings
	max = 0;
	for (i = 0; i < numranks; i++)
	{
		if (rankings[i] < 0) continue;
		if (rankings[i] > max) max = rankings[i];
	} //end for
	for (i = 0; i < numranks; i++)
	{
		if (rankings[i] < 0) continue;
		rankings[i] = max - rankings[i];
	} //end for
	//select child
	*child = GeneticSelection(numranks, rankings);
	return qtrue;
} //end of the function GeneticParentsAndChildSelection
