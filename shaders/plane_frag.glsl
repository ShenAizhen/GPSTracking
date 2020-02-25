in vec3 vPosition;
in vec3 vNear;
in vec3 vFar;

uniform vec4 baseColor;
uniform mat4 projection;
uniform mat4 view;

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = projection * view * vec4(pos.xyz, 1.0);
    float clip_space_depth = clip_space_pos.z / clip_space_pos.w;
    float far = gl_DepthRange.far;
    float near = gl_DepthRange.near;
    float depth = (((far-near) * clip_space_depth) + near + far) / 2.0;
    return depth;
}

/* Generates the grid pattern texture with box filter for anti-aliasing */
const float N = 50.0; // grid ratio
float gridTextureGradBox(vec2 p, vec2 ddx, vec2 ddy){
    // filter kernel
    vec2 w = max(abs(ddx), abs(ddy)) + 0.01;
    // analytic (box) filtering
    vec2 a = p + 0.5*w;
    vec2 b = p - 0.5*w;
    vec2 i = (floor(a)+min(fract(a)*N,1.0)-
              floor(b)-min(fract(b)*N,1.0))/(N*w);
    //pattern
    return (1.0-i.x)*(1.0-i.y);
}

void main(void) {
   float t = -vNear.y / (vFar.y-vNear.y);
   vec3 R = vNear + t * (vFar-vNear);
   float k = 0.11;
   vec2 uv = k * R.xz;
   vec2 ddx_uv = dFdx(uv); /* get derivative in x direction */
   vec2 ddy_uv = dFdy(uv); /* get derivative in y direction */
   float c = gridTextureGradBox(uv, ddx_uv, ddy_uv); /* box filter intergration */
   vec3 color = vec3(c) * baseColor.rgb;
   gl_FragColor = vec4(color, 1) * float(t > 0);
   gl_FragDepth = computeDepth(R);
}
