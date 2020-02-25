#ifdef GL_ES
layout(location = 0) in highp vec3 position;
layout(location = 1) in highp mat3 maskData;
uniform highp mat4 model;
uniform highp mat4 view;
uniform highp mat4 projection;
uniform highp float segmentLength;
uniform highp int segmentCount;

flat out highp vec4 appMask;
flat out highp vec4 hitMask;
flat out highp float segLength;
smooth out highp float vertexSegment;
#else
layout(location = 0) in vec3 position;
layout(location = 1) in mat3 maskData;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float elevation;
uniform float segmentLength;
uniform int segmentCount;

flat out vec4 appMask;
flat out vec4 hitMask;
flat out float segLength;
out float vertexSegment;
#endif

void main(void){
    int i = 0;
    int ivertexSeg = 0;
    int target = 0;

    target = int( maskData[1][1] );
    target = (target == segmentCount-1 ? target+1 : target);
    vertexSegment = float(target);
    segLength = segmentLength;

    hitMask = vec4(maskData[0], maskData[1][0]);
    appMask = vec4(maskData[1][2], maskData[2][0],maskData[2][1],maskData[2][2]);

    mat4 MV = view * model;
    vec4 pos = vec4(position, 1.0);
    vec3 eyeSpacePosition = vec3(MV * pos);
    gl_Position = projection * vec4(eyeSpacePosition, 1.0);
}
