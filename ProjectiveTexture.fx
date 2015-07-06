//--------------------------------------------------------------------------------------
// File: Tutorial07.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D shaderTexture  : register( t0 );
Texture2D projectionTexture  : register( t1 );
Texture2D depthTexture  : register( t2 );
SamplerState SampleType : register( s0 );


cbuffer cbChangesEveryFrame : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
	matrix PTView;
	matrix PTProj;
	float3 projectorPos;
	float t;
	float3 projectorDir;
	float4 vMeshColor;
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 Pos : POSITION;
	float4 Normal : NORMAL0;
	float2 Tex : TEXCOORD0;
};

struct GSPS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL0;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL0;
	float2 Tex : TEXCOORD0;
	float4 viewPosition : NORMAL1;
	float4 viewPos : NORMAL2;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
GSPS_INPUT VS( VS_INPUT input )
{
    GSPS_INPUT output = (GSPS_INPUT)0;
    
    output.Pos = mul( float4(input.Pos), World );
    output.Normal = mul( input.Normal, (float3x3)World );
    output.Tex = input.Tex;
    
    return output;
}

//--------------------------------------------------------------------------------------
//Geometry Shader
//--------------------------------------------------------------------------------------
[maxvertexcount(9)]
void GS( triangle GSPS_INPUT input[3], inout TriangleStream<PS_INPUT> TriStream )
{
    PS_INPUT output;
    
    //
    // Calculate the face normal
    //
    float3 faceEdgeA = input[1].Pos - input[0].Pos;
    float3 faceEdgeB = input[2].Pos - input[0].Pos;
    float3 faceNormal = normalize( cross(faceEdgeA, faceEdgeB) );
    float3 ExplodeAmt = faceNormal*(sin(t)/3+0.1);
    
    //
    // Calculate the face center
    //
    float3 centerPos = (input[0].Pos.xyz + input[1].Pos.xyz + input[2].Pos.xyz)/3.0;
    float2 centerTex = (input[0].Tex + input[1].Tex + input[2].Tex)/3.0;
    centerPos += ExplodeAmt;
    
    //
    // Output the pyramid
    //
	
	

    for( int i=0; i<3; i++ )
    {
		float3 e1=input[i].Pos.xyz-centerPos;
		float3 e2=input[(i+1)%3].Pos.xyz-centerPos;
		float3 n=normalize(cross(e1,e2));
		output.Pos = float4(centerPos,1) ;
		output.Pos = mul( output.Pos, View );
		output.Pos = mul( output.Pos, Projection );
		output.viewPos = float4(centerPos,1);
		output.viewPosition = mul( output.viewPos,PTView );
		output.viewPosition = mul(  output.viewPosition,PTProj);
		output.Tex = centerTex;
		output.Normal = n;
		TriStream.Append( output );

		output.Pos = input[i].Pos ;
        output.Pos = mul( output.Pos, View );
        output.Pos = mul( output.Pos, Projection );
		output.viewPos = input[i].Pos;
		output.viewPosition = mul( output.viewPos,PTView );
		output.viewPosition = mul(  output.viewPosition,PTProj);
        output.Normal = n;
        output.Tex = input[i].Tex;
        TriStream.Append( output );
        
        int iNext = (i+1)%3;
        output.Pos = input[iNext].Pos;
        output.Pos = mul( output.Pos, View );
        output.Pos = mul( output.Pos, Projection );
		output.viewPos = input[iNext].Pos;
		output.viewPosition = mul( output.viewPos,PTView );
		output.viewPosition = mul(  output.viewPosition,PTProj);
        output.Normal = n;
        output.Tex = input[iNext].Tex;
        TriStream.Append( output );
        
        TriStream.RestartStrip();
    }
}

//PS_INPUT VS( VS_INPUT input )
//{
//	PS_INPUT output = (PS_INPUT)0;
//	output.Pos = mul( input.Pos, World );
//	output.Pos = mul( output.Pos, View );
//	output.Pos = mul( output.Pos, Projection );
//	output.Normal=mul( input.Normal.xyz, (float3x3)World );
//	output.Normal = normalize(output.Normal);
//	output.viewPos = mul(  input.Pos, World);
//	output.viewPosition = mul( output.viewPos,PTView );
//	output.viewPosition = mul(  output.viewPosition,PTProj);
//	output.Tex = input.Tex;
//
//	return output;
//}

//--------------------------------------------------------------------------------------
// RT Pixel Shader
//--------------------------------------------------------------------------------------
float4 RTPS( PS_INPUT input) : SV_Target
{
	return input.Pos;
}
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
	float4 color=float4(0.1,0.1,0.1,1);
	float3 lightDir=normalize(float3(2*sin(t/2),7,cos(t/2)));
	float4 diffuseColor=float4(1,1,1,1);
	float4 textureColor;
	float lightIntensity = saturate(dot(input.Normal, lightDir));
	float4 Dir=float4(projectorPos.xyz-float3(input.viewPos.xyz),0);
	float projectorIntensity =max(dot(input.Normal, normalize(Dir)),0);
	float2 projectTexCoord;
	float4 projectionColor;

		// Determine the light color based on the diffuse color and the amount of light intensity.
		color += 0.1*(diffuseColor * lightIntensity);
	


	// Saturate the light color.
	color = saturate(color);

	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	textureColor = shaderTexture.Sample(SampleType, input.Tex);

	// Combine the light color and the texture color.
	color = color * textureColor*0.5;

	// Calculate the projected texture coordinates.
	projectTexCoord.x =  input.viewPosition.x / input.viewPosition.w / 2.0f + 0.5f;
	projectTexCoord.y = -input.viewPosition.y / input.viewPosition.w / 2.0f + 0.5f;
	
	

	// Determine if the projected coordinates are in the 0 to 1 range.  If it is then this pixel is inside the projected view port.
	if((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
	{
		float test=depthTexture.Sample(SampleType,projectTexCoord).z;
		if(test>=input.viewPosition.z/input.viewPosition.w-0.000001)
	{
		// Sample the color value from the projection texture using the sampler at the projected texture coordinate location.
		projectionColor = projectionTexture.Sample(SampleType, projectTexCoord);
		float d=distance(projectorPos,float3(input.viewPos.xyz));
		// Set the output color of this pixel to the projection texture overriding the regular color value.
		if(projectorIntensity>=0)
	color+=0.7*projectionColor*projectorIntensity*600.0f/d/d;
	}
	}

	return color;
}
