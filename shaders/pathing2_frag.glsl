#ifdef GL_ES
out vec4 OUT_COLOR_VAR;
uniform highp vec4 baseColor;
uniform highp vec4 lineColor;
uniform highp float pData[80];
uniform highp int segmentCount;

flat in highp float segLength;
flat in highp vec4 appMask;
flat in highp vec4 hitMask;
smooth in highp float vertexSegment;
#else
#define OUT_COLOR_VAR fragColor
out vec4 fragColor;

uniform vec4 baseColor;
uniform vec4 lineColor;
uniform float pData[80];
uniform int segmentCount;

flat in float segLength;
flat in vec4 appMask;
flat in vec4 hitMask;
in float vertexSegment;
#endif

vec4 get_base_color(int fragSegment){
    if(fragSegment == 0) return vec4(1.0, 0.2, 0.1, 1.0);
    if(fragSegment == 1) return vec4(0.2, 1.0, 0.1, 1.0);
    if(fragSegment == 2) return vec4(0.2, 0.1, 1.0, 1.0);
    if(fragSegment == 3) return vec4(0.92, 0.93, 0.1, 1.0);
    if(fragSegment == 4) return vec4(0.1, 0.92, 0.87, 1.0);
    if(fragSegment == 5) return vec4(0.98, 0.18, 0.98, 1.0);
    return vec4(0.0, 0.0, 0.0, 1.0);
}

int compute_bit_is_set(vec4 value,int segment){
    int index = segment / 20;
    int num = segment % 20;

    if(index < 4)
    {
        int mask = int(value[index]);
        return (mask >> num) & 1;
    }

    //error
    return 0;
}

int compute_segment(){
    float start = 0.0;
    float uFragSeg = vertexSegment / float(segmentCount);
    for(int i = 0; i < segmentCount; i += 1){
        start += pData[i];
        float f = start / segLength;
        if(f > uFragSeg){
            return i;
        }
    }

    //error
    return int(ceil(vertexSegment) -1.0);
}

void main(void){
    int fragSegment = compute_segment();
    if(fragSegment > segmentCount || fragSegment < 0) discard;
    int first = fragSegment;
    int isIntersect = compute_bit_is_set(hitMask , first);
    int isOff       = compute_bit_is_set(appMask , first);
    vec4 color = baseColor;
    color.a = 1.0;
    if(isOff != 0){
        if(isIntersect != 0){
            color = color * 0.5;
            color.a = 0.5;
        }
        OUT_COLOR_VAR =  color;
    }else discard;
}

