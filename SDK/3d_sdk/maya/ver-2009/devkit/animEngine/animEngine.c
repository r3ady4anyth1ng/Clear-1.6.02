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

#include <animEngine.h>
#include <fileEngine.h>
#include <utilEngine.h>

/* local constants */
#define kDegRad 0.0174532925199432958
#define kFourThirds (4.0 / 3.0)
#define kTwoThirds (2.0 / 3.0)
#define kOneThird (1.0 / 3.0)
#define kMaxTan 5729577.9485111479

/* tangent types */
enum EtTangentType {
	kTangentFixed,
	kTangentLinear,
	kTangentFlat,
	kTangentStep,
	kTangentStepNext,
	kTangentSlow,
	kTangentFast,
	kTangentSmooth,
	kTangentClamped,
	kTangentPlateau
};
typedef enum EtTangentType EtTangentType;

/* a heavy-weight structure to read a key */
struct EtReadKey {
	EtTime			time;
	EtValue			value;
	EtTangentType	inTangentType;
	EtTangentType	outTangentType;
	EtFloat			inAngle;
	EtFloat			inWeightX;
	EtFloat			inWeightY;
	EtFloat			outAngle;
	EtFloat			outWeightX;
	EtFloat			outWeightY;
	struct EtReadKey *	next;
};
typedef struct EtReadKey EtReadKey;

/* Tangent type words */
#define kWordTangentFixed		((EtByte *)"fixed")
#define kWordTangentLinear		((EtByte *)"linear")
#define kWordTangentFlat		((EtByte *)"flat")
#define kWordTangentSmooth		((EtByte *)"spline")
#define kWordTangentStep		((EtByte *)"step")
#define kWordTangentStepNext	((EtByte *)"stepnext")
#define kWordTangentSlow		((EtByte *)"slow")
#define kWordTangentFast		((EtByte *)"fast")
#define kWordTangentClamped		((EtByte *)"clamped")
#define kWordTangentPlateau		((EtByte *)"plateau")

/*
//	Function Name:
//		wordAsTangentType
//
//	Description:
//		A static helper function to translate a string into a tangent type
//
//  Input Arguments:
//		EtByte *type				The string to translate
//
//  Return Value:
//		EtTangentType type			The translated tangent type (kTangentSmooth
//										by default)
*/
static EtTangentType
wordAsTangentType (EtByte *type)
{
	if (engineUtilStringsMatch (type, kWordTangentFixed)) {
		return (kTangentFixed);
	}
	if (engineUtilStringsMatch (type, kWordTangentLinear)) {
		return (kTangentLinear);
	}
	if (engineUtilStringsMatch (type, kWordTangentFlat)) {
		return (kTangentFlat);
	}
	if (engineUtilStringsMatch (type, kWordTangentSmooth)) {
		return (kTangentSmooth);
	}
	if (engineUtilStringsMatch (type, kWordTangentStepNext)) {
		return (kTangentStepNext);
	}
	if (engineUtilStringsMatch (type, kWordTangentStep)) {
		return (kTangentStep);
	}
	if (engineUtilStringsMatch (type, kWordTangentSlow)) {
		return (kTangentSlow);
	}
	if (engineUtilStringsMatch (type, kWordTangentFast)) {
		return (kTangentFast);
	}
	if (engineUtilStringsMatch (type, kWordTangentClamped)) {
		return (kTangentClamped);
	}
	if (engineUtilStringsMatch (type, kWordTangentPlateau)) {
		return (kTangentPlateau);
	}
	return (kTangentSmooth);
}

/* Infinity type words */
#define kWordInfinityConstant		((EtByte *)"constant")
#define kWordInfinityLinear			((EtByte *)"linear")
#define kWordInfinityCycle			((EtByte *)"cycle")
#define kWordInfinityCycleRelative	((EtByte *)"cycleRelative")
#define kWordInfinityOscillate		((EtByte *)"oscillate")

/*
//	Function Name:
//		wordAsInfinityType
//
//	Description:
//		A static helper function to translate a string into an infinity type
//
//  Input Arguments:
//		EtByte *type				The string to translate
//
//  Return Value:
//		EtInfinityType type			The translated infinity type
//										(kInfinityConstant by default)
*/
static EtInfinityType
wordAsInfinityType (EtByte *type)
{
	if (engineUtilStringsMatch (type, kWordInfinityConstant)) {
		return (kInfinityConstant);
	}
	if (engineUtilStringsMatch (type, kWordInfinityLinear)) {
		return (kInfinityLinear);
	}
	if (engineUtilStringsMatch (type, kWordInfinityCycle)) {
		return (kInfinityCycle);
	}
	if (engineUtilStringsMatch (type, kWordInfinityCycleRelative)) {
		return (kInfinityCycleRelative);
	}
	if (engineUtilStringsMatch (type, kWordInfinityOscillate)) {
		return (kInfinityOscillate);
	}
	return (kInfinityConstant);
}

/*
//	Function Name:
//		assembleAnimCurve
//
//	Description:
//		A static helper function to assemble an EtCurve animation curve
//	from a linked list of heavy-weights keys
//
//  Input Arguments:
//		EtReadKey *firstKey			The linked list of keys
//		EtInt numKeys				The number of keyss
//		EtBoolean isWeighted		Whether or not the curve has weighted tangents
//		EtBoolean useOldSmooth		Whether or not to use pre-Maya2.0 smooth
//									tangent computation
//
//  Return Value:
//		EtCurve *animCurve			The assembled animation curve
//
//	Note:
//		This function will also free the memory used by the heavy-weight keys
*/
static EtCurve *
assembleAnimCurve (EtReadKey *firstKey, EtInt numKeys, EtBoolean isWeighted, EtBoolean useOldSmooth)
{
	EtCurve *	animCurve;
	EtReadKey *	prevKey = kEngineNULL;
	EtReadKey *	key = kEngineNULL;
	EtReadKey *	nextKey = kEngineNULL;
	EtInt		index;
	EtKey *		thisKey;
	EtValue		py, ny, dx;
	EtBoolean	hasSmooth;
	EtFloat		length;
	EtFloat		inLength, outLength, inSlope, outSlope;
	EtFloat		inTanX, inTanY, outTanX, outTanY;
	EtFloat		inTanXs, inTanYs, outTanXs, outTanYs;

	/* make sure we have useful information */
	if ((firstKey == kEngineNULL) || (numKeys == 0)) {
		return (kEngineNULL);
	}

	/* allocate some memory for the animation curve */
	animCurve = (EtCurve *)engineUtilAllocate (sizeof (EtCurve) + ((numKeys - 1) * sizeof (EtKey)));
	if (animCurve == kEngineNULL) {
		return (kEngineNULL);
	}

	/* initialise the animation curve parameters */
	animCurve->numKeys = numKeys;
	animCurve->isWeighted = isWeighted;
	animCurve->isStatic = kEngineTRUE;
	animCurve->preInfinity = kInfinityConstant;
	animCurve->postInfinity = kInfinityConstant;

	/* initialise the cache */
	animCurve->lastKey = kEngineNULL;
	animCurve->lastIndex = -1;
	animCurve->lastInterval = -1;
	animCurve->isStep = kEngineFALSE;
	animCurve->isStepNext = kEngineFALSE;

	/* compute tangents */
	nextKey = firstKey;
	index = 0;
	while (nextKey != kEngineNULL) {
		if (prevKey != kEngineNULL) engineUtilFree ((EtByte *)prevKey);
		prevKey = key;
		key = nextKey;
		nextKey = nextKey->next;

		/* construct the final EtKey (light-weight key) */
		thisKey = &(animCurve->keyList[index++]);
		thisKey->time = key->time;
		thisKey->value = key->value;

		/* compute the in-tangent values */
		/* kTangentClamped */
		if ((key->inTangentType == kTangentClamped) && (prevKey != kEngineNULL)) {
			py = prevKey->value - key->value;
			if (py < 0.0) py = -py;
			ny = (nextKey == kEngineNULL ? py : nextKey->value - key->value);
			if (ny < 0.0) ny = -ny;
			if ((ny <= 0.05) || (py <= 0.05)) {
				key->inTangentType = kTangentFlat;
			}
		}
		hasSmooth = kEngineFALSE;
		switch (key->inTangentType) {
		case kTangentFixed:
			inTanX = key->inWeightX * cos (key->inAngle) * 3.0 ;
			inTanY = key->inWeightY * sin (key->inAngle) * 3.0 ;
			break;
		case kTangentLinear:
			if (prevKey == kEngineNULL) {
				inTanX = 1.0;
				inTanY = 0.0;
			}
			else {
				inTanX = key->time - prevKey->time;
				inTanY = key->value - prevKey->value;
			}
			break;
		case kTangentFlat:
			if (prevKey == kEngineNULL) {
				inTanX = (nextKey == kEngineNULL ? 0.0 : nextKey->time - key->time);
				inTanY = 0.0;
			}
			else {
				inTanX = key->time - prevKey->time;
				inTanY = 0.0;
			}
			break;
		case kTangentStep:
			inTanX = 0.0;
			inTanY = 0.0;
			break;
		case kTangentStepNext:
			inTanX = kEngineFloatMax;
			inTanY = kEngineFloatMax;
			break;
		case kTangentSlow:
		case kTangentFast:
			key->inTangentType = kTangentSmooth;
			if (prevKey == kEngineNULL) {
				inTanX = 1.0;
				inTanY = 0.0;
			}
			else {
				inTanX = key->time - prevKey->time;
				inTanY = key->value - prevKey->value;
			}
			break;
		case kTangentSmooth:
		case kTangentClamped:
			key->inTangentType = kTangentSmooth;
			hasSmooth = kEngineTRUE;
			break;
		}

		/* compute the out-tangent values */
		/* kTangentClamped */
		if ((key->outTangentType == kTangentClamped) && (nextKey != kEngineNULL)) {
			ny = nextKey->value - key->value;
			if (ny < 0.0) ny = -ny;
			py = (prevKey == kEngineNULL ? ny : prevKey->value - key->value);
			if (py < 0.0) py = -py;
			if ((ny <= 0.05) || (py <= 0.05)) {
				key->outTangentType = kTangentFlat;
			}
		}
		switch (key->outTangentType) {
		case kTangentFixed:
			outTanX = key->outWeightX * cos (key->outAngle) * 3.0 ;
			outTanY = key->outWeightY * sin (key->outAngle) * 3.0 ;
			break;
		case kTangentLinear:
			if (nextKey == kEngineNULL) {
				outTanX = 1.0;
				outTanY = 0.0;
			}
			else {
				outTanX = nextKey->time - key->time;
				outTanY = nextKey->value - key->value;
			}
			break;
		case kTangentFlat:
			if (nextKey == kEngineNULL) {
				outTanX = (prevKey == kEngineNULL ? 0.0 : key->time - prevKey->time);
				outTanY = 0.0;
			}
			else {
				outTanX = nextKey->time - key->time;
				outTanY = 0.0;
			}
			break;
		case kTangentStep:
			outTanX = 0.0;
			outTanY = 0.0;
			break;
		case kTangentStepNext:
			outTanX = kEngineFloatMax;
			outTanY = kEngineFloatMax;
			break;
		case kTangentSlow:
		case kTangentFast:
			key->outTangentType = kTangentSmooth;
			if (nextKey == kEngineNULL) {
				outTanX = 1.0;
				outTanY = 0.0;
			}
			else {
				outTanX = nextKey->time - key->time;
				outTanY = nextKey->value - key->value;
			}
			break;
		case kTangentSmooth:
		case kTangentClamped:
			key->outTangentType = kTangentSmooth;
			hasSmooth = kEngineTRUE;
			break;
		}

		/* compute smooth tangents (if necessary) */
		if (hasSmooth) {
			if (useOldSmooth && animCurve->isWeighted) {
				/* pre-Maya 2.0 smooth tangents */
				if ((prevKey == kEngineNULL) && (nextKey != kEngineNULL)) {
					outTanXs = nextKey->time - key->time;
					outTanYs = nextKey->value - key->value;
					inTanXs = outTanXs;
					inTanYs = outTanYs;
				}
				else if ((prevKey != kEngineNULL) && (nextKey == kEngineNULL)) {
					outTanXs = key->time - prevKey->time;
					outTanYs = key->value - prevKey->value;
					inTanXs = outTanXs;
					inTanYs = outTanYs;
				}
				else if ((prevKey != kEngineNULL) && (nextKey != kEngineNULL)) {
					/* There is a CV before and after this one */
					/* Find average of the adjacent in and out tangents */
					inTanXs = key->time - prevKey->time;
					inTanYs = key->value - prevKey->value;
					outTanXs = nextKey->time - key->time;
					outTanYs = nextKey->value - key->value;

					if (inTanXs > 0.01) {
						inSlope = inTanYs / inTanXs;
					}
					else {
						inSlope = 0.0;
					}
					inLength = (inTanXs * inTanXs) + (inTanYs * inTanYs);
					if (outTanXs > 0.01) {
						outSlope = outTanYs / outTanXs;
					}
					else {
						outSlope = 0.0;
					}
					outLength = (outTanXs * outTanXs) + (outTanYs * outTanYs);

					if ((inLength != 0.0) || (outLength != 0.0)) {
						inLength = sqrt (inLength);
						outLength = sqrt (outLength);
						outTanYs = ((inSlope * outLength) + (outSlope * inLength)) / (inLength + outLength);
						inTanYs = outTanYs * inTanXs;
						outTanYs *= outTanXs;
						/*
						// Set the In and Out tangents, at that keyframe, to be the
						// smaller (in length) off the two.
						*/
						inLength = (inTanXs * inTanXs) + (inTanYs * inTanYs);
						outLength = (outTanXs * outTanXs) + (outTanYs * outTanYs);
						if (inLength < outLength) {
							outTanXs = inTanXs;
							outTanYs = inTanYs;
						}
						else {
							inTanXs = outTanXs;
							inTanYs = outTanYs;
						}
					}
				}
				else {
					inTanXs = 1.0;
					inTanYs = 0.0;
					outTanXs = 1.0;
					outTanYs = 0.0; 
				}
			}
			else {
				/* Maya 2.0 smooth tangents */
				if ((prevKey == kEngineNULL) && (nextKey != kEngineNULL)) {
					outTanXs = nextKey->time - key->time;
					outTanYs = nextKey->value - key->value;
					inTanXs = outTanXs;
					inTanYs = outTanYs;
				}
				else if ((prevKey != kEngineNULL) && (nextKey == kEngineNULL)) {
					outTanXs = key->time - prevKey->time;
					outTanYs = key->value - prevKey->value;
					inTanXs = outTanXs;
					inTanYs = outTanYs;
				}
				else if ((prevKey != kEngineNULL) && (nextKey != kEngineNULL)) {
					/* There is a CV before and after this one*/
					/* Find average of the adjacent in and out tangents. */

					dx = nextKey->time - prevKey->time;
					if (dx < 0.0001) {
						outTanYs = kMaxTan;
					}
					else {
						outTanYs = (nextKey->value - prevKey->value) / dx;
					}

					outTanXs = nextKey->time - key->time;
					inTanXs = key->time - prevKey->time;
					inTanYs = outTanYs * inTanXs;
					outTanYs *= outTanXs;
				}
				else {
					inTanXs = 1.0;
					inTanYs = 0.0;
					outTanXs = 1.0;
					outTanYs = 0.0;
				}
			}
			if (key->inTangentType == kTangentSmooth) {
				inTanX = inTanXs;
				inTanY = inTanYs;
			}
			if (key->outTangentType == kTangentSmooth) {
				outTanX = outTanXs;
				outTanY = outTanYs;
			}
		}

		/* make sure the computed tangents are valid */
		if (animCurve->isWeighted) {
			if (inTanX < 0.0) inTanX = 0.0;
			if (outTanX < 0.0) outTanX = 0.0;
		}
		else if (( inTanX == kEngineFloatMax && inTanY == kEngineFloatMax ) 
			|| ( outTanX == kEngineFloatMax && outTanY == kEngineFloatMax ) )
		{
			// SPecial case for step next tangents, do nothing
		}
		else {
			if (inTanX < 0.0) {
				inTanX = 0.0;
			}
			length = sqrt ((inTanX * inTanX) + (inTanY * inTanY));
			if (length != 0.0) {	/* zero lengths can come from step tangents */
				inTanX /= length;
				inTanY /= length;
			}
			if ((inTanX == 0.0) && (inTanY != 0.0)) {
				inTanX = 0.0001;
				inTanY = (inTanY < 0.0 ? -1.0 : 1.0) * (inTanX * kMaxTan);
			}
			if (outTanX < 0.0) {
				outTanX = 0.0;
			}
			length = sqrt ((outTanX * outTanX) + (outTanY * outTanY));
			if (length != 0.0) {	/* zero lengths can come from step tangents */
				outTanX /= length;
				outTanY /= length;
			}
			if ((outTanX == 0.0) && (outTanY != 0.0)) {
				outTanX = 0.0001;
				outTanY = (outTanY < 0.0 ? -1.0 : 1.0) * (outTanX * kMaxTan);
			}
		}

		thisKey->inTanX = inTanX;
		thisKey->inTanY = inTanY;
		thisKey->outTanX = outTanX;
		thisKey->outTanY = outTanY;

		/*
		// check whether or not this animation curve is static (i.e. all the
		// key values are the same)
		*/
		if (animCurve->isStatic) {
			if ((prevKey != kEngineNULL) && (prevKey->value != key->value)) {
				animCurve->isStatic = kEngineFALSE;
			}
			else if ((inTanY != 0.0) || (outTanY != 0.0)) {
				animCurve->isStatic = kEngineFALSE;
			}
		}
	}
	if (animCurve->isStatic) {
		if ((prevKey != kEngineNULL) && (key != kEngineNULL) && (prevKey->value != key->value)) {
			animCurve->isStatic = kEngineFALSE;
		}
	}
	if (prevKey != kEngineNULL) engineUtilFree ((EtByte *)prevKey);
	if (key != kEngineNULL) engineUtilFree ((EtByte *)key);
	return (animCurve);
}

/*
//	Function Name:
//		engineAnimReadCurves
//
//	Description:
//		A function to read a list of channels from a file
//
//  Input Arguments:
//		EtFileName fileName			The name of the file to read
//		EtInt *numCurves			The number of channels read
//
//  Return Value:
//		EtChannel *channelList		A linked list of channels in the file
//
//	Note:
//		The calling function is responsible for freeing the memory allocated
//	by the channel list.  Use: engineAnimFreeChannelList (channelList)
*/
EtChannel *
engineAnimReadCurves (EtFileName fileName, EtInt *numCurves)
{
	EtByte			nameBuffer[256];
	EtByte			attributeBuffer[256];
	EtInt			nameLength;
	EtInt			attributeLength;
	EtChannel *		channel = kEngineNULL;
	EtChannel *		lastChannel;
	EtCurve *		animCurve;
	EtFileHandle	animFile;
	EtByte *		buffer;
	EtBoolean		continueReading = kEngineTRUE;
	EtFloat			animVersion;
	EtFloat			mayaVersion = 2.5;
	EtFloat			frameRate = 24.0;	/* film */
	EtFloat			weight;
	EtValue			unitConversion;
	EtBoolean		endOfCurve;
	EtBoolean		endOfKeys;
	EtBoolean		isWeighted;
	EtInfinityType	preInfinity;
	EtInfinityType	postInfinity;
	EtInt			numKeys;
	EtReadKey *		readKey;
	EtReadKey *		firstKey;
	EtReadKey *		lastKey;
	EtValue			angularConversion = kDegRad;

	if (numCurves != kEngineNULL) {
		*numCurves = 0;
	}
	/* make sure we have a valid file name */
	if (fileName == kEngineNULL) {
		return (kEngineNULL);
	}
	/* make sure we can open the file */
	animFile = engineFileOpen (fileName);
	if (animFile == kFileNotOpened) {
		return (kEngineNULL);
	}

	/* Read the header section */

	/* animVersion */
	buffer = engineFileReadWord (animFile);
	while ((buffer != NULL) && !engineUtilStringsMatch (buffer, (EtByte *)"animVersion")) {
		/* Eat the rest of the line */
		while (engineFileReadByte (animFile, buffer) && (buffer[0] >= ' ')) {}
		buffer = engineFileReadWord (animFile);
	}
	if (buffer == NULL) {
		continueReading = kEngineFALSE;
	}
	else {
		animVersion = engineFileReadFloat (animFile);
	}

	/* mayaVersion */
	buffer = engineFileReadWord (animFile);
	if (engineUtilStringsMatch (buffer, (EtByte *)"mayaVersion")) {
		mayaVersion = engineFileReadFloat (animFile);
		buffer = engineFileReadWord (animFile);
	}

	/* timeUnit */
	while ((buffer != NULL) && !engineUtilStringsMatch (buffer, (EtByte *)"timeUnit")) {
		/* Eat the rest of the line */
		while (engineFileReadByte (animFile, buffer) && (buffer[0] >= ' ')) {}
		buffer = engineFileReadWord (animFile);
	}
	if (buffer == NULL) {
		continueReading = kEngineFALSE;
	}
	else {
		buffer = engineFileReadWord (animFile);
		if (buffer == NULL) {
			continueReading = kEngineFALSE;
		}
		else {
			if (engineUtilStringsMatch (buffer, (EtByte *)"film")) {
				frameRate = 24.0;
			}
			else if (engineUtilStringsMatch (buffer, (EtByte *)"ntsc")) {
				frameRate = 30.0;
			}
			else if (engineUtilStringsMatch (buffer, (EtByte *)"pal")) {
				frameRate = 25.0;
			}
			else if (engineUtilStringsMatch (buffer, (EtByte *)"game")) {
				frameRate = 15.0;
			}
		}
	}

	/* angularUnit */
	buffer = (continueReading ? engineFileReadWord (animFile) : NULL);
	while ((buffer != NULL) && !engineUtilStringsMatch (buffer, (EtByte *)"angularUnit")) {
		/* Eat the rest of the line */
		while (engineFileReadByte (animFile, buffer) && (buffer[0] >= ' ')) {}
		buffer = engineFileReadWord (animFile);
	}
	if (buffer == NULL) {
		continueReading = kEngineFALSE;
	}
	else {
		buffer = engineFileReadWord (animFile);
		if (buffer == NULL) {
			continueReading = kEngineFALSE;
		}
		else {
			if (engineUtilStringsMatch (buffer, (EtByte *)"deg")) {
				angularConversion = kDegRad;
			}
			else if (engineUtilStringsMatch (buffer, (EtByte *)"rad")) {
				angularConversion = 1.0;
			}
		}
	}

	/* anim, animData */
	while (continueReading) {
		buffer = engineFileReadWord (animFile);
		continueReading = (buffer != kEngineNULL);
		/* anim */
		if (engineUtilStringsMatch (buffer, (EtByte *)"anim")) {
			/* full attribute name */
			buffer = engineFileReadWord (animFile);
			continueReading = (buffer != kEngineNULL);
			if (continueReading) {
				/* short attribute name */
				buffer = engineFileReadWord (animFile);
				continueReading = (buffer != kEngineNULL);
				if (continueReading) {
					for (attributeLength = 0; buffer[attributeLength] != 0x00; attributeLength++) {}
					engineUtilCopyString (buffer, attributeBuffer);
					/* object name */
					buffer = engineFileReadWord (animFile);
					continueReading = (buffer != kEngineNULL);
					if (continueReading) {
						for (nameLength = 0; buffer[nameLength] != 0x00; nameLength++) {}
						engineUtilCopyString (buffer, nameBuffer);
					}
				}
				/* Eat the rest of the line */
				while (engineFileReadByte (animFile, buffer) && (buffer[0] >= ' ')) {}
			}
		}
		/* animData */
		else if (engineUtilStringsMatch (buffer, (EtByte *)"animData")) {
			/* set the default parameters */
			endOfCurve = kEngineFALSE;
			firstKey = kEngineNULL;
			lastKey = kEngineNULL;
			numKeys = 0;
			isWeighted = kEngineTRUE;
			preInfinity = kInfinityConstant;
			postInfinity = kInfinityConstant;
			unitConversion = 1.0;
			/* read the parameters for the animation curve */
			while (continueReading && !endOfCurve) {
				buffer = engineFileReadWord (animFile);
				continueReading = (buffer != kEngineNULL);
				if (!continueReading) break;
				if (buffer[0] == '}') {
					endOfCurve = kEngineTRUE;
				}
				else if (engineUtilStringsMatch (buffer, (EtByte *)"preInfinity")) {
					preInfinity = wordAsInfinityType (engineFileReadWord (animFile));
				}
				else if (engineUtilStringsMatch (buffer, (EtByte *)"postInfinity")) {
					postInfinity = wordAsInfinityType (engineFileReadWord (animFile));
				}
				else if (engineUtilStringsMatch (buffer, (EtByte *)"weighted")) {
					isWeighted = (engineFileReadInt (animFile) == 1);
				}
				else if (engineUtilStringsMatch (buffer, (EtByte *)"output")) {
					buffer = engineFileReadWord (animFile);
					continueReading = (buffer != kEngineNULL);
					if (!continueReading) break;
					if (engineUtilStringsMatch (buffer, (EtByte *)"angular")) {
						unitConversion = angularConversion;
					}
				}
				else if (engineUtilStringsMatch (buffer, (EtByte *)"keys")) {
					/* read the list of keys */
					endOfKeys = kEngineFALSE;
					while (continueReading && !endOfKeys) {
						buffer = engineFileReadWord (animFile);
						continueReading = (buffer != kEngineNULL);
						if (!continueReading) break;
						if (buffer[0] == '}') {
							endOfKeys = kEngineTRUE;
						}
						else if (buffer[0] != '{') {
							readKey = (EtReadKey *)engineUtilAllocate (sizeof (EtReadKey));
							readKey->time = (EtTime)atof ((const char *)buffer) / frameRate;
							readKey->value = (EtValue)engineFileReadFloat (animFile) * unitConversion;
							readKey->inTangentType = wordAsTangentType (engineFileReadWord (animFile));
							readKey->outTangentType = wordAsTangentType (engineFileReadWord (animFile));
							engineFileReadInt (animFile);
							engineFileReadInt (animFile);
							if (animVersion >= 1.1) {
								engineFileReadInt (animFile);
							}
							if (readKey->inTangentType == kTangentFixed) {
								readKey->inAngle = engineFileReadFloat (animFile) * angularConversion;
								weight = engineFileReadFloat (animFile); 
								readKey->inWeightX = weight / frameRate; 
								readKey->inWeightY = weight; 
							}
							if (readKey->outTangentType == kTangentFixed) {
								readKey->outAngle = engineFileReadFloat (animFile) * angularConversion;
								weight = engineFileReadFloat (animFile); 
								readKey->outWeightX = weight / frameRate; 
								readKey->outWeightY = weight; 
							}
							readKey->next = kEngineNULL;
							if (firstKey == kEngineNULL) {
								firstKey = readKey;
								lastKey = firstKey;
							}
							else {
								lastKey->next = readKey;
								lastKey = readKey;
							}
							numKeys++;
						}
					}
				}
				else {
					/* Eat the rest of the line */
					while (engineFileReadByte (animFile, buffer) && (buffer[0] >= ' ')) {}
				}
			}
			if (numKeys != 0) {
				/* assemble the animation curve and add it to our channel list */
				animCurve = assembleAnimCurve (
					firstKey,
					numKeys,
					isWeighted,
					kEngineFALSE
				);
				if (animCurve != NULL) {
					animCurve->preInfinity = preInfinity;
					animCurve->postInfinity = postInfinity;
					if (channel == kEngineNULL) {
						lastChannel = (EtChannel *)engineUtilAllocate (sizeof (EtChannel));
						channel = lastChannel;
					}
					else {
						lastChannel->next = (EtChannel *)engineUtilAllocate (sizeof (EtChannel));
						lastChannel = lastChannel->next;
					}
					lastChannel->channel = engineUtilAllocate (nameLength + attributeLength + 2);
					engineUtilCopyString (nameBuffer, lastChannel->channel);
					lastChannel->channel[nameLength] = '.';
					engineUtilCopyString (attributeBuffer, lastChannel->channel + nameLength + 1);
					lastChannel->curve = animCurve;
					lastChannel->next = kEngineNULL;
					if (numCurves != kEngineNULL) {
						*numCurves = *numCurves + 1;
					}
				}
			}
		}
		else if (continueReading) {
			/* Eat the rest of the line */
			while (engineFileReadByte (animFile, buffer) && (buffer[0] >= ' ')) {}
		}
	}

	/* We are done with the file */
	engineFileClose (animFile);
	return (channel);
}

/*
//	Function Name:
//		engineAnimFreeChannelList
//
//	Description:
//		A helper function to free the memory used by a channel list
//
//  Input Arguments:
//		EtChannel *channelList		The channel list to free
//
//  Return Value:
//      None
*/
EtVoid
engineAnimFreeChannelList (EtChannel *channelList)
{
	EtChannel *channel;
	EtChannel *next;

	/* make sure we have something to free */
	if (channelList == kEngineNULL) {
		return;
	}

	channel = channelList;
	while (channel != kEngineNULL) {
		next = channel->next;
		if (channel->channel != kEngineNULL) {
			engineUtilFree ((EtByte *)(channel->channel));
		}
		if (channel->curve != kEngineNULL) {
			engineUtilFree ((EtByte *)(channel->curve));
		}
		engineUtilFree ((EtByte *)channel);
		channel = next;
	}
}

/*
//	The parameterisation functions used for weighted curves need to know
//	the floating point tolerance on the machine.  The functions below
//	help compute that tolerance
*/

/*
// statics for sMachineTolerance computation
*/
static EtValue sMachineTolerance;

static EtInt
dbl_gt (EtValue *a, EtValue *b)
{
	return (*a > *b ? 1 : 0);
}

static EtVoid
dbl_mult (EtValue *a, EtValue *b, EtValue *atimesb)
{
	EtValue product = (*a) * (*b);
	*atimesb = product;
}

static EtVoid
dbl_add (EtValue *a, EtValue *b, EtValue *aplusb)
{
	EtValue sum = (*a) + (*b);
	*aplusb = sum;
}

static EtVoid
init_tolerance ()
/*
// Description:
//		Init the machine tolerance, sMachineTolerance, is defined to be the
//		double that satisfies:
//		(1)  sMachineTolerance = 2^(k), for some integer k;
//		(2*) (1.0 + sMachineTolerance > 1.0) is TRUE;
//		(3*) (1.0 + sMachineTolerance/2.0 == 1.0) is TRUE.
//		(*)  When NO floating point optimization is used.
//
//		To foil floating point optimizers, sMachineTolerance must be
//		computed using dbl_mult(), dbl_add() and dbl_gt().
*/
{
    EtValue one, half, sum;
    one  = 1.0;
    half = 0.5;
    sMachineTolerance = 1.0;
    do {
		dbl_mult (&sMachineTolerance, &half, &sMachineTolerance);
		dbl_add (&sMachineTolerance, &one, &sum);
    } while (dbl_gt (&sum, &one));
	sMachineTolerance = 2.0 * sMachineTolerance;
}

/*
//	Description:
//		We want to ensure that (x1, x2) is inside the ellipse
//		(x1^2 + x2^2 - 2(x1 +x2) + x1*x2 + 1) given that we know
//		x1 is within the x bounds of the ellipse.
*/
static EtVoid
constrainInsideBounds (EtValue *x1, EtValue *x2)
{
	EtValue b, c,  discr,  root;

	if ((*x1 + sMachineTolerance) < kFourThirds) {
		b = *x1 - 2.0;
		c = *x1 - 1.0;
		discr = sqrt (b * b - 4 * c * c);
		root = (-b + discr) * 0.5;
		if ((*x2 + sMachineTolerance) > root) {
			*x2 = root - sMachineTolerance;
		}
		else {
			root = (-b - discr) * 0.5;
			if (*x2 < (root + sMachineTolerance)) {
				*x2 = root + sMachineTolerance;
			}
		}
	}
	else {
		*x1 = kFourThirds - sMachineTolerance;
		*x2 = kOneThird - sMachineTolerance;
	}
}

/*
//	Description:
//
//		Given the bezier curve
//			 B(t) = [t^3 t^2 t 1] * | -1  3 -3  1 | * | 0  |
//									|  3 -6  3  0 |   | x1 |
//									| -3  3  0  0 |   | x2 |
//									|  1  0  0  0 |   | 1  |
//
//		We want to ensure that the B(t) is a monotonically increasing function.
//		We can do this by computing
//			 B'(t) = [3t^2 2t 1 0] * | -1  3 -3  1 | * | 0  |
//									 |  3 -6  3  0 |   | x1 |
//									 | -3  3  0  0 |   | x2 |
//									 |  1  0  0  0 |   | 1  |
//
//		and finding the roots where B'(t) = 0.  If there is at most one root
//		in the interval [0, 1], then the curve B(t) is monotonically increasing.
//
//		It is easier if we use the control vector [ 0 x1 (1-x2) 1 ] since
//		this provides more symmetry, yields better equations and constrains
//		x1 and x2 to be positive.
//
//		Therefore:
//			 B'(t) = [3t^2 2t 1 0] * | -1  3 -3  1 | * | 0    |
//									 |  3 -6  3  0 |   | x1   |
//									 | -3  3  0  0 |   | 1-x2 |
//									 |  1  0  0  0 |   | 1    |
//
//				   = [t^2 t 1 0] * | 3*(3*x1 + 3*x2 - 2)  |
//								   | 2*(-6*x1 - 3*x2 + 3) |
//								   | 3*x1                 |
//								   | 0                    |
//
//		gives t = (2*x1 + x2 -1) +/- sqrt(x1^2 + x2^2 + x1*x2 - 2*(x1 + x2) + 1)
//				  --------------------------------------------------------------
//								3*x1 + 3* x2 - 2
//
//		If the ellipse [x1^2 + x2^2 + x1*x2 - 2*(x1 + x2) + 1] <= 0, (Note
//		the symmetry) x1 and x2 are valid control values and the curve is
//		monotonic.  Otherwise, x1 and x2 are invalid and have to be projected
//		onto the ellipse.
//
//		It happens that the maximum value that x1 or x2 can be is 4/3.
//		If one of the values is less than 4/3, we can determine the
//		boundary constraints for the other value.
*/
static EtVoid
checkMonotonic (EtValue *x1, EtValue *x2)
{
	EtValue d;

	/*
	// We want a control vector of [ 0 x1 (1-x2) 1 ] since this provides
	// more symmetry. (This yields better equations and constrains x1 and x2
	// to be positive.)
	*/
	*x2 = 1.0 - *x2;

	/* x1 and x2 must always be positive */
	if (*x1 < 0.0) *x1 = 0.0;
	if (*x2 < 0.0) *x2 = 0.0;

	/*
	// If x1 or x2 are greater than 1.0, then they must be inside the
	// ellipse (x1^2 + x2^2 - 2(x1 +x2) + x1*x2 + 1).
	// x1 and x2 are invalid if x1^2 + x2^2 - 2(x1 +x2) + x1*x2 + 1 > 0.0
	*/
	if ((*x1 > 1.0) || (*x2 > 1.0)) {
		d = *x1 * (*x1 - 2.0 + *x2) + *x2 * (*x2 - 2.0) + 1.0;
		if ((d + sMachineTolerance) > 0.0) {
			constrainInsideBounds (x1, x2);
		}
	}

	*x2 = 1.0 - *x2;
}

/*
//	Description:
//		Convert the control values for a polynomial defined in the Bezier
//		basis to a polynomial defined in the power basis (t^3 t^2 t 1).
*/
static EtVoid
bezierToPower (
	EtValue a1, EtValue b1, EtValue c1, EtValue d1,
	EtValue *a2, EtValue *b2, EtValue *c2, EtValue *d2)
{
	EtValue a = b1 - a1;
	EtValue b = c1 - b1;
	EtValue c = d1 - c1;
	EtValue d = b - a;
	*a2 = c - b - d;
	*b2 = d + d + d;
	*c2 = a + a + a;
	*d2 = a1;
}

/*
//   Evaluate a polynomial in array form ( value only )
//   input:
//      P               array 
//      deg             degree
//      s               parameter
//   output:
//      ag_horner1      evaluated polynomial
//   process: 
//      ans = sum (i from 0 to deg) of P[i]*s^i
//   restrictions: 
//      deg >= 0           
*/
static EtValue
ag_horner1 (EtValue P[], EtInt deg, EtValue s)
{
	EtValue h = P[deg];
	while (--deg >= 0) h = (s * h) + P[deg];
	return (h);
}

typedef struct ag_polynomial {
	EtValue *p;
	EtInt deg;
} AG_POLYNOMIAL;

/*
//	Description
//   Compute parameter value at zero of a function between limits
//       with function values at limits
//   input:
//       a, b      real interval
//       fa, fb    double values of f at a, b
//       f         real valued function of t and pars
//       tol       tolerance
//       pars      pointer to a structure
//   output:
//       ag_zeroin2   a <= zero of function <= b
//   process:
//       We find the zeroes of the function f(t, pars).  t is
//       restricted to the interval [a, b].  pars is passed in as
//       a pointer to a structure which contains parameters
//       for the function f.
//   restrictions:
//       fa and fb are of opposite sign.
//       Note that since pars comes at the end of both the
//       call to ag_zeroin and to f, it is an optional parameter.
*/
static EtValue
ag_zeroin2 (EtValue a, EtValue b, EtValue fa, EtValue fb, EtValue tol, AG_POLYNOMIAL *pars)
{
	EtInt test;
	EtValue c, d, e, fc, del, m, machtol, p, q, r, s;

	/* initialization */
	machtol = sMachineTolerance;

	/* start iteration */
label1:
	c = a;  fc = fa;  d = b-a;  e = d;
label2:
	if (fabs(fc) < fabs(fb)) {
		a = b;   b = c;   c = a;   fa = fb;   fb = fc;   fc = fa;
	}

	/* convergence test */
	del = 2.0 * machtol * fabs(b) + 0.5*tol;
	m = 0.5 * (c - b);
	test = ((fabs(m) > del) && (fb != 0.0));
	if (test) {
		if ((fabs(e) < del) || (fabs(fa) <= fabs(fb))) {
			/* bisection */
			d = m;  e= d;
		}
		else {
			s = fb / fa;
			if (a == c) {
				/* linear interpolation */
				p = 2.0*m*s;    q = 1.0 - s;
			}
			else {
				/* inverse quadratic interpolation */
				q = fa/fc;
				r = fb/fc;
				p = s*(2.0*m*q*(q-r)-(b-a)*(r-1.0));
				q = (q-1.0)*(r-1.0)*(s-1.0);
			}
			/* adjust the sign */
			if (p > 0.0) q = -q;  else p = -p;
			/* check if interpolation is acceptable */
			s = e;   e = d;
			if ((2.0*p < 3.0*m*q-fabs(del*q))&&(p < fabs(0.5*s*q))) {
				d = p/q;
			}
			else {
				d = m;	e = d;
			}
		}
		/* complete step */
		a = b;	fa = fb;
		if ( fabs(d) > del )   b += d;
		else if (m > 0.0) b += del;  else b -= del;
		fb = ag_horner1 (pars->p, pars->deg, b);
		if (fb*(fc/fabs(fc)) > 0.0 ) {
			goto label1;
		}
		else {
			goto label2;
		}
	}
	return (b);
}

/*
//	Description:
//   Compute parameter value at zero of a function between limits
//   input:
//       a, b            real interval
//       f               real valued function of t and pars
//       tol             tolerance
//       pars            pointer to a structure
//   output:
//       ag_zeroin       zero of function
//   process:
//       Call ag_zeroin2 to find the zeroes of the function f(t, pars).
//       t is restricted to the interval [a, b].
//       pars is passed in as a pointer to a structure which contains
//       parameters for the function f.
//   restrictions:
//       f(a) and f(b) are of opposite sign.
//       Note that since pars comes at the end of both the
//         call to ag_zeroin and to f, it is an optional parameter.
//       If you already have values for fa,fb use ag_zeroin2 directly
*/
static EtValue
ag_zeroin (EtValue a, EtValue b, EtValue tol, AG_POLYNOMIAL *pars)
{
	EtValue fa, fb;

	fa = ag_horner1 (pars->p, pars->deg, a);
	if (fabs(fa) < sMachineTolerance) return(a);

	fb = ag_horner1 (pars->p, pars->deg, b);
	if (fabs(fb) < sMachineTolerance) return(b);

	return (ag_zeroin2 (a, b, fa, fb, tol, pars));
} 

/*
// Description:
//   Find the zeros of a polynomial function on an interval
//   input:
//       Poly                 array of coefficients of polynomial
//       deg                  degree of polynomial
//       a, b                 interval of definition a < b
//       a_closed             include a in interval (TRUE or FALSE)
//       b_closed             include b in interval (TRUE or FALSE)
//   output: 
//       polyzero             number of roots 
//                            -1 indicates Poly == 0.0
//       Roots                zeroes of the polynomial on the interval
//   process:
//       Find all zeroes of the function on the open interval by 
//       recursively finding all of the zeroes of the derivative
//       to isolate the zeroes of the function.  Return all of the 
//       zeroes found adding the end points if the corresponding side
//       of the interval is closed and the value of the function 
//       is indeed 0 there.
//   restrictions:
//       The polynomial p is simply an array of deg+1 doubles.
//       p[0] is the constant term and p[deg] is the coef 
//       of t^deg.
//       The array roots should be dimensioned to deg+2. If the number
//       of roots returned is greater than deg, suspect numerical
//       instabilities caused by some nearly flat portion of Poly.
*/
static EtInt
polyZeroes (EtValue Poly[], EtInt deg, EtValue a, EtInt a_closed, EtValue b, EtInt b_closed, EtValue Roots[])
{
	EtInt i, left_ok, right_ok, nr, ndr, skip;
	EtValue e, f, s, pe, ps, tol, *p, p_x[22], *d, d_x[22], *dr, dr_x[22];
	AG_POLYNOMIAL ply;

	e = pe = 0.0;  
	f = 0.0;

	for (i = 0 ; i < deg + 1; ++i) {
		f += fabs(Poly[i]);
	}
	tol = (fabs(a) + fabs(b))*(deg+1)*sMachineTolerance;

	/* Zero polynomial to tolerance? */
	if (f <= tol)  return(-1);

	p = p_x;  d = d_x;  dr = dr_x;
	for (i = 0 ; i < deg + 1; ++i) {
		p[i] = 1.0/f * Poly[i];
	}

	/* determine true degree */
	while ( fabs(p[deg]) < tol) deg--;

	/* Identically zero poly already caught so constant fn != 0 */
	nr = 0;
	if (deg == 0) return (nr);

	/* check for linear case */
	if (deg == 1) {
		Roots[0] = -p[0] / p[1];
		left_ok  = (a_closed) ? (a<Roots[0]+tol) : (a<Roots[0]-tol);
		right_ok = (b_closed) ? (b>Roots[0]-tol) : (b>Roots[0]+tol);
		nr = (left_ok && right_ok) ? 1 : 0;
		if (nr) {
			if (a_closed && Roots[0]<a) Roots[0] = a;
			else if (b_closed && Roots[0]>b) Roots[0] = b;
		}
		return (nr);
	}
	/* handle non-linear case */
	else {
		ply.p = p;  ply.deg = deg;

		/* compute derivative */
		for (i=1; i<=deg; i++) d[i-1] = i*p[i];

		/* find roots of derivative */
		ndr = polyZeroes ( d, deg-1, a, 0, b, 0, dr );
		if (ndr == -1) return (0);

		/* find roots between roots of the derivative */
		for (i=skip=0; i<=ndr; i++) {
			if (nr>deg) return (nr);
			if (i==0) {
				s=a; ps = ag_horner1( p, deg, s);
				if ( fabs(ps)<=tol && a_closed) Roots[nr++]=a;
			}
			else { s=e; ps=pe; }
			if (i==ndr) { e = b; skip = 0;}
			else e=dr[i];
			pe = ag_horner1( p, deg, e );
			if (skip) skip = 0;
			else {
				if ( fabs(pe) < tol ) {
					if (i!=ndr || b_closed) {
						Roots[nr++] = e;
						skip = 1;
					}
				}
				else if ((ps<0 && pe>0)||(ps>0 && pe<0)) {
					Roots[nr++] = ag_zeroin(s, e, 0.0, &ply );
					if ((nr>1) && Roots[nr-2]>=Roots[nr-1]-tol) { 
						Roots[nr-2] = (Roots[nr-2]+Roots[nr-1]) * 0.5;
						nr--;
					}
				}
			}
		}
	}

	return (nr);
} 

/*
//	Description:
//		Create a constrained single span cubic 2d bezier curve using the
//		specified control points.  The curve interpolates the first and
//		last control point.  The internal two control points may be
//		adjusted to ensure that the curve is monotonic.
*/
static EtVoid
engineBezierCreate (EtCurve *animCurve, EtValue x[4], EtValue y[4])
{
	static EtBoolean sInited = kEngineFALSE;
	EtValue rangeX, dx1, dx2, nX1, nX2, oldX1, oldX2;

	if (!sInited) {
		init_tolerance ();
		sInited = kEngineTRUE;
	}

	if (animCurve == kEngineNULL) {
		return;
	}
	rangeX = x[3] - x[0];
	if (rangeX == 0.0) {
		return;
	}
	dx1 = x[1] - x[0];
	dx2 = x[2] - x[0];

	/* normalize X control values */
	nX1 = dx1 / rangeX;
	nX2 = dx2 / rangeX;

	/* if all 4 CVs equally spaced, polynomial will be linear */
	if ((nX1 == kOneThird) && (nX2 == kTwoThirds)) {
		animCurve->isLinear = kEngineTRUE;
	} else {
		animCurve->isLinear = kEngineFALSE;
	}

	/* save the orig normalized control values */
	oldX1 = nX1;
	oldX2 = nX2;

	/*
	// check the inside control values yield a monotonic function.
	// if they don't correct them with preference given to one of them.
	//
	// Most of the time we are monotonic, so do some simple checks first
	*/
	if (nX1 < 0.0) nX1 = 0.0;
	if (nX2 > 1.0) nX2 = 1.0;
	if ((nX1 > 1.0) || (nX2 < -1.0)) {
		checkMonotonic (&nX1, &nX2);
	}

	/* compute the new control points */
	if (nX1 != oldX1) {
		x[1] = x[0] + nX1 * rangeX;
		if (oldX1 != 0.0) {
			y[1] = y[0] + (y[1] - y[0]) * nX1 / oldX1;
		}
	}
	if (nX2 != oldX2) {
		x[2] = x[0] + nX2 * rangeX;
		if (oldX2 != 1.0) {
			y[2] = y[3] - (y[3] - y[2]) * (1.0 - nX2) / (1.0 - oldX2);
		}
	}

	/* save the control points */
	animCurve->fX1 = x[0];
	animCurve->fX4 = x[3];

	/* convert Bezier basis to power basis */
	bezierToPower (
		0.0, nX1, nX2, 1.0,
		&(animCurve->fCoeff[3]), &(animCurve->fCoeff[2]), &(animCurve->fCoeff[1]), &(animCurve->fCoeff[0])
	);
	bezierToPower (
		y[0], y[1], y[2], y[3],
		&(animCurve->fPolyY[3]), &(animCurve->fPolyY[2]), &(animCurve->fPolyY[1]), &(animCurve->fPolyY[0])
	);
}

/*
// Description:
//		Given the time between fX1 and fX4, return the
//		value of the curve at that time.
*/
static EtValue
engineBezierEvaluate (EtCurve *animCurve, EtTime time)
{
	EtValue t, s, poly[4], roots[5];
	EtInt numRoots;

	if (animCurve == kEngineNULL) {
		return (0.0);
	}

	if (animCurve->fX1 == time) {
		s = 0.0;
	}
	else if (animCurve->fX4 == time) {
		s = 1.0;
	}
	else {
		s = (time - animCurve->fX1) / (animCurve->fX4 - animCurve->fX1);
	}

	if (animCurve->isLinear) {
		t = s;
	}
	else {
		poly[3] = animCurve->fCoeff[3];
		poly[2] = animCurve->fCoeff[2];
		poly[1] = animCurve->fCoeff[1];
		poly[0] = animCurve->fCoeff[0] - s;

		numRoots = polyZeroes (poly, 3, 0.0, 1, 1.0, 1, roots);
		if (numRoots == 1) {
			t = roots[0];
		}
		else {
			t = 0.0;
		}
	}
	return (t * (t * (t * animCurve->fPolyY[3] + animCurve->fPolyY[2]) + animCurve->fPolyY[1]) + animCurve->fPolyY[0]);
}

static EtVoid
engineHermiteCreate (EtCurve *animCurve, EtValue x[4], EtValue y[4])
{
	EtValue dx, dy, tan_x, m1, m2, length, d1, d2;

	if (animCurve == kEngineNULL) {
		return;
	}

	/* save the control points */
	animCurve->fX1 = x[0];

	/*	
	 *	Compute the difference between the 2 keyframes.					
	 */
	dx = x[3] - x[0];
	dy = y[3] - y[0];

	/* 
	 * 	Compute the tangent at the start of the curve segment.			
	 */
	tan_x = x[1] - x[0];
	m1 = m2 = 0.0;
	if (tan_x != 0.0) {
		m1 = (y[1] - y[0]) / tan_x;
	}

	tan_x = x[3] - x[2];
	if (tan_x != 0.0) {
		m2 = (y[3] - y[2]) / tan_x;
	}

	length = 1.0 / (dx * dx);
	d1 = dx * m1;
	d2 = dx * m2;
	animCurve->fCoeff[0] = (d1 + d2 - dy - dy) * length / dx;
	animCurve->fCoeff[1] = (dy + dy + dy - d1 - d1 - d2) * length;
	animCurve->fCoeff[2] = m1;
	animCurve->fCoeff[3] = y[0];
}

/*
// Description:
//		Given the time between fX1 and fX2, return the function
//		value of the curve
*/
static EtValue
engineHermiteEvaluate (EtCurve *animCurve, EtTime time)
{
	EtValue t;
	if (animCurve == kEngineNULL) {
		return (0.0);
	}
	t = time - animCurve->fX1;
	return (t * (t * (t * animCurve->fCoeff[0] + animCurve->fCoeff[1]) + animCurve->fCoeff[2]) + animCurve->fCoeff[3]);
}

/*
//	Function Name:
//		evaluateInfinities
//
//	Description:
//		A static helper function to evaluate the infinity portion of an
//	animation curve.  The infinity portion is the parts of the animation
//	curve outside the range of keys.
//
//  Input Arguments:
//		EtCurve *animCurve			The animation curve to evaluate
//		EtTime time					The time (in seconds) to evaluate
//		EtBoolean evalPre
//			kEngineTRUE				evaluate the pre-infinity portion
//			kEngineFALSE			evaluate the post-infinity portion
//
//  Return Value:
//		EtValue value				The evaluated value of the curve at time
*/
static EtValue
evaluateInfinities (EtCurve *animCurve, EtTime time, EtBoolean evalPre)
{
	EtValue value = 0.0;
	EtValue	valueRange;
	EtTime	factoredTime, firstTime, lastTime, timeRange;
	EtFloat	remainder, tanX, tanY;
	double numCycles, notUsed;

	/* make sure we have something to evaluate */
	if ((animCurve == kEngineNULL) || (animCurve->numKeys == 0)) {
		return (value);
	}

	/* find the number of cycles of the base animation curve */
	firstTime = animCurve->keyList[0].time;
	lastTime = animCurve->keyList[animCurve->numKeys - 1].time;
	timeRange = lastTime - firstTime;
	if (timeRange == 0.0) {
		/*
		// Means that there is only one key in the curve.. Return the value
		// of that key..
		*/
		return (animCurve->keyList[0].value);
	}
	if (time > lastTime) {
		remainder = fabs (modf ((time - lastTime) / timeRange, &numCycles));
	}
	else {
		remainder = fabs (modf ((time - firstTime) / timeRange, &numCycles));
	}
	factoredTime = timeRange * remainder;
	numCycles = fabs (numCycles) + 1;

	if (evalPre) {
		/* evaluate the pre-infinity */
		if (animCurve->preInfinity == kInfinityOscillate) {
			if ((remainder = modf (numCycles / 2.0, &notUsed)) != 0.0) {
				factoredTime = firstTime + factoredTime;
			}
			else {
				factoredTime = lastTime - factoredTime;
			}
		}
		else if ((animCurve->preInfinity == kInfinityCycle)
		||	(animCurve->preInfinity == kInfinityCycleRelative)) {
			factoredTime = lastTime - factoredTime;
		}
		else if (animCurve->preInfinity == kInfinityLinear) {
			factoredTime = firstTime - time;
			tanX = animCurve->keyList[0].inTanX;
			tanY = animCurve->keyList[0].inTanY;
			value = animCurve->keyList[0].value;
			if (tanX != 0.0) {
				value -= ((factoredTime * tanY) / tanX);
			}
			return (value);
		}
	}
	else {
		/* evaluate the post-infinity */
		if (animCurve->postInfinity == kInfinityOscillate) {
			if ((remainder = modf (numCycles / 2.0, &notUsed)) != 0.0) {
				factoredTime = lastTime - factoredTime;
			}
			else {
				factoredTime = firstTime + factoredTime;
			}
		}
		else if ((animCurve->postInfinity == kInfinityCycle)
		||	(animCurve->postInfinity == kInfinityCycleRelative)) {
			factoredTime = firstTime + factoredTime;
		}
		else if (animCurve->postInfinity == kInfinityLinear) {
			factoredTime = time - lastTime;
			tanX = animCurve->keyList[animCurve->numKeys - 1].outTanX;
			tanY = animCurve->keyList[animCurve->numKeys - 1].outTanY;
			value = animCurve->keyList[animCurve->numKeys - 1].value;
			if (tanX != 0.0) {
				value += ((factoredTime * tanY) / tanX);
			}
			return (value);
		}
	}

	value = engineAnimEvaluate (animCurve, factoredTime);

	/* Modify the value if infinityType is cycleRelative */
	if (evalPre && (animCurve->preInfinity == kInfinityCycleRelative)) {
		valueRange = animCurve->keyList[animCurve->numKeys - 1].value -
						animCurve->keyList[0].value;
		value -= (numCycles * valueRange);
	}
	else if (!evalPre && (animCurve->postInfinity == kInfinityCycleRelative)) {
		valueRange = animCurve->keyList[animCurve->numKeys - 1].value -
						animCurve->keyList[0].value;
		value += (numCycles * valueRange);
	}
	return (value);
}

/*
//	Function Name:
//		find
//
//	Description:
//		A static helper method to find a key prior to a specified time
//
//  Input Arguments:
//		EtCurve *animCurve			The animation curve to search
//		EtTime time					The time (in seconds) to find
//		EtInt *index				The index of the key prior to time
//
//  Return Value:
//      EtBoolean result
//			kEngineTRUE				time is represented by an actual key
//										(with the index in index)
//			kEngineFALSE			the index key is the key less than time
//
//	Note:
//		keys are sorted by ascending time, which means we can use a binary
//	search to find the key
*/
static EtBoolean
find (EtCurve *animCurve, EtTime time, EtInt *index)
{
	EtInt len, mid, low, high;

	/* make sure we have something to search */
	if ((animCurve == kEngineNULL) || (index == kEngineNULL)) {
		return (kEngineFALSE);
	}

	/* use a binary search to find the key */
	*index = 0;
	len = animCurve->numKeys;
	if (len > 0) {
		low = 0;
		high = len - 1;
		do {
			mid = (low + high) >> 1;
			if (time < animCurve->keyList[mid].time) {
				high = mid - 1;			/* Search lower half */
			} else if (time > animCurve->keyList[mid].time) {
				low  = mid + 1;			/* Search upper half */
			}
			else {
				*index = mid;	/* Found item! */
				return (kEngineTRUE);
			}
		} while (low <= high);
		*index = low;
	}
	return (kEngineFALSE);
}

/*
//	Function Name:
//		engineAnimEvaluate
//
//	Description:
//		A function to evaluate an animation curve at a specified time
//
//  Input Arguments:
//		EtCurve *animCurve			The animation curve to evaluate
//		EtTime time					The time (in seconds) to evaluate
//
//  Return Value:
//		EtValue value				The evaluated value of the curve at time
*/
EtValue
engineAnimEvaluate (EtCurve *animCurve, EtTime time)
{
	EtBoolean withinInterval = kEngineFALSE;
	EtKey *nextKey;
	EtInt index;
	EtValue value = 0.0;
	EtValue x[4];
	EtValue y[4];

	/* make sure we have something to evaluate */
	if ((animCurve == kEngineNULL) || (animCurve->numKeys == 0)) {
		return (value);
	}

	/* check if the time falls into the pre-infinity */
	if (time < animCurve->keyList[0].time) {
		if (animCurve->preInfinity == kInfinityConstant) {
			return (animCurve->keyList[0].value);
		}
		return (evaluateInfinities (animCurve, time, kEngineTRUE));
	}

	/* check if the time falls into the post-infinity */
	if (time > animCurve->keyList[animCurve->numKeys - 1].time) {
		if (animCurve->postInfinity == kInfinityConstant) {
			return (animCurve->keyList[animCurve->numKeys - 1].value);
		}
		return (evaluateInfinities (animCurve, time, kEngineFALSE));
	}

	/* check if the animation curve is static */
	if (animCurve->isStatic) {
		return (animCurve->keyList[0].value);
	}

	/* check to see if the time falls within the last segment we evaluated */
	if (animCurve->lastKey != kEngineNULL) {
		if ((animCurve->lastIndex < (animCurve->numKeys - 1))
		&&	(time > animCurve->lastKey->time)) {
			nextKey = &(animCurve->keyList[animCurve->lastIndex + 1]);
			if (time == nextKey->time) {
				animCurve->lastKey = nextKey;
				animCurve->lastIndex++;
				return (animCurve->lastKey->value);
			}
			if (time < nextKey->time ) {
				index = animCurve->lastIndex + 1;
				withinInterval = kEngineTRUE;
			}
		}
		else if ((animCurve->lastIndex > 0)
			&&	(time < animCurve->lastKey->time)) {
			nextKey = &(animCurve->keyList[animCurve->lastIndex - 1]);
			if (time > nextKey->time) {
				index = animCurve->lastIndex;
				withinInterval = kEngineTRUE;
			}
			if (time == nextKey->time) {
				animCurve->lastKey = nextKey;
				animCurve->lastIndex--;
				return (animCurve->lastKey->value);
			}
		}
	}

	/* it does not, so find the new segment */
	if (!withinInterval) {
		if (find (animCurve, time, &index) || (index == 0)) {
			/*
			//	Exact match or before range of this action,
			//	return exact keyframe value.
			*/
			animCurve->lastKey = &(animCurve->keyList[index]);
			animCurve->lastIndex = index;
			return (animCurve->lastKey->value);
		}
		else if (index == animCurve->numKeys) {
			/* Beyond range of this action return end keyframe value */
			animCurve->lastKey = &(animCurve->keyList[0]);
			animCurve->lastIndex = 0;
			return (animCurve->keyList[animCurve->numKeys - 1].value);
		}
	}

	/* if we are in a new segment, pre-compute and cache the bezier parameters */
	if (animCurve->lastInterval != (index - 1)) {
		animCurve->lastInterval = index - 1;
		animCurve->lastIndex = animCurve->lastInterval;
		animCurve->lastKey = &(animCurve->keyList[animCurve->lastInterval]);
		if ((animCurve->lastKey->outTanX == 0.0)
		&&	(animCurve->lastKey->outTanY == 0.0)) {
			animCurve->isStep = kEngineTRUE;
		}
		else if ((animCurve->lastKey->outTanX == kEngineFloatMax)
		&&	(animCurve->lastKey->outTanY == kEngineFloatMax)) {
			animCurve->isStepNext = kEngineTRUE;
			nextKey = &(animCurve->keyList[index]);
		}
		else {
			animCurve->isStep = kEngineFALSE;
			animCurve->isStepNext = kEngineFALSE;
			x[0] = animCurve->lastKey->time;
			y[0] = animCurve->lastKey->value;
			x[1] = x[0] + (animCurve->lastKey->outTanX * kOneThird);
			y[1] = y[0] + (animCurve->lastKey->outTanY * kOneThird);

			nextKey = &(animCurve->keyList[index]);
			x[3] = nextKey->time;
			y[3] = nextKey->value;
			x[2] = x[3] - (nextKey->inTanX * kOneThird);
			y[2] = y[3] - (nextKey->inTanY * kOneThird);

			if (animCurve->isWeighted) {
				engineBezierCreate (animCurve, x, y);
			}
			else {
				engineHermiteCreate (animCurve, x, y);
			}
		}
	}

	/* finally we can evaluate the segment */
	if (animCurve->isStep) {
		value = animCurve->lastKey->value;
	}
	else if (animCurve->isStepNext) {
		value = nextKey->value;
	}
	else if (animCurve->isWeighted) {
		value = engineBezierEvaluate (animCurve, time);
	}
	else {
		value = engineHermiteEvaluate (animCurve, time);
	}
	return (value);
}
