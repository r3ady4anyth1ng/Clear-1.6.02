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

#include <slopeShaderBehavior.h>
#include <maya/MGlobal.h>
#include <maya/MPlugArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MObjectArray.h>

slopeShaderBehavior::slopeShaderBehavior()
//
//	Description:
//		Constructor
//
{
}

slopeShaderBehavior::~slopeShaderBehavior()
//
//	Description:
//		Destructor
//
{
}

void *slopeShaderBehavior::creator()
//
//	Description:
//		Returns a new instance of this class
//
{
	return new slopeShaderBehavior;
}

bool slopeShaderBehavior::shouldBeUsedFor( MObject &sourceNode, MObject &destinationNode,
										   MPlug &sourcePlug, MPlug &destinationPlug)
//
//	Description:
//		Overloaded function from MPxDragAndDropBehavior
//	this method will return true if it is going to handle the connection
//	between the two nodes given.
//
{
	bool result = false;

	if(sourceNode.hasFn(MFn::kLambert))
	{
		//if the source node was a lambert
		//than we will check the downstream connections to see 
		//if a slope shader is assigned to it.
		//
		MObject shaderNode;
		MFnDependencyNode src(sourceNode);
		MPlugArray connections;

		src.getConnections(connections);
		for(unsigned i = 0; i < connections.length(); i++)
		{
			//check the incoming connections to this plug
			//
			MPlugArray connectedPlugs;
			connections[i].connectedTo(connectedPlugs, true, false);
			for(unsigned j = 0; j < connectedPlugs.length(); j++)
			{
				//if the incoming node is a slope shader than 
				//set shaderNode equal to it and break out of the inner 
				//loop
				//
				if(MFnDependencyNode(connectedPlugs[j].node()).typeName() == "slopeShader")
				{
					shaderNode = connectedPlugs[j].node();
					break;
				}
			}

			//if the shaderNode is not null
			//than we have found a slopeShader
			//
			if(!shaderNode.isNull())
			{
				//if the destination node is a mesh than we will take
				//care of this connection so set the result to true
				//and break out of the outer loop
				//
				if(destinationNode.hasFn(MFn::kMesh))
					result = true;

				break;
			}
		}
	}
	if(MFnDependencyNode(sourceNode).typeName() == "slopeShader")
	//if the sourceNode is a slope shader than check what we
	//are dropping on to
	//
	{
		if(destinationNode.hasFn(MFn::kLambert))
			result = true;
		else if(destinationNode.hasFn(MFn::kMesh))
			result = true;
	}	

	return result;
}

MStatus slopeShaderBehavior::connectNodeToNode( MObject &sourceNode, MObject &destinationNode, bool force )
//
//	Description:
//		Overloaded function from MPxDragAndDropBehavior
//	this method will handle the connection between the slopeShader and the shader it is
//	assigned to as well as any meshes that it is assigned to. 
//
{
	MStatus result = MS::kFailure;
	MFnDependencyNode src(sourceNode);

	//if we are dragging from a lambert
	//we want to check what we are dragging
	//onto.
	if(sourceNode.hasFn(MFn::kLambert))
	{
		MObject shaderNode;
		MPlugArray connections;
		MObjectArray shaderNodes;
		shaderNodes.clear();

		//if the source node was a lambert
		//than we will check the downstream connections to see 
		//if a slope shader is assigned to it.
		//
		src.getConnections(connections);
		unsigned i;
		for(i = 0; i < connections.length(); i++)
		{
			//check the incoming connections to this plug
			//
			MPlugArray connectedPlugs;
			connections[i].connectedTo(connectedPlugs, true, false);
			for(unsigned j = 0; j < connectedPlugs.length(); j++)
			{
				//if the incoming node is a slope shader than 
				//append the node to the shaderNodes array
				//
				MObject currentnode = connectedPlugs[j].node();
				if(MFnDependencyNode(currentnode).typeName() == "slopeShader")
				{
					shaderNodes.append(currentnode);
				}
			}
		}

		//if we found a shading node
		//than check the destination node 
		//type to see if it is a mesh
		//
		if(shaderNodes.length() > 0)
		{
			MFnDependencyNode dest(destinationNode);
			if(destinationNode.hasFn(MFn::kMesh))
			{
				//if the node is a mesh than for each slopeShader
				//connect the worldMesh attribute to the dirtyShaderPlug
				//attribute to force an evaluation of the node when the mesh
				//changes
				//
				for(i = 0; i < shaderNodes.length(); i++)
				{
					MPlug srcPlug = dest.findPlug("worldMesh");
					MPlug destPlug = MFnDependencyNode(shaderNodes[i]).findPlug("dirtyShaderPlug");

					if(!srcPlug.isNull() && !destPlug.isNull())
					{
						MString cmd = "connectAttr -na ";
						cmd += srcPlug.name() + " ";
						cmd += destPlug.name();
						MGlobal::executeCommand(cmd);
					}
				}

				//get the shading engine so we can assign the shader
				//to the mesh after doing the connection
				//
				MObject shadingEngine = findShadingEngine(sourceNode);

				//if there is a valid shading engine than make
				//the connection
				//
				if(!shadingEngine.isNull())
				{
					MString cmd = "sets -edit -forceElement ";
					cmd += MFnDependencyNode(shadingEngine).name() + " ";
					cmd += MFnDagNode(destinationNode).partialPathName();
					result = MGlobal::executeCommand(cmd);
				}
			}
		}
	}
	else if(src.typeName() == "slopeShader")
	//if we are dragging from a slope shader
	//than we want to see what we are dragging onto
	//
	{
		if(destinationNode.hasFn(MFn::kMesh))
		{
			//if the user is dragging onto a mesh
			//than make the connection from the worldMesh
			//to the dirtyShader plug on the slopeShader
			//
			MFnDependencyNode dest(destinationNode);
			MPlug srcPlug = dest.findPlug("worldMesh");
			MPlug destPlug = src.findPlug("dirtyShaderPlug");
			if(!srcPlug.isNull() && !destPlug.isNull())
			{
				MString cmd = "connectAttr -na ";
				cmd += srcPlug.name() + " ";
				cmd += destPlug.name();
				result = MGlobal::executeCommand(cmd);
			}
		}
	}

	return result;
}

MStatus slopeShaderBehavior::connectNodeToAttr( MObject &sourceNode, MPlug &destinationPlug, bool force )
//
//	Description:
//		Overloaded function from MPxDragAndDropBehavior
//	this method will assign the correct output from the slope shader 
//	onto the given attribute.
//
{
	MStatus result = MS::kFailure;
	MFnDependencyNode src(sourceNode);

	//if we are dragging from a slopeShader
	//to a shader than connect the outColor
	//plug to the plug being passed in
	//
	if(destinationPlug.node().hasFn(MFn::kLambert)) {
		if(src.typeName() == "slopeShader")
		{
			MPlug srcPlug = src.findPlug("outColor");
			if(!srcPlug.isNull() && !destinationPlug.isNull())
			{
				MString cmd = "connectAttr ";
				cmd += srcPlug.name() + " ";
				cmd += destinationPlug.name();
				result = MGlobal::executeCommand(cmd);
			}
		}
	} else {
		//in all of the other cases we do not need the plug just the node
		//that it is on
		//
        MObject destinationNode = destinationPlug.node();
		result = connectNodeToNode(sourceNode, destinationNode, force);
	}
        
	return result;
}

MObject slopeShaderBehavior::findShadingEngine(MObject &node)
//
//	Description:
//		Given the material MObject this method will
//	return the shading group that it is assigned to.
//	if there is no shading group associated with
//	the material than a null MObject is apssed back.
//
{
	MFnDependencyNode nodeFn(node);
	MPlug srcPlug = nodeFn.findPlug("outColor");
	MPlugArray nodeConnections;
	srcPlug.connectedTo(nodeConnections, false, true);
	//loop through the connections
	//and find the shading engine node that
	//it is connected to
	//
	for(unsigned i = 0; i < nodeConnections.length(); i++)
	{
		if(nodeConnections[i].node().hasFn(MFn::kShadingEngine))
			return nodeConnections[i].node();
	}

	//no shading engine associated so return a
	//null MObject
	//
	return MObject();
}

