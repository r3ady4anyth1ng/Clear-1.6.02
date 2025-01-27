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

///////////////////////////////////////////////////////////////////////////////
//
//    Description    :    Contrast shader plugin
//
//    Notes        :     The contrast operation consists in
//                redistributing the energy around the middle
//                point. In other words: every intensity below
//                0.5 is pushed towards black, and every
//                intensity above is pushed towards white.
//                The trick is to do that in a "smooth" way.
//
//        The way to do that is:
//            On interval [0,0.5] apply a gamma to intensity
//            On interval [0.5,1] apply an "inverted" gamma to intensity
//
//        That is:
//            When I<0.5,    newI=0.5*(2*I)^contrast
//            When I>=0.5,    newI=1-0.5*(2*(1-I))^contrast
//
//        This function is C1 on [0,1], and if contrast==1, it is the
//        identity.
//
//        The bias value stems from: why 0.5 and not something else ?
//        So, we use a function f to remap the [0,1] interval on itself
//        with the constraint:
//            f(0)=0
//            f(bias)=0.5
//            f(1)=1
//        and we compose with the above function.
//
//        Of course, when bias=0.5, f *has* to be the identity, and of
//        course, f *has* to be smooth.
//        A good candidate is f(x)=x^alpha, with alpha=log(0.5)/log(bias)
//
///////////////////////////////////////////////////////////////////////////////

#include <math.h>

#include <maya/MPxNode.h>
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MFnPlugin.h>

//
// DESCRIPTION:
///////////////////////////////////////////////////////
class Contrast:public MPxNode
{
	public:
                    Contrast();
    virtual         ~Contrast();

	virtual MStatus compute( const MPlug&, MDataBlock& );
	virtual void    postConstructor();

    static void    *creator();
    static MStatus initialize();

	//  Id tag for use with binary file format
    static MTypeId id;

	private:

	// Input attributes
	static MObject aColor;
	static MObject aContrast;
	static MObject aBias;

	// Output attributes
	static MObject aOutColor;
};

// static data
MTypeId Contrast::id( 0x81008 );

// Attributes
MObject Contrast::aColor;
MObject Contrast::aContrast;
MObject Contrast::aBias;

MObject Contrast::aOutColor;

#define MAKE_INPUT(attr)						\
    CHECK_MSTATUS(attr.setKeyable(true));  		\
	CHECK_MSTATUS(attr.setStorable(true));		\
    CHECK_MSTATUS(attr.setReadable(true)); 		\
	CHECK_MSTATUS(attr.setWritable(true));

#define MAKE_OUTPUT(attr)						\
    CHECK_MSTATUS(attr.setKeyable(false)); 		\
	CHECK_MSTATUS(attr.setStorable(false));		\
    CHECK_MSTATUS(attr.setReadable(true)); 		\
	CHECK_MSTATUS(attr.setWritable(false));

//
// DESCRIPTION:
///////////////////////////////////////////////////////
void Contrast::postConstructor( )
{
	setMPSafe(true);
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
Contrast::Contrast()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
Contrast::~Contrast()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
void *Contrast::creator()
{
    return new Contrast();
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus Contrast::initialize()
{
    MFnNumericAttribute    nAttr; 

    // Create input attributes

	aColor = nAttr.createColor("inputColor", "ic");
	MAKE_INPUT(nAttr);

	aContrast = nAttr.createColor("contrast", "c");
	MAKE_INPUT(nAttr);
	CHECK_MSTATUS(nAttr.setDefault(1.f, 1.f, 1.f));

	aBias = nAttr.createColor("bias", "b");
	MAKE_INPUT(nAttr);
	CHECK_MSTATUS(nAttr.setDefault(.5f, .5f, .5f));

	// Create output attributes
    aOutColor = nAttr.createColor("outColor", "oc");
	MAKE_OUTPUT(nAttr);

    /* Plug inputs and outputs
    ---------------------------*/
    CHECK_MSTATUS(addAttribute(aColor));
    CHECK_MSTATUS(addAttribute(aContrast));
    CHECK_MSTATUS(addAttribute(aBias));
    CHECK_MSTATUS(addAttribute(aOutColor));

    /* Build dependancies
    ----------------------*/
    CHECK_MSTATUS(attributeAffects(aColor,    aOutColor));
    CHECK_MSTATUS(attributeAffects(aContrast, aOutColor));
    CHECK_MSTATUS(attributeAffects(aBias,     aOutColor));

    return MS::kSuccess;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus Contrast::compute(const MPlug& plug, MDataBlock& block) 
{
	// outColor or individial R, G, B channel
    if((plug != aOutColor) && (plug.parent() != aOutColor))
		return MS::kUnknownParameter;

    MFloatVector resultColor;
    MFloatVector & col = block.inputValue(aColor).asFloatVector();
    MFloatVector & cont = block.inputValue(aContrast).asFloatVector();
    MFloatVector & bias = block.inputValue(aBias).asFloatVector();

    float xp;
    float logp5= logf(0.5F);

    xp = powf(col[0], logp5/logf(bias[0]));
    resultColor[0] = xp<0.5F ?
		0.5F*powf(2.0F*xp, cont[0]) : 
		1.0F-0.5F*powf(2.0F*(1.0F-xp), cont[0]);

    xp = powf(col[1], logp5/logf(bias[1]));
    resultColor[1] = xp<0.5F ?
		0.5F*powf(2.0F*xp, cont[1]) : 
		1.0F-0.5F*powf(2.0F*(1.0F-xp), cont[1]);

    xp = powf(col[2], logp5/logf(bias[2]));
    resultColor[2] = xp<0.5F ?
		0.5F*powf(2.0F*xp, cont[2]) : 
		1.0F-0.5F*powf(2.0F*(1.0F-xp), cont[2]);

	// set ouput color attribute
    MDataHandle outColorHandle = block.outputValue( aOutColor );
    MFloatVector& outColor = outColorHandle.asFloatVector();
    outColor = resultColor;
    outColorHandle.setClean();

    return MS::kSuccess;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{
	const MString UserClassify( "utility/color" );

	MFnPlugin plugin( obj, PLUGIN_COMPANY, "4.5", "Any");
	CHECK_MSTATUS( plugin.registerNode("contrastNode", Contrast::id, 
						Contrast::creator, Contrast::initialize,
						MPxNode::kDependNode, &UserClassify ) );

	return MS::kSuccess;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus uninitializePlugin( MObject obj )
{
	MFnPlugin plugin( obj );
	CHECK_MSTATUS( plugin.deregisterNode( Contrast::id ) );

	return MS::kSuccess;
}
