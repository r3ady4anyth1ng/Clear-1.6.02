/*
//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+
*/

//
// Description: 

// VRML2.0 translator for Maya,  Animation-related functions
//
//

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#if defined (SGI) || defined (LINUX)
#include <X11/Intrinsic.h>
#endif

#include "vrml2Export.h"

#ifdef MAYA_SRC
#include <MDt.h>
#include <MDtExt.h>

#ifndef MAYA101
#include <maya/MComputation.h>
#endif

#else

#include <Dt.h>
#include <DtOM.h>

#endif

#ifdef WIN32
#pragma warning(disable: 4244)
#endif // WIN32

extern int      VR_PrintMessage (const char *,...);
extern int      VR_SetMessage (const char *,...);
extern int      VR_SetMessageLine (int LLineNum, const char *LMessage,...);

extern void     VR_DecomposeMatrix (const float *LMatrix, 
									DECOMP_MAT * LDMatrix, int LRotOrder);
extern unsigned int VR_StrCpyL (register char *LStr0, register char *LStr1);
extern void     VR_FilterName (register char *LStr);

extern int      VR_Precision;
extern Boolean  VR_SceneInited,
                VR_AutoUpdate;
extern Boolean  VR_LoopAnim,
                VR_AnimTrans,
                VR_AnimMaterials,
                VR_AnimLights,
                VR_AnimVertices,
				VR_AnimCameras,
                VR_LongLines;
extern Boolean  VR_ConversionRun;
extern double   VR_FramesPerSec;

extern int		VR_KeyCurves;

Boolean         VR_MetaCycleValid;

int             MC_MinNode,
                MC_MaxNode,
                MC_MaxSnippets,
                MC_MaxArcs,
                MC_NumberOfCharacters;
int             MC_MaxArcConns;
int             MC_ActiveRun; 			//0 - wait 1 - snippet 2 - arc
int             MC_ActiveRunId; 		//|___ > Assoc.Id.
int             MCyArcActivation;

MCyMainTable   *MC_MainTablePtr;
MCySnippet     *MC_SnippetPtr;
MCyArc         *MC_ArcPtr;

extern float MToVrml2_mapFloatRange( double zeroToInfinity );


// Note:
// 
// Metacycle is not Implemented in Maya
//
// This code is here as a carry over from the PowerAnimator translator
//

/* ====================================================== */
/* MetaCycle : Copy BlindData to Internal Buffer	 */
/* ====================================================== */

void 
VR_MCCopyDataXToTable (char *LSrc, char *LDst)
{
	register unsigned int LC;

	if (LSrc == NULL)
		LDst[0] = '\0';
	else
	{
		for (LC = 0; LC < MAXLENGTHOFBLINDDATA; LC++)
		{
			if ((LDst[LC] = LSrc[LC]) == '\0')
				LC = MAXLENGTHOFBLINDDATA;
		}
	}
	LDst[MAXLENGTHOFBLINDDATA] = '\0';
}

#define MC_DEBUG	1

/* ============================== */
/* Get MetaCycle data		 */
/* ============================== */
int 
VR_InitMetaCycle ()
{
	register int LC,
	                LC1 = 0,
	                LC2,
	                LC3;
	int			    LNumOfCharacters;
	int             LNumOfSnippets,
	                LNumOfArcs,
	                LCharacterFirstFrame,
	                LCharacterFirstSnippet,
	                LCharacterFirstArc;
	register int    LMinCNode = 0,
	                LMaxCNode = 0,
	                LActArcConn = 0;
	DtStateTable   *LTable = NULL;
	DtStateTable  **LTablePtrs = NULL;

	VR_MetaCycleValid = false;
	DtCnetCharacterCount (&LNumOfCharacters);
	VR_PrintMessage ("\n\n\n\n\n\n\n");

	MC_MinNode = 99999;
	MC_MaxNode = 0;
	MC_MaxArcConns = 0;
	MC_NumberOfCharacters = LNumOfCharacters;
	if (LNumOfCharacters > 0)
	{
		VR_PrintMessage ("MC characters: %d\n", LNumOfCharacters);
		if ((MC_MainTablePtr = (MCyMainTable *) malloc ((unsigned int) ((sizeof (MCyMainTable)) * (LNumOfCharacters + 1)))) == NULL)
			return (0);
		if ((LTablePtrs = (DtStateTable **) malloc ((unsigned int) ((sizeof (DtStateTable *)) * (LNumOfCharacters + 1)))) == NULL)
			return (0);
		VR_PrintMessage ("Table ptr. allocations Ok. (%x %x)\nCollecting characters : ", MC_MainTablePtr, LTablePtrs);
		fflush (stdout);

		LNumOfSnippets = 0;
		LNumOfArcs = 0;
		LCharacterFirstFrame = 0;

		//Collect how many nodes and arcs we have and init table

		for (LC = 0; LC < LNumOfCharacters; LC++)
		{
#if MC_DEBUG
			VR_PrintMessage ("%d", LC);
			fflush (stdout);
#endif
			LTable = NULL;
			DtCnetGetStateTable (LC, &LTable);
#if MC_DEBUG
			VR_PrintMessage ("(%x -> %x)", LTable, &LTablePtrs[LC]);
#endif
			if (LTable != NULL)
			{
				LTablePtrs[LC] = LTable;
#if MC_DEBUG
				VR_PrintMessage ("<%d:%d>", LTable->nodes, LTable->arcs);
#endif
				LNumOfSnippets += (LTable->nodes);
#if MC_DEBUG
				VR_PrintMessage ("{%d:", LNumOfSnippets);
				fflush (stdout);
#endif
				LNumOfArcs += (LTable->arcs);
#if MC_DEBUG
				VR_PrintMessage ("%d}", LNumOfArcs);
#endif
			} else
				LTablePtrs[LC] = (DtStateTable *) NULL;
#if MC_DEBUG
			VR_PrintMessage ("...Ok (%d = %x : %x)", LC, LTablePtrs[LC], &LTablePtrs[LC]);
#endif
		}

		if ((LTable->arcs <= 0) && (LTable->nodes <= 0))
			return (0);

#if MC_DEBUG
		VR_PrintMessage ("  Allocating space for %d snippets and %d arcs...", LNumOfSnippets, LNumOfArcs);
#endif
		if ((MC_SnippetPtr = (MCySnippet *) malloc ((unsigned int) (sizeof (MCySnippet) * LNumOfSnippets))) == NULL)
			return (0);
		if ((MC_ArcPtr = (MCyArc *) malloc ((unsigned int) (sizeof (MCyArc) * LNumOfArcs))) == NULL)
			return (0);
		MC_MaxSnippets = LNumOfSnippets;
		MC_MaxArcs = LNumOfArcs;
#if MC_DEBUG
		VR_PrintMessage ("Ok.");
#endif

		LNumOfSnippets = 0;
		LNumOfArcs = 0; //initialize running - counter

			for (LC = 0; LC < LNumOfCharacters; LC++)
		{
			LCharacterFirstSnippet = LNumOfSnippets;
			LCharacterFirstArc = LNumOfArcs;
			LTable = LTablePtrs[LC];
			if (LTable)
			{
				LMinCNode = 999999;
				LMaxCNode = 0; //Collectting min / max frame numbers
					if (LTable->nodes > 0)
				{
					for (LC1 = 0; LC1 < LTable->nodes; LC1++)
					{
						if (LTable->nodeData[LC1].storage < LMinCNode)
							LMinCNode = LTable->nodeData[LC1].storage;
						if ((LTable->nodeData[LC1].storage 
								+ LTable->nodeData[LC1].length) > LMaxCNode)
							LMaxCNode = LTable->nodeData[LC1].storage 
										+ LTable->nodeData[LC1].length;
					}
				}
				if (LTable->arcs > 0)
				{
					for (LC1 = 0; LC1 < LTable->arcs; LC1++)
					{
						if (LTable->arcData[LC1].storage < LMinCNode)
							LMinCNode = LTable->arcData[LC1].storage;
						if ((LTable->arcData[LC1].storage 
							+ LTable->arcData[LC1].length) > LMaxCNode)
							LMaxCNode = LTable->arcData[LC1].storage 
										+ LTable->arcData[LC1].length;
						if (LTable->arcData[LC1].entryFrame < LMinCNode)
							LMinCNode = LTable->arcData[LC1].entryFrame;
						if (LTable->arcData[LC1].entryFrame > LMaxCNode)
							LMaxCNode = LTable->arcData[LC1].entryFrame;
					}
				}
#if MC_DEBUG
				VR_PrintMessage ("\nMC : TableInfo for No %d :", LC);
				VR_PrintMessage ("\n	NofNodes: %d", LTable->nodes);
				VR_PrintMessage ("\n	NofArcs	: %d", LTable->arcs);
				VR_PrintMessage ("\n	Data1	: '%s'", LTable->data1);
				VR_PrintMessage ("\n	Data2	: '%s'", LTable->data2);
				VR_PrintMessage ("\n	Data3	: '%s'", LTable->data3);
				VR_PrintMessage ("\n	Data4	: '%s'", LTable->data4);
				VR_PrintMessage ("\n	LMinCNode: %d", LMinCNode);
				VR_PrintMessage ("\n	LMaxCNode: %d", LMaxCNode);
				VR_PrintMessage ("\n");
#endif

				MC_MainTablePtr[LC].StartEntryOfSnippets = LCharacterFirstSnippet;
				MC_MainTablePtr[LC].EndEntryOfSnippets = LCharacterFirstSnippet - 1 + LTable->nodes;
				MC_MainTablePtr[LC].StartEntryOfArcs = LCharacterFirstArc;
				MC_MainTablePtr[LC].EndEntryOfArcs = LCharacterFirstArc - 1 + LTable->arcs;
				VR_MCCopyDataXToTable (LTable->data1, MC_MainTablePtr[LC].BlindData[0]);
				VR_MCCopyDataXToTable (LTable->data2, MC_MainTablePtr[LC].BlindData[1]);
				VR_MCCopyDataXToTable (LTable->data3, MC_MainTablePtr[LC].BlindData[2]);
				VR_MCCopyDataXToTable (LTable->data4, MC_MainTablePtr[LC].BlindData[3]);

#if MC_DEBUG
				VR_PrintMessage ("\n  MainTable Init Ok.");
#endif

				//Filling out the appropriate table - entries                

				if (LTable->nodes > 0)
				{
					LC3 = LTable->arcs;

#if MC_DEBUG
					VR_PrintMessage ("\n  SnippetEntries (%d %d): ", LTable->nodes, LC3);
#endif

					for (LC1 = 0; LC1 < LTable->nodes; LC1++)
					{

#if MC_DEBUG
						VR_PrintMessage ("%d : Main...", LNumOfSnippets);
#endif
						MC_SnippetPtr[LNumOfSnippets].StartFrame = LTable->nodeData[LC1].storage - LMinCNode + LCharacterFirstFrame;
						MC_SnippetPtr[LNumOfSnippets].EndFrame = MC_SnippetPtr[LNumOfSnippets].StartFrame + LTable->nodeData[LC1].length - 1;
						VR_MCCopyDataXToTable (LTable->nodeData[LC1].data1, MC_SnippetPtr[LNumOfSnippets].BlindData[0]);
						VR_MCCopyDataXToTable (LTable->nodeData[LC1].data2, MC_SnippetPtr[LNumOfSnippets].BlindData[1]);
						VR_MCCopyDataXToTable (LTable->nodeData[LC1].data3, MC_SnippetPtr[LNumOfSnippets].BlindData[2]);
						VR_MCCopyDataXToTable (LTable->nodeData[LC1].data4, MC_SnippetPtr[LNumOfSnippets].BlindData[3]);
#if MC_DEBUG
						VR_PrintMessage ("Ok  Links:");
#endif
						if (LC3 > 0)
							MC_SnippetPtr[LNumOfSnippets].LinkData = (int *) malloc ((unsigned int) (sizeof (int) * (LC3 + 1)));
						else
							MC_SnippetPtr[LNumOfSnippets].LinkData = (int *) NULL;
						if (MC_SnippetPtr[LNumOfSnippets].LinkData != NULL)
						{
							LActArcConn = 0;
							for (LC2 = 0; LC2 < LC3; LC2++)
							{
#if MC_DEBUG
								VR_PrintMessage ("%d,", LC2);
#endif
								if (LTable->adjacency[(LC1 * LC3) + LC2] < 0)
									MC_SnippetPtr[LNumOfSnippets].LinkData[LC2] = (int) -1;
								else
								{
								   //Relative to MC_MainTablePtr[LC].StartEntryOfArcs
									MC_SnippetPtr[LNumOfSnippets].LinkData[LC2] = (int) (LTable->adjacency[(LC1 * LC3) + LC2] - LMinCNode);
									++ LActArcConn;
								}
							}
							//EOTableMark

							MC_SnippetPtr[LNumOfSnippets].LinkData[LC3] = (int) -2;
						}
#if MC_DEBUG
						VR_PrintMessage ("L)");
#endif

						if (MC_MaxArcConns < LActArcConn)
							MC_MaxArcConns = LActArcConn;
						++LNumOfSnippets;
#if MC_DEBUG
						VR_PrintMessage ("\n-- Snipet #%d --", LC1);
						VR_PrintMessage ("\n  Flags   : %d", LTable->nodeData[LC1].flags);
						VR_PrintMessage ("\n  Storage : %d (%d)", LTable->nodeData[LC1].storage, (LTable->nodeData[LC1].storage - LMinCNode));
						VR_PrintMessage ("\n  Length  : %d", LTable->nodeData[LC1].length);
						VR_PrintMessage ("\n  Data1   : '%s'", LTable->nodeData[LC1].data1);
						VR_PrintMessage ("\n  Data2   : '%s'", LTable->nodeData[LC1].data2);
						VR_PrintMessage ("\n  Data3   : '%s'", LTable->nodeData[LC1].data3);
						VR_PrintMessage ("\n  Data4   : '%s'", LTable->nodeData[LC1].data4);
#endif
					}
				}
				if (LTable->arcs > 0)
				{
					for (LC1 = 0; LC1 < LTable->arcs; LC1++)
					{
						MC_ArcPtr[LNumOfArcs].StartFrame = LTable->arcData[LC1].storage - LMinCNode + LCharacterFirstFrame;
						MC_ArcPtr[LNumOfArcs].EndFrame = MC_ArcPtr[LNumOfArcs].StartFrame + LTable->arcData[LC1].length - 1;
						MC_ArcPtr[LNumOfArcs].NextSnippet = LTable->arcData[LC1].nextNode + LCharacterFirstSnippet;
						MC_ArcPtr[LNumOfArcs].NextSnippetEntryFrame = LTable->arcData[LC1].entryFrame - LMinCNode + LCharacterFirstFrame;
						VR_MCCopyDataXToTable (LTable->arcData[LC1].data1, MC_ArcPtr[LNumOfArcs].BlindData[0]);
						VR_MCCopyDataXToTable (LTable->arcData[LC1].data2, MC_ArcPtr[LNumOfArcs].BlindData[1]);
						VR_MCCopyDataXToTable (LTable->arcData[LC1].data3, MC_ArcPtr[LNumOfArcs].BlindData[2]);
						VR_MCCopyDataXToTable (LTable->arcData[LC1].data4, MC_ArcPtr[LNumOfArcs].BlindData[3]);
						++LNumOfArcs;
#if MC_DEBUG
						VR_PrintMessage ("\n-- Arc #%d --", LC1);
						VR_PrintMessage ("\n  Storage : %d (%d)", LTable->arcData[LC1].storage, (LTable->arcData[LC1].storage - LMinCNode));
						VR_PrintMessage ("\n  Length  : %d", LTable->arcData[LC1].length);
						VR_PrintMessage ("\n  NextNode: %d", LTable->arcData[LC1].nextNode);
						VR_PrintMessage ("\n  EntryFrm: %d (%d)", LTable->arcData[LC1].entryFrame, (LTable->arcData[LC1].entryFrame - LMinCNode));
						VR_PrintMessage ("\n  Data1   : '%s'", LTable->arcData[LC1].data1);
						VR_PrintMessage ("\n  Data2   : '%s'", LTable->arcData[LC1].data2);
						VR_PrintMessage ("\n  Data3   : '%s'", LTable->arcData[LC1].data3);
						VR_PrintMessage ("\n  Data4   : '%s'", LTable->arcData[LC1].data4);
#endif
					}
				}
#if MC_DEBUG
				if ((LTable->nodes > 0) || (LTable->arcs > 0))
				{
					for (LC1 = 0; LC1 < LTable->nodes; LC1++)
					{
						VR_PrintMessage ("\n%d node : ", LC1);
						for (LC2 = 0; LC2 < LTable->arcs; LC2++)
						{
							if (LTable->adjacency[(LC1 * LTable->arcs) + LC2] < 0)
								VR_PrintMessage ("----, ");
							else
								VR_PrintMessage ("%4d, ", LTable->adjacency[(LC1 * LTable->arcs) + LC2]);
						}
					}
					VR_PrintMessage ("\n");
					for (LC1 = 0; LC1 < LTable->nodes; LC1++)
					{
						VR_PrintMessage ("\n%d node : ", LC1);
						for (LC2 = 0; LC2 < LTable->arcs; LC2++)
						{
							if (LTable->adjacency[(LC1 * LTable->arcs) + LC2] < 0)
								VR_PrintMessage ("--, ");
							else
								VR_PrintMessage ("%2d, ", LTable->adjacency[(LC1 * LTable->arcs) + LC2] - LMinCNode);
						}
					}
				} else
				{
					VR_PrintMessage ("\nMC : No valid table-entries for the adjacency table for Character #%d.", LC);
				}
#endif
				if (LTable->data1)
					free (LTable->data1);
				if (LTable->data2)
					free (LTable->data2);
				if (LTable->data3)
					free (LTable->data3);
				if (LTable->data4)
					free (LTable->data4);
				if (LTable->arcData)
					free (LTable->arcData);
				if (LTable->nodeData)
					free (LTable->nodeData);
				if (LTable->adjacency)
					free (LTable->adjacency);
				free (LTable);
			} //if (LTable)
				LCharacterFirstFrame += (LMaxCNode - LMinCNode);
			if (MC_MinNode > LMinCNode)
				MC_MinNode = LMinCNode;
			if (MC_MaxNode < LMaxCNode)
				MC_MaxNode = LMaxCNode;
		} //characters 'for' loop
	} //NoCharacters > 0
		else
	{
#if MC_DEBUG
		VR_PrintMessage ("\nMC : No Characters...");
#endif
		return (0);
	}
	VR_MetaCycleValid = true;
	if (LTablePtrs != NULL)
		free (LTablePtrs);

	DtFrameSetStart (MC_MinNode);
	DtFrameSetEnd (MC_MaxNode);

#if MC_DEBUG

	VR_PrintMessage ("\n\n*** METACYCLE SUMMARY ***\n");
	VR_PrintMessage ("\nNumber of characters : %d", MC_NumberOfCharacters);
	VR_PrintMessage ("\nAnimation Frames : %d - %d", MC_MinNode, MC_MaxNode);

	for (LC = 0; LC < MC_NumberOfCharacters; LC++)
	{
		VR_PrintMessage ("\n MainTableInfo for No %d :", LC1);
		VR_PrintMessage ("\n Snippet Start : %d", 
							MC_MainTablePtr[LC].StartEntryOfSnippets);
		VR_PrintMessage ("\n         Last  : %d", 
							MC_MainTablePtr[LC].EndEntryOfSnippets);
		VR_PrintMessage ("\n Arc     Start : %d", 
							MC_MainTablePtr[LC].StartEntryOfArcs);
		VR_PrintMessage ("\n         Last  : %d", 
							MC_MainTablePtr[LC].EndEntryOfArcs);
		VR_PrintMessage ("\n Data1         : '%s'", 
							MC_MainTablePtr[LC].BlindData[0]);
		VR_PrintMessage ("\n Data2         : '%s'", 
							MC_MainTablePtr[LC].BlindData[1]);
		VR_PrintMessage ("\n Data3         : '%s'", 
							MC_MainTablePtr[LC].BlindData[2]);
		VR_PrintMessage ("\n Data4         : '%s'", 
							MC_MainTablePtr[LC].BlindData[3]);
		VR_PrintMessage ("\n");

		for (LC1 = MC_MainTablePtr[LC].StartEntryOfSnippets; 
					LC1 <= MC_MainTablePtr[LC].EndEntryOfSnippets; LC1++)
		{
			VR_PrintMessage ("\n  --- Snippet #%d ---", LC1);
			VR_PrintMessage ("\n   StartFrame : %d", 
									MC_SnippetPtr[LC1].StartFrame);
			VR_PrintMessage ("\n     EndFrame : %d", 
									MC_SnippetPtr[LC1].EndFrame);
			VR_PrintMessage ("\n    Data1     : '%s'", 
									MC_SnippetPtr[LC1].BlindData[0]);
			VR_PrintMessage ("\n    Data2     : '%s'", 
									MC_SnippetPtr[LC1].BlindData[1]);
			VR_PrintMessage ("\n    Data3     : '%s'", 
									MC_SnippetPtr[LC1].BlindData[2]);
			VR_PrintMessage ("\n    Data4     : '%s'", 
									MC_SnippetPtr[LC1].BlindData[3]);
			VR_PrintMessage ("\n   Entries    :");

			if (MC_SnippetPtr[LC1].LinkData != NULL)
			{
				LC2 = 0;
				do
				{
					VR_PrintMessage ("%3d", MC_SnippetPtr[LC1].LinkData[LC2]);
				} while (MC_SnippetPtr[LC1].LinkData[++LC2] != -2);

			} else
			
				VR_PrintMessage ("- NULL TABLE -");
		}

		VR_PrintMessage ("\n");
		for (LC1 = MC_MainTablePtr[LC].StartEntryOfArcs; 
					LC1 <= MC_MainTablePtr[LC].EndEntryOfArcs; LC1++)
		{
			VR_PrintMessage ("\n  -&- Arc #%d -&-", LC1);
			VR_PrintMessage ("\n   StartFrame : %d", 
									MC_ArcPtr[LC1].StartFrame);
			VR_PrintMessage ("\n     EndFrame : %d", 
									MC_ArcPtr[LC1].EndFrame);
			VR_PrintMessage ("\n   NextSnippet: %d", 
									MC_ArcPtr[LC1].NextSnippet);
			VR_PrintMessage ("\n      EntryFrm: %d", 
									MC_ArcPtr[LC1].NextSnippetEntryFrame);
			VR_PrintMessage ("\n    Data1     : '%s'", 
									MC_ArcPtr[LC1].BlindData[0]);
			VR_PrintMessage ("\n    Data2     : '%s'", 
									MC_ArcPtr[LC1].BlindData[1]);
			VR_PrintMessage ("\n    Data3     : '%s'", 
									MC_ArcPtr[LC1].BlindData[2]);
			VR_PrintMessage ("\n    Data4     : '%s'", 
									MC_ArcPtr[LC1].BlindData[3]);
		}
	}
#endif

	return (1);
}

char	*VR_MCScript =
" DEF PlayererScript Script\n\
 {\n\
  eventIn SFFloat	set_fraction IS set_fraction\n\
  eventOut SFVec3f	LTranslationOut IS LTranslationOut\n\
  eventOut SFRotation	LOrientationOut IS LOrientationOut \n\
  field MFVec3f    LTransAnim IS LTransAnim \n\
  field MFRotation LOriAnim IS LOriAnim \n\
  url\n\
  [\n\
   \"vrmlscript:\n\
\n\
     function set_fraction(eventValue)\n\
     {\n\
      LCurrentFrame=eventValue*4;\n\
      LTranslationOut=LTransAnim[LCurrentFrame];\n\
      LOrientationOut=LOriAnim[LCurrent Frame];\n\
/*\n\
      LMainTimer=eventValue*6.28;\n\
      LTranslationOut[0]=Math.sin(LMainTimer)*5.0;\n\
      LTranslationOut[1]=Math.cos(LMainTimer)*1.0;\n\
	  LTranslationOut[2]=0.0;\n\
*/\n\
     }\",\n\
  ]\n\
 }\n\
";


/* ====================================== */
/* Insert the Metacycle driver script	 */
/* ====================================== */
int 
VR_MetaCycleCreateScript (int LAnimStart, int LAnimEnd, VRShapeRec * LVRShapes, int LNumOfShapes, FILE * LOutFile, char *LOutBuffer, unsigned int *LBufferPtrP, char *LIndentStr, int *LCurrentIndentP, int *LActMsgLineNum)
{
	char            LTmpStr0[64];
	unsigned int   LBufferPtr = *LBufferPtrP;
	int             LCurrentIndent = *LCurrentIndentP;
	register int LC = 0,
	                LFrame;
	register VRTransformRec *LActTransform;
	register int LShapeID;

	VR_SetMessageLine (*LActMsgLineNum, "Inserting Metacycle driver script\n");
	(*LActMsgLineNum)++;
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "PROTO MCDriver\n");
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "[\n");
	M_IncIndentStr (LIndentStr, LCurrentIndent);

	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "eventIn SFFloat	set_fraction\n");
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "field SFInt32		LUserInput 0\n");
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "field SFInt32		LMCFrameOffset ");
	sprintf (LTmpStr0, "%d\n", LAnimStart);
	LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);

	for (LShapeID = 0; LShapeID < LNumOfShapes; LShapeID++)
	{
		if (LVRShapes[LShapeID].TransAnimated)
		{
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "field MFVec3f		LTransAnim");
			sprintf (LTmpStr0, "%d\n", LShapeID);
			LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "[\n");
			M_IncIndentStr (LIndentStr, LCurrentIndent);
			fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
			LBufferPtr = 0;

			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "");
			for (LFrame = LAnimStart, LC = 0, LActTransform = LVRShapes[LShapeID].TransformAnimation; LFrame <= LAnimEnd; LFrame++, LActTransform++)
			{
				sprintf (LTmpStr0, "%f %f %f ", LActTransform->Translate[0], LActTransform->Translate[1], LActTransform->Translate[2]);
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);
				if (LC++ >= 3)
				{
					LC = 0;
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, "\n");
					if (LFrame < LAnimEnd)
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "");
				}
				if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
				{
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
			}
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "\n");
			M_DecIndentStr (LIndentStr, LCurrentIndent);
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");
		}
		if (LVRShapes[LShapeID].OriAnimated)
		{
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "field MFRotation	LOriAnim");
			sprintf (LTmpStr0, "%d\n", LShapeID);
			LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "[\n");
			M_IncIndentStr (LIndentStr, LCurrentIndent);
			fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
			LBufferPtr = 0;

			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "");
			for (LFrame = LAnimStart, LActTransform = LVRShapes[LShapeID].TransformAnimation; LFrame <= LAnimEnd; LFrame++, LActTransform++)
			{
				sprintf (LTmpStr0, "%f %f %f %f ", LActTransform->Orientation[0], LActTransform->Orientation[1], LActTransform->Orientation[2], LActTransform->Orientation[3]);
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);
				if (LC++ >= 3)
				{
					LC = 0;
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, "\n");
					if (LFrame < LAnimEnd)
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "");
				}
				if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
				{
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
			}
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "\n");
			M_DecIndentStr (LIndentStr, LCurrentIndent);
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");
		}
	}

	M_DecIndentStr (LIndentStr, LCurrentIndent);
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");

	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
	M_IncIndentStr (LIndentStr, LCurrentIndent);
	fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
	LBufferPtr = 0;

	fwrite (VR_MCScript, 1, strlen (VR_MCScript), LOutFile);

	M_DecIndentStr (LIndentStr, LCurrentIndent);
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");


	fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
	LBufferPtr = 0;
	*LBufferPtrP = LBufferPtr;
	*LCurrentIndentP = LCurrentIndent;
	return (1);
}


/* ====================================== */
/* Collect animation data		 */
/* ====================================== */
Boolean 
VR_GetAnimation (int *LAnimStartP, int *LAnimEndP, 
				 VRShapeRec * LVRShapes, int LNumOfShapes, 
				 VRMaterialRec * LVRMaterials, int LNumOfMaterials, 
				 VRLightRec * LVRLights, int LNumOfLights, 
				 VRCameraRec *LVRCameras, int LNumOfCameras, int *LActMsgLineNum)
{
	int             LI,
	                LVI,
	                LAnimStart,
	                LAnimEnd,
	                LVCount;
	int            *LVIndices;
	register int	LC,
	                LC1,
	                LN1,
	                LMatID,
	                LFrame = 0,
	                LFrameL;
	register VRTransformRec *LActTransform;
	register VRVertexRec *LActVertexArray;
	int            *LActVertexIndices;
	register VRMaterialValRec *LMatVal;
	register VRLightValRec *LLightVal;
	register VRCameraValRec *LCameraVal;
	register double LF;
	int             LVIdxN;
	float          *LMatrix;
	float          *LQ;
	DECOMP_MAT      LDecomposedMatrix;
	float           LRF,
	                LGF,
	                LBF,
					LWF;
	//float           LRF1;
	VRShapeRec     *LVRShape;
	DtVec3f        *LVertices;
	Boolean         LAnimated,
	                LLampOk;

	if ((!VR_AutoUpdate) && (VR_SceneInited))
		return (true);
	LAnimated = false;
	if (VR_AnimMaterials)
		DtSetUpdateShaders (1);
	else
		DtSetUpdateShaders (0);
	LAnimStart = *LAnimStartP;
	LAnimEnd = *LAnimEndP;

	//Reset flags in internal structures
	//
	if (LAnimEnd > LAnimStart)
	{
		LFrame = LAnimStart;
		VR_SetMessageLine (*LActMsgLineNum, 
			"Collecting animation data... frames %d to %d\n", 
											LAnimStart, LAnimEnd);
		(*LActMsgLineNum)++;

		for (LC = 0, LVRShape = LVRShapes; LC < LNumOfShapes; LC++, LVRShape++)
		{
			LVRShape->TransformAnimation = (VRTransformRec *) malloc (sizeof (VRTransformRec) * (LAnimEnd - LAnimStart + 1));
			if (VR_AnimVertices 
				&& DtShapeGetVerticesAnimated (LC, &LVCount, &LVIndices))
			{
				LVRShape->NumOfAnimatedVertices = LVCount;
				LVRShape->AnimatedVertices = LVIndices;
				LVRShape->VertexAnimation = (VRVertexRec *) malloc (LVCount * sizeof (VRVertexRec) * (LAnimEnd - LAnimStart + 1));
			}

			if (VR_KeyCurves )
			{
				if ( !LVRShape->trsAnimKeyFrames ) 
					LVRShape->trsAnimKeyFrames = new MIntArray;
				DtShapeGetTRSAnimKeys( LC, LVRShape->trsAnimKeyFrames );

                if ( !LVRShape->vtxAnimKeyFrames )
                    LVRShape->vtxAnimKeyFrames = new MIntArray;
                DtShapeGetVtxAnimKeys( LC, LVRShape->vtxAnimKeyFrames );

			}
			LVRShape->TransAnimated = false;
			LVRShape->OriAnimated = false;
			LVRShape->ScaleAnimated = false;
			LVRShape->ShapeAnimated = false;
		}
		for (LC = 0; LC < LNumOfMaterials; LC++)
		{
			LVRMaterials[LC].Animation = (VRMaterialValRec *) malloc (sizeof (VRMaterialValRec) * (LAnimEnd - LAnimStart + 1));
			LVRMaterials[LC].AmbientColAnimated = false;
			LVRMaterials[LC].DiffuseColAnimated = false;
			LVRMaterials[LC].SpecularColAnimated = false;
			LVRMaterials[LC].EmissiveColAnimated = false;
			LVRMaterials[LC].ShininessAnimated = false;
			LVRMaterials[LC].TransparencyAnimated = false;

            if (VR_KeyCurves )
            {   
                if ( !LVRMaterials[LC].AnimKeyFrames ) 
                    LVRMaterials[LC].AnimKeyFrames = new MIntArray;
                DtMtlGetAnimKeys( LC, LVRMaterials[LC].AnimKeyFrames );
            }

		}
		for (LC = 0; LC < LNumOfLights; LC++)
		{
			DtLightGetType (LC, &LI);
			LVRLights[LC].Type = LI;
			LVRLights[LC].Animation = (VRLightValRec *) malloc (sizeof (VRLightValRec) * (LAnimEnd - LAnimStart + 1));
			LVRLights[LC].LocationAnimated = false;
			LVRLights[LC].ColorAnimated = false;
			LVRLights[LC].IntensityAnimated = false;
			LVRLights[LC].CutOffAngleAnimated = false;
			LVRLights[LC].DirectionAnimated = false;
			LVRLights[LC].OnAnimated = false;

            if (VR_KeyCurves )
            {   
                if ( !LVRLights[LC].trsAnimKeyFrames ) 
                    LVRLights[LC].trsAnimKeyFrames = new MIntArray;
                DtLightGetTRSAnimKeys( LC, LVRLights[LC].trsAnimKeyFrames );
                
                if ( !LVRLights[LC].AnimKeyFrames )
                    LVRLights[LC].AnimKeyFrames = new MIntArray;
                DtLightGetAnimKeys( LC, LVRLights[LC].AnimKeyFrames );
            }

		}

		for (LC = 0; LC < LNumOfCameras; LC++)
		{
			LVRCameras[LC].Animation = (VRCameraValRec *) malloc (sizeof (VRCameraValRec) * (LAnimEnd - LAnimStart + 1));
			LVRCameras[LC].LocationAnimated = false;
			LVRCameras[LC].DirectionAnimated = false;
			LVRCameras[LC].OnAnimated = false;

            if (VR_KeyCurves )
            {   
                if ( !LVRCameras[LC].trsAnimKeyFrames ) 
                    LVRCameras[LC].trsAnimKeyFrames = new MIntArray;
                DtCameraGetTRSAnimKeys( LC, LVRCameras[LC].trsAnimKeyFrames );
                
                if ( !LVRCameras[LC].AnimKeyFrames )
                    LVRCameras[LC].AnimKeyFrames = new MIntArray;
                DtCameraGetAnimKeys( LC, LVRCameras[LC].AnimKeyFrames );
            }

		}

		//Go through the given frame range to get the animation data
		//

		int LAnimBy = DtFrameGetBy();

#ifndef MAYA101
	    //
	    //  if we have this function then lets use it
	    //
	    //  Allow the user to break out of the process loop early
	    //

	    MComputation computation;
	
	    computation.beginComputation();
#endif

		for (LFrame = LAnimStart, LFrameL = 0; 
				(LFrame <= LAnimEnd) && VR_ConversionRun; 
					LFrame+=LAnimBy, LFrameL++)
		{
			VR_SetMessageLine (*LActMsgLineNum, "Frame: %d\n", LFrame);
			DtFrameSet (LFrame);
			//Get vertex animation
			//
			for (LC = 0; LC < LNumOfShapes; LC++)
			{
				LN1 = LVRShapes[LC].NumOfAnimatedVertices;
				LActVertexIndices = LVRShapes[LC].AnimatedVertices;
				if (VR_AnimVertices 
					&& ((LActVertexArray = (LVRShapes[LC].VertexAnimation) + LFrameL * LN1) != NULL))
				{
					DtShapeGetVertices (LC, &LVIdxN, &LVertices);
					for (LC1 = 0; LC1 < LN1; LC1++)
					{
						LVI = LActVertexIndices[LC1];
						LActVertexArray[LC1].X = LVertices[LVI].vec[0];
						LActVertexArray[LC1].Y = LVertices[LVI].vec[1];
						LActVertexArray[LC1].Z = LVertices[LVI].vec[2];
					}
				}
				//Get the transformation matrix
				//
				if (VR_AnimTrans && ((LActTransform = (LVRShapes[LC].TransformAnimation) + LFrameL) != NULL))
				{
					DtShapeGetMatrix (LC, &LMatrix);
					VR_DecomposeMatrix (LMatrix, &LDecomposedMatrix, VRROT_NO_EULER);
					LActTransform->Translate[0] = LDecomposedMatrix.Translate[0];
					LActTransform->Translate[1] = LDecomposedMatrix.Translate[1];
					LActTransform->Translate[2] = LDecomposedMatrix.Translate[2];

					M_QuaternionToVRML (LDecomposedMatrix.Orientation, LQ);
					LActTransform->Orientation[0] = LQ[QX];
					LActTransform->Orientation[1] = LQ[QY];
					LActTransform->Orientation[2] = LQ[QZ];
					LActTransform->Orientation[3] = LQ[QW];

					LActTransform->Scale[0] = LDecomposedMatrix.Scale[0];
					LActTransform->Scale[1] = LDecomposedMatrix.Scale[1];
					LActTransform->Scale[2] = LDecomposedMatrix.Scale[2];

					if (LFrame > LAnimStart)
					{
						if (!(LVRShapes[LC].TransAnimated))
						{
							if ((LActTransform->Translate[0] != (LActTransform - 1)->Translate[0])
							    || (LActTransform->Translate[1] != (LActTransform - 1)->Translate[1])
							    || (LActTransform->Translate[2] != (LActTransform - 1)->Translate[2]))
								LVRShapes[LC].TransAnimated = true;
						}
						if (!(LVRShapes[LC].OriAnimated))
						{
							if ((LActTransform->Orientation[0] != (LActTransform - 1)->Orientation[0])
							    || (LActTransform->Orientation[1] != (LActTransform - 1)->Orientation[1])
							    || (LActTransform->Orientation[2] != (LActTransform - 1)->Orientation[2])
							    || (LActTransform->Orientation[3] != (LActTransform - 1)->Orientation[3]))
								LVRShapes[LC].OriAnimated = true;
						}
						if (!(LVRShapes[LC].ScaleAnimated))
						{
							if ((LActTransform->Scale[0] != (LActTransform - 1)->Scale[0])
							    || (LActTransform->Scale[1] != (LActTransform - 1)->Scale[1])
							    || (LActTransform->Scale[2] != (LActTransform - 1)->Scale[2]))
								LVRShapes[LC].ScaleAnimated = true;
						}
					}
				}
			}

			for (LMatID = 0; LMatID < LNumOfMaterials; LMatID++)
			{
				if ((LMatVal = LVRMaterials[LMatID].Animation + LFrameL) != NULL)
				{
					DtMtlGetAllClrbyID (LMatID, 0, &(LMatVal->AmbientR), &(LMatVal->AmbientG),
							    &(LMatVal->AmbientB), &(LMatVal->DiffuseR), &(LMatVal->DiffuseG),
							    &(LMatVal->DiffuseB), &(LMatVal->SpecularR), &(LMatVal->SpecularG),
							    &(LMatVal->SpecularB), &(LMatVal->EmissiveR), &(LMatVal->EmissiveG),
							    &(LMatVal->EmissiveB), &(LMatVal->Shininess), &(LMatVal->Transparency));

					//Check difference with previous frame                 

					if (LFrame > LAnimStart)
					{
						if (!(LVRMaterials[LMatID].DiffuseColAnimated))
						{
							if ((LMatVal->DiffuseR != (LMatVal - 1)->DiffuseR)
							    || (LMatVal->DiffuseG != (LMatVal - 1)->DiffuseG)
							    || (LMatVal->DiffuseB != (LMatVal - 1)->DiffuseB))
								LVRMaterials[LMatID].DiffuseColAnimated = true;
						}
						if (!(LVRMaterials[LMatID].SpecularColAnimated))
						{
							if ((LMatVal->SpecularR != (LMatVal - 1)->SpecularR)
							    || (LMatVal->SpecularG != (LMatVal - 1)->SpecularG)
							    || (LMatVal->SpecularB != (LMatVal - 1)->SpecularB))
								LVRMaterials[LMatID].SpecularColAnimated = true;
						}
						if (!(LVRMaterials[LMatID].EmissiveColAnimated))
						{
							if ((LMatVal->EmissiveR != (LMatVal - 1)->EmissiveR)
							    || (LMatVal->EmissiveG != (LMatVal - 1)->EmissiveG)
							    || (LMatVal->EmissiveB != (LMatVal - 1)->EmissiveB))
								LVRMaterials[LMatID].EmissiveColAnimated = true;
						}
						if (!(LVRMaterials[LMatID].ShininessAnimated))
						{
							if (LMatVal->Shininess != (LMatVal - 1)->Shininess)
								LVRMaterials[LMatID].ShininessAnimated = true;
						}
						if (!(LVRMaterials[LMatID].TransparencyAnimated))
						{
							if (LMatVal->Transparency != (LMatVal - 1)->Transparency)
								LVRMaterials[LMatID].TransparencyAnimated = true;
						}
					}
				}
			}

			for (LC = 0; LC < LNumOfLights; LC++)
			{
				if ((LLightVal = LVRLights[LC].Animation + LFrameL) != NULL)
				{
					switch (LVRLights[LC].Type)
					{
					case DT_POINT_LIGHT:
					case DT_DIRECTIONAL_LIGHT:
					case DT_SPOT_LIGHT:
						LLampOk = true;
						break;

					default:
						LLampOk = false;
						break;
					}
					if (LLampOk)
					{
						switch (LVRLights[LC].Type)
						{
						case DT_POINT_LIGHT:
						case DT_SPOT_LIGHT:
							DtLightGetPosition (LC, &LRF, &LGF, &LBF);
							LLightVal->Location[0] = LRF;
							LLightVal->Location[1] = LGF;
							LLightVal->Location[2] = LBF;

							//Check differences with previous frame

							if ((!(LVRLights[LC].LocationAnimated)) 
													&& (LFrame > LAnimStart))
							{
								if ((LRF != (LLightVal - 1)->Location[0])
								    || (LGF != (LLightVal - 1)->Location[1])
								    || (LBF != (LLightVal - 1)->Location[2]))
									LVRLights[LC].LocationAnimated = true;
							}
							break;
						}

						switch (LVRLights[LC].Type)
						{
						case DT_SPOT_LIGHT:
						case DT_DIRECTIONAL_LIGHT:
							DtLightGetDirection (LC, &LRF, &LGF, &LBF);
							LLightVal->Direction[0] = LRF;
							LLightVal->Direction[1] = LGF;
							LLightVal->Direction[2] = LBF;
							if ((!(LVRLights[LC].DirectionAnimated)) 
												&& (LFrame > LAnimStart))
							{
								if ((LRF != (LLightVal - 1)->Direction[0])
								    || (LGF != (LLightVal - 1)->Direction[1])
								    || (LBF != (LLightVal - 1)->Direction[2]))
									LVRLights[LC].DirectionAnimated = true;
							}
							break;
						}

						switch (LVRLights[LC].Type)
						{
						case DT_SPOT_LIGHT:
							DtLightGetCutOffAngle (LC, &LRF);
							LLightVal->CutOffAngle = LRF;
							if ((!(LVRLights[LC].CutOffAngleAnimated)) 
													&& (LFrame > LAnimStart))
							{
								if (LRF != (LLightVal - 1)->CutOffAngle)
									LVRLights[LC].CutOffAngleAnimated = true;
							}
							break;
						}

						// Check if colour is animated

						DtLightGetColor (LC, &LRF, &LGF, &LBF);
						LLightVal->ColorR = LRF;
						LLightVal->ColorG = LGF;
						LLightVal->ColorB = LBF;
						if ((!(LVRLights[LC].ColorAnimated)) 
												&& (LFrame > LAnimStart))
						{
							if ((LRF != (LLightVal - 1)->ColorR)
							    || (LGF != (LLightVal - 1)->ColorG)
							    || (LBF != (LLightVal - 1)->ColorB))
								LVRLights[LC].ColorAnimated = true;
						}


						// Check for Light Intensity to be animated

                        DtLightGetIntensity (LC, &LRF);
						LRF = MToVrml2_mapFloatRange( LRF );
                        LLightVal->Intensity = LRF;
		
                        if ((!(LVRLights[LC].IntensityAnimated))
                                                && (LFrame > LAnimStart))
                        {
                            if ((LRF != (LLightVal - 1)->Intensity) )
							{
                                LVRLights[LC].IntensityAnimated = true;
							}
                        }


					}
				}
			}


			for (LC = 0; LC < LNumOfCameras; LC++)
			{
				if ((LCameraVal = LVRCameras[LC].Animation + LFrameL) != NULL)
				{
					DtCameraGetPosition (LC, &LRF, &LGF, &LBF);
					LCameraVal->Location[0] = LRF;
					LCameraVal->Location[1] = LGF;
					LCameraVal->Location[2] = LBF;

					//Check differences with previous frame

					if ((!(LVRCameras[LC].LocationAnimated)) 
													&& (LFrame > LAnimStart))
					{
						if ((LRF != (LCameraVal - 1)->Location[0])
						    || (LGF != (LCameraVal - 1)->Location[1])
						    || (LBF != (LCameraVal - 1)->Location[2]))
							LVRCameras[LC].LocationAnimated = true;
					}

					DtCameraGetOrientationQuaternion (LC, &LRF, &LGF, &LBF, &LWF );
								
					//Convert standard quaternion to 
					// 'VRML rotation' (nx, ny, nz, angle (rad))
					// Normalized

					LF = LWF;
					LWF = acos (LF) * 2.0;
					LF = sqrt (LRF * LRF + LGF * LGF + LBF * LBF);
					if (LF > 0.0)
					{
						LF = 1.0 / LF;
						LRF *= LF;
						LGF *= LF;
						LBF *= LF;
					}

					LCameraVal->Orientation[0] = LRF;
					LCameraVal->Orientation[1] = LGF;
					LCameraVal->Orientation[2] = LBF;
					LCameraVal->Orientation[3] = LWF;

					if ((!(LVRCameras[LC].DirectionAnimated)) 
											&& (LFrame > LAnimStart))
					{
						if ((LRF != (LCameraVal - 1)->Orientation[0])
						    || (LGF != (LCameraVal - 1)->Orientation[1])
						    || (LBF != (LCameraVal - 1)->Orientation[2])
							|| (LWF != (LCameraVal - 1)->Orientation[3]))
							LVRCameras[LC].DirectionAnimated = true;
					}

				}
			}

#ifndef MAYA101
	        if ( computation.isInterruptRequested() )
	        {  
	           VR_ConversionRun = false;
	        }
#endif

		}

#ifndef MAYA101
	    computation.endComputation();
#endif

		LAnimated = true;
		(*LActMsgLineNum)++;
		if (!VR_ConversionRun)
			LAnimEnd = LFrame - 1;
		if (LAnimEnd < LAnimStart)
			LAnimEnd = LAnimStart;

	}

	
	if ((!VR_ConversionRun) && (LFrame > LAnimStart))
		VR_SetMessage ("Animation conversion aborted at frame:\n %d\nThe output file will contain the\nanimation until this frame.\n", LFrame);
	*LAnimStartP = LAnimStart;
	*LAnimEndP = LAnimEnd;

	return (LAnimated);
}

//
//	Routine to scan the array of keyFrames to see if the given
//	element can be found.
//
bool isIn( int frameNum, MIntArray *frameArray )
{
	if ( !frameArray )
		return false;
		
	for ( unsigned int i=0; i < frameArray->length(); i++ )
	{
		if ( frameNum == (*frameArray)[i] )
		{
			return true;
		}
	}
	return false;
}

#define VRM_DefKeys()\
{\
 LFrame=LAnimStart;LF=0.0;\
 if(VR_LongLines)\
 {\
  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"key [ ");\
  for(;LFrame<=LAnimEnd;LFrame+=LAnimStep,LF+=LAnimStepFactor)\
  {\
   if ( !useKeyFrames || isIn( LFrame, keyFrames ) \
   		|| LFrame == LAnimStart || LFrame == LAnimEnd ) \
   {\
     sprintf(LTmpStr0,"%f ",LF);\
	 LBufferPtr+=VR_StrCpyL(LOutBuffer+LBufferPtr,LTmpStr0);\
     if(LBufferPtr>=(VR_OUTBUFFER_SIZE-VR_MAX_LINE_SIZE)) \
   	 { fwrite(LOutBuffer,1,LBufferPtr,LOutFile);LBufferPtr=0; }\
   } \
  }\
  LBufferPtr+=VR_StrCpyL(LOutBuffer+LBufferPtr,"]\n");\
 }\
 else\
 {\
  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"key [\n");\
  M_IncIndentStr(LIndentStr,LCurrentIndent);\
  for(;LFrame<=LAnimEnd;LFrame+=LAnimStep,LF+=LAnimStepFactor)\
  {\
   if ( !useKeyFrames || isIn( LFrame, keyFrames ) \
        || LFrame == LAnimStart || LFrame == LAnimEnd ) \
   {\
     sprintf(LTmpStr0,"%f\n",LF);\
	 M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,LTmpStr0);\
     if(LBufferPtr>=(VR_OUTBUFFER_SIZE-VR_MAX_LINE_SIZE)) \
   	 { fwrite(LOutBuffer,1,LBufferPtr,LOutFile);LBufferPtr=0; }\
   }\
  }\
  M_DecIndentStr(LIndentStr,LCurrentIndent);\
  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"]\n");\
 }\
}

#define VRM_DefKeyValues(i,f)\
{\
 LFrame=LAnimStart;\
 if(VR_LongLines)\
 {\
  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"keyValue [ ");\
  for(;LFrame<=LAnimEnd;LFrame+=LAnimStep,i)\
  {\
   f;strcat(LTmpStr0," ");\
   if ( !useKeyFrames || isIn( LFrame, keyFrames ) \
        || LFrame == LAnimStart || LFrame == LAnimEnd  ) \
   {\
     LBufferPtr+=VR_StrCpyL(LOutBuffer+LBufferPtr,LTmpStr0);\
     if(LBufferPtr>=(VR_OUTBUFFER_SIZE-VR_MAX_LINE_SIZE)) \
	 { fwrite(LOutBuffer,1,LBufferPtr,LOutFile);LBufferPtr=0; }\
   }\
  }\
  LBufferPtr+=VR_StrCpyL(LOutBuffer+LBufferPtr,"]\n");\
 }\
 else\
 {\
  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"keyValue [\n");\
  M_IncIndentStr(LIndentStr,LCurrentIndent);\
  for(;LFrame<=LAnimEnd;LFrame+=LAnimStep,i)\
  {\
   f;strcat(LTmpStr0,"\n"); \
   if ( !useKeyFrames || isIn( LFrame, keyFrames ) \
        || LFrame == LAnimStart || LFrame == LAnimEnd ) \
   { \
     M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,LTmpStr0);\
     if(LBufferPtr>=(VR_OUTBUFFER_SIZE-VR_MAX_LINE_SIZE)) \
	 { fwrite(LOutBuffer,1,LBufferPtr,LOutFile);LBufferPtr=0; }\
   } \
  }\
  M_DecIndentStr(LIndentStr,LCurrentIndent);\
  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"]\n");\
 }\
}


/* ============================================== */
/* Save interpolator based animation data	 */
/* ============================================== */
Boolean 
VR_SaveAnimation (FILE * LOutFile, char *LOutBuffer, unsigned int *LBufferPtrP, 
				  char *LIndentStr, int *LCurrentIndentP, int LAnimStart, int LAnimEnd, 
				  VRShapeRec * LVRShapes, int LNumOfShapes, 
				  VRMaterialRec * LVRMaterials, int LNumOfMaterials, 
				  VRLightRec * LVRLights, int LNumOfLights,
				  VRCameraRec * LVRCameras, int LNumOfCameras)
{
	unsigned int   LBufferPtr = *LBufferPtrP;
	int             LVI,
	                LCurrentIndent = *LCurrentIndentP;
	register int LShapeID;
	register int LC,
	                LC1,
	                LN1,
	                LFrameL,
	                LMatID,
	                LFrame;
	register VRShapeRec *LActShape;
	register VRVertexRec *LActVertexArray;
	register VRTransformRec *LActTransform;
	register VRMaterialValRec *LMatVal;
	register VRMaterialValRec *LAnimMatVals;
	register VRLightValRec *LLightVal;
	register VRLightValRec *LAnimLightVals;
	register VRCameraValRec *LCameraVal;
	register VRCameraValRec *LAnimCameraVals;
	register double LAnimStepFactor;
	register double LF;
	char           *LShapeName;
	char           *LMatName;
	char           *LTmpStrN;
	char            LMatNameF[256];
	char            LTmpStrC[256],
	                LTmpStrCN[256];
	char            LLightName[256];
	char			LCameraName[256];
	register char   LTmpStr0[512];
	int             LVIdxN;
	int            *LActVertexIndices;
	DtVec3f        *LVertices;

	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "DEF TOUCH TouchSensor {}\n");

	//M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "DEF NNAnim CoKeyframeAnimation\n");
	//#06{
	//M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
	//M_IncIndentStr (LIndentStr, LCurrentIndent);

	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "DEF TimeSource TimeSensor\n");
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
	M_IncIndentStr (LIndentStr, LCurrentIndent);
	if (VR_LoopAnim)
		M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "loop			TRUE\n");
	else
		M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "loop			FALSE\n");
	if (VR_FramesPerSec > 0.0)
		sprintf (LTmpStr0, "cycleInterval	%f\n", (double) (LAnimEnd - LAnimStart + 1) / VR_FramesPerSec);
	else
		sprintf (LTmpStr0, "cycleInterval	10.0\n");
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
	//M_DecIndentStr (LIndentStr, LCurrentIndent);
	//M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
	fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
	LBufferPtr = 0;

	//#06}
	M_DecIndentStr (LIndentStr, LCurrentIndent);
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");

	int LAnimStep = DtFrameGetBy();

	bool useKeyFrames = false;
	MIntArray	*keyFrames;
	
	LAnimStepFactor = 1.0 * LAnimStep / (float) (LAnimEnd - LAnimStart);
	if (VR_AnimVertices || VR_AnimTrans)
	{
		LNumOfShapes = DtShapeGetCount ();
		strcpy (LTmpStrC, "%.1f %.1f %.1f  ");
		LTmpStrC[2] = LTmpStrC[7] = LTmpStrC[12] = (char) VR_Precision + '0';
		strcpy (LTmpStrCN, "%.1f %.1f %.1f\n");
		LTmpStrCN[2] = LTmpStrCN[7] = LTmpStrCN[12] = (char) VR_Precision + '0';

		for (LShapeID = 0, LActShape = LVRShapes; 
				LShapeID < LNumOfShapes; LShapeID++, LActShape++)
		{
			LShapeName = LVRShapes[LShapeID].Name;

			useKeyFrames = false;
			keyFrames = NULL;
			
			//Animate vertices
			//
			if (VR_AnimVertices)
			{

                useKeyFrames = (0 != VR_KeyCurves);
                keyFrames = LVRShapes[LShapeID].vtxAnimKeyFrames;

				if (LVRShapes[LShapeID].NumOfAnimatedVertices != 0)
				{
					DtShapeGetVertices (LShapeID, &LVIdxN, &LVertices);

					LN1 = LVRShapes[LShapeID].NumOfAnimatedVertices;
					LActVertexIndices = LVRShapes[LShapeID].AnimatedVertices;

					sprintf (LTmpStr0, "DEF %sVtxAnim CoordinateInterpolator\n%s{\n", LShapeName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);

					VRM_DefKeys ();

					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;

					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "keyValue [\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					for (LFrame = LAnimStart, LFrameL = 0; 
								LFrame <= LAnimEnd; 
								LFrame += LAnimStep, LFrameL++)
					{
				
						// Check to see if we are doing keyframe, and if this
						// frame matches one that we want to output.
						if ( useKeyFrames &&
							      !(LFrame == LAnimStart ||
								    LFrame == LAnimEnd   ||
							  	    isIn( LFrame, keyFrames) ) )

							continue;
						
						//
						// else we want to output this if it has anything
						//

						if (VR_AnimVertices && ((LActVertexArray = (LVRShapes[LShapeID].VertexAnimation) + LFrameL * LN1) != NULL))
						{
							for (LC1 = 0; LC1 < LN1; LC1++)
							{
								LVI = LActVertexIndices[LC1];
								LVertices[LVI].vec[0] = LActVertexArray[LC1].X;
								LVertices[LVI].vec[1] = LActVertexArray[LC1].Y;
								LVertices[LVI].vec[2] = LActVertexArray[LC1].Z;
							}
						}
						if (VR_LongLines)
						{
							LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LIndentStr);
							for (LC1 = 0; LC1 < LVIdxN; LC1++)
							{
								if (LC1 < (LVIdxN - 1))
									sprintf (LTmpStr0, LTmpStrC, LVertices[LC1].vec[0], LVertices[LC1].vec[1], LVertices[LC1].vec[2]);
								else
									sprintf (LTmpStr0, LTmpStrCN, LVertices[LC1].vec[0], LVertices[LC1].vec[1], LVertices[LC1].vec[2]);

								LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);
								if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
								{
									fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
									LBufferPtr = 0;
								}
							}
						} else {
							for (LC1 = 0; LC1 < LVIdxN; LC1++)
							{
								sprintf (LTmpStr0, LTmpStrCN, LVertices[LC1].vec[0], LVertices[LC1].vec[1], LVertices[LC1].vec[2]);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
								if ((LC1 == (LVIdxN - 1)) && (LFrame < LAnimEnd))
									M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "\n");
								if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
								{
									fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
									LBufferPtr = 0;
								}
							}
						}
					}
					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");


					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
				{
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
			}
			//Animate translation
			//
			if (VR_AnimTrans)
			{
				useKeyFrames = (0 != VR_KeyCurves);
				keyFrames = LVRShapes[LShapeID].trsAnimKeyFrames;

				if (LVRShapes[LShapeID].TransAnimated)
				{
					sprintf (LTmpStr0, 
						"DEF %sPosAnim PositionInterpolator\n%s{\n", 
													LShapeName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);

					VRM_DefKeys ();

					LActTransform = LVRShapes[LShapeID].TransformAnimation;
					VRM_DefKeyValues (LActTransform++, sprintf (LTmpStr0, "%f %f %f", LActTransform->Translate[0], LActTransform->Translate[1], LActTransform->Translate[2]));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//Animate orientation
				//
				if (LVRShapes[LShapeID].OriAnimated)
				{
					sprintf (LTmpStr0, "DEF %sRotAnim OrientationInterpolator\n%s{\n", LShapeName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);

					VRM_DefKeys ();
					LActTransform = LVRShapes[LShapeID].TransformAnimation;
					VRM_DefKeyValues (LActTransform++, sprintf (LTmpStr0, "%f %f %f %f", LActTransform->Orientation[0], LActTransform->Orientation[1], LActTransform->Orientation[2], LActTransform->Orientation[3]));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//Animate scaling
				//
				if (LVRShapes[LShapeID].ScaleAnimated)
				{
					sprintf (LTmpStr0, "DEF %sSclAnim PositionInterpolator\n%s{\n", LShapeName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					
					VRM_DefKeys ();
					
					LActTransform = LVRShapes[LShapeID].TransformAnimation;
					VRM_DefKeyValues (LActTransform++, sprintf (LTmpStr0, "%f %f %f", LActTransform->Scale[0], LActTransform->Scale[1], LActTransform->Scale[2]));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
			}
		}
	}
	//Animate materials
	//

    useKeyFrames = false;
    keyFrames = NULL;

	if (VR_AnimMaterials)
	{
		for (LMatID = 0; LMatID < LNumOfMaterials; LMatID++)
		{
            useKeyFrames = (0 != VR_KeyCurves);
            keyFrames = LVRMaterials[LMatID].AnimKeyFrames;

			if ((LAnimMatVals = LVRMaterials[LMatID].Animation) != NULL)
			{
				DtMtlGetNameByID (LMatID, &LMatName);
				sprintf (LMatNameF, "%s_%d", LMatName, LMatID);
				VR_FilterName (LMatNameF);

				//Diffuse color
				//
				if (LVRMaterials[LMatID].DiffuseColAnimated)
				{
					sprintf (LTmpStr0, "DEF %sMatDifAnim ColorInterpolator\n%s{\n", LMatNameF, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LMatVal = LAnimMatVals;
					VRM_DefKeyValues (LMatVal++, sprintf (LTmpStr0, "%f %f %f", LMatVal->DiffuseR, LMatVal->DiffuseG, LMatVal->DiffuseB));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//Specular color
				//
				if (LVRMaterials[LMatID].SpecularColAnimated)
				{
					sprintf (LTmpStr0, "DEF %sMatSpcAnim ColorInterpolator\n%s{\n", LMatNameF, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LMatVal = LAnimMatVals;
					VRM_DefKeyValues (LMatVal++, sprintf (LTmpStr0, "%f %f %f", LMatVal->SpecularR, LMatVal->SpecularG, LMatVal->SpecularB));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//Emissive color
				//
				if (LVRMaterials[LMatID].EmissiveColAnimated)
				{
					sprintf (LTmpStr0, "DEF %sMatEmsAnim ColorInterpolator\n%s{\n", LMatNameF, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LMatVal = LAnimMatVals;
					VRM_DefKeyValues (LMatVal++, sprintf (LTmpStr0, "%f %f %f", LMatVal->EmissiveR, LMatVal->EmissiveG, LMatVal->EmissiveB));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//Shininess
				//
				if (LVRMaterials[LMatID].ShininessAnimated)
				{
					sprintf (LTmpStr0, "DEF %sMatShnAnim ScalarInterpolator\n%s{\n", LMatNameF, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LMatVal = LAnimMatVals;
					VRM_DefKeyValues (LMatVal++, sprintf (LTmpStr0, "%f", LMatVal->Shininess));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//Transparency
				//
				if (LVRMaterials[LMatID].TransparencyAnimated)
				{
					sprintf (LTmpStr0, "DEF %sMatTrsAnim ScalarInterpolator\n%s{\n", LMatNameF, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();

					LMatVal = LAnimMatVals;
					VRM_DefKeyValues (LMatVal++, sprintf (LTmpStr0, "%f", LMatVal->Transparency));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
			}
		}
	}
	//Animate lights
	//
	if (VR_AnimLights)
	{
		for (LC = 0; LC < LNumOfLights; LC++)
		{
            useKeyFrames = (0 != VR_KeyCurves);
            keyFrames = NULL;

			if ((LAnimLightVals = LVRLights[LC].Animation) != NULL)
			{
				sprintf (LLightName, "Light_%d", LC);

				//Location
				//
                keyFrames = LVRLights[LC].trsAnimKeyFrames;

				if (LVRLights[LC].LocationAnimated)
				{
					sprintf (LTmpStr0, "DEF %sLightLocAnim PositionInterpolator\n%s{\n", LLightName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LLightVal = LAnimLightVals;
					VRM_DefKeyValues (LLightVal++, sprintf (LTmpStr0, "%f %f %f", LLightVal->Location[0], LLightVal->Location[1], LLightVal->Location[2]));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//Direction
				//
				if (LVRLights[LC].DirectionAnimated)
				{
					sprintf (LTmpStr0, "DEF %sLightDirAnim PositionInterpolator\n%s{\n", LLightName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LLightVal = LAnimLightVals;
					VRM_DefKeyValues (LLightVal++, sprintf (LTmpStr0, "%f %f %f", LLightVal->Direction[0], LLightVal->Direction[1], LLightVal->Direction[2]));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//CutOffAngle
				//

                keyFrames = LVRLights[LC].AnimKeyFrames;

				if (LVRLights[LC].CutOffAngleAnimated)
				{
					sprintf (LTmpStr0, "DEF %sLightCOAAnim ScalarInterpolator\n%s{\n", LLightName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LLightVal = LAnimLightVals;
					VRM_DefKeyValues (LLightVal++, sprintf (LTmpStr0, "%f", LLightVal->CutOffAngle));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//Color
				//
				if (LVRLights[LC].ColorAnimated)
				{
					sprintf (LTmpStr0, "DEF %sLightColAnim ColorInterpolator\n%s{\n", LLightName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LLightVal = LAnimLightVals;
					VRM_DefKeyValues (LLightVal++, 
						sprintf (LTmpStr0, "%f %f %f", 
					LLightVal->ColorR, LLightVal->ColorG, LLightVal->ColorB));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}

				// Now output the Light Intensity Animation if needed

                if (LVRLights[LC].IntensityAnimated)
                {
                    sprintf (LTmpStr0, 
							"DEF %sLightIntAnim ScalarInterpolator\n%s{\n", 
													LLightName, LIndentStr);
                    M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
                    M_IncIndentStr (LIndentStr, LCurrentIndent);

                    VRM_DefKeys ();

                    LLightVal = LAnimLightVals;
                    VRM_DefKeyValues (LLightVal++,
                        sprintf (LTmpStr0, "%f", LLightVal->Intensity) );
                    
                    M_DecIndentStr (LIndentStr, LCurrentIndent);
                    M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
                }   

				if (LVRLights[LC].OnAnimated)
				{
					/*
					  sprintf(LTmpStr0,"DEF %sLightOnAnim PositionInterpolator\n%s{\n",LLight Name,LIndentStr); 
					  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,LTmpStr0); 
					  M_IncIndentStr(LIndentStr,LCurrentIndent); 
					  
					 VRM_DefKeys();
					 LLightVal=LAnimLightVals;
					 VRM_DefKeyValues(LMatVal++,sprintf(
					 LTmpStr0,LLightVal->ColorR));
					  
					 M_DecIndentStr(LIndentStr,LCurretIndent);
					 M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"}\n");
					 */
				}
			}
		}
	}

	//Animate cameras
	//
	if (VR_AnimCameras)
	{
		for (LC = 0; LC < LNumOfCameras; LC++)
		{
            useKeyFrames = (0 != VR_KeyCurves);
            keyFrames = NULL;

			if ((LAnimCameraVals = LVRCameras[LC].Animation) != NULL)
			{
				if (DtCameraGetName (LC, &LTmpStrN))
					strcpy (LCameraName, LTmpStrN);
				else
					sprintf (LCameraName, "Camera_%d", LC);
				
				//Location
				//
                keyFrames = LVRCameras[LC].trsAnimKeyFrames;

				if (LVRCameras[LC].LocationAnimated)
				{
					sprintf (LTmpStr0, "DEF %sCameraLocAnim PositionInterpolator\n%s{\n", LCameraName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LCameraVal = LAnimCameraVals;
					VRM_DefKeyValues (LCameraVal++, sprintf (LTmpStr0, "%f %f %f", LCameraVal->Location[0], LCameraVal->Location[1], LCameraVal->Location[2]));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
				//Direction
				//
				if (LVRCameras[LC].DirectionAnimated)
				{
					sprintf (LTmpStr0, "DEF %sCameraDirAnim OrientationInterpolator\n%s{\n", LCameraName, LIndentStr);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					VRM_DefKeys ();
					LCameraVal = LAnimCameraVals;
					VRM_DefKeyValues (LCameraVal++, sprintf (LTmpStr0, "%f %f %f %f", LCameraVal->Orientation[0], LCameraVal->Orientation[1], LCameraVal->Orientation[2], LCameraVal->Orientation[3]));

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				}
			}
		}
	}


	/*
	 //#05] 
	 M_DecIndentStr(LIndentStr,LCurrentIndent);
	 M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"]\n");
	  
	 //#04} 
	 M_DecIndentStr(LIndentStr,LCurrentIndent);
	 M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"}\n");
	 */
	*LBufferPtrP = LBufferPtr;
	*LCurrentIndentP = LCurrentIndent;
	return (true);
}

#ifdef WIN32
#pragma warning(default: 4244)
#endif // WIN32
