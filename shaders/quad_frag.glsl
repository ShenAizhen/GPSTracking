smooth in vec2 texCoords;
uniform vec3 cam_eye;
uniform vec3 cam_target;
uniform vec3 cam_up;
uniform float cam_fov;
uniform float win_width;
uniform float win_height;
uniform vec4 plane_color;
uniform vec4 edge_color;
uniform mat4 projection;
uniform mat4 view;

#define USE_BOX_AA

struct Ray{
    vec3 origin;
    vec3 direction;
};

struct Camera {
    vec3 origin;
    vec3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
};

struct Plane{
    vec3 normal;
    vec3 center;
};

struct Hit{
    vec3 point;
    float t;
    int hit;
};

Hit trace(Ray ray, Plane plane){
    Hit info;
    info.hit = -1;
    float denom = dot(plane.normal, ray.direction);
    if(abs(denom) > 0.0001){
        float t = dot((plane.center - ray.origin), plane.normal) / denom;
        if(t >= 0){
            info.point = ray.origin + t * ray.direction;
            info.hit = 1;
            info.t = t;
        }
    }

    return info;
}

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = projection * view * vec4(pos.xyz, 1.0);
    float clip_space_depth = clip_space_pos.z / clip_space_pos.w;
    float far = gl_DepthRange.far;
    float near = gl_DepthRange.near;
    float depth = (((far-near) * clip_space_depth) + near + far) / 2.0;
    return depth;
}

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

float gridTexture(vec2 p){
    vec2 grid = abs(fract(p - 0.5) - 0.5) / fwidth(p);
    return min(grid.x, grid.y);
}

Camera make_camera(vec3 lookfrom, vec3 lookat, vec3 vup, float vfov, float aspect){
    Camera camera;
    float theta = radians(vfov);
    float half_height = tan(theta/2);
    float half_width = aspect * half_height;
    camera.origin = lookfrom;
    camera.w = normalize(lookfrom - lookat);
    camera.u = normalize(cross(vup, camera.w));
    camera.v = normalize(cross(camera.w, camera.u));
    camera.lower_left_corner = camera.origin  -
            half_width*camera.u -
            half_height*camera.v -
            camera.w;
    camera.horizontal = 2 * half_width*camera.u;
    camera.vertical = 2 * half_height*camera.v;
    return camera;
}

Ray get_ray(float s, float t, Camera camera) {
    Ray ray;
    ray.origin = camera.origin;
    ray.direction = camera.lower_left_corner + s * camera.horizontal
                    + t * camera.vertical - camera.origin;
    return ray;
}

void main(void) {
    vec2 uv = 2.0*texCoords - 1.0;
    float aspect = win_width / win_height;
    Camera camera = make_camera(cam_eye, cam_target, cam_up, cam_fov, aspect);
    Ray ray = get_ray(texCoords.x, texCoords.y, camera);

    Plane plane;
    plane.center = vec3(0, 0, 0);
    plane.normal = vec3(0, 1, 0);

    Hit hit = trace(ray, plane);
    if(hit.hit > 0){
        vec3 R = hit.point;
        float dist = distance(R, cam_target);
        float k = 0.14;
        uv = k * R.xz;
#ifdef USE_BOX_AA
        vec2 ddx_uv = dFdx(uv); /* get derivative in x direction */
        vec2 ddy_uv = dFdy(uv); /* get derivative in y direction */
        float c = gridTextureGradBox(uv, ddx_uv, ddy_uv);
#else
        float c = gridTexture(uv);
        c = c > 1 ? 1 : c;
#endif
        float spotlight = min(1.0, 1.5 - 0.002*length(R.xz - cam_target.xz));
        vec3 color = vec3(c) * plane_color.rgb;// + 0.25 * edge_color.rgb;
        color *= spotlight;
        if(color.r + color.g + color.b < 0.1){ // assure pixels will fail depth test
            gl_FragColor = vec4(0);
            gl_FragDepth = 1;

            /* TODO(Felipe): Check if discard is better */
            //discard;
        }else{ // an actual plane pixel, compute its correct depth
            gl_FragColor = vec4(color, 1);
            gl_FragDepth = computeDepth(R);
        }
    }else{ // assure pixels will fail depth test
        gl_FragColor = vec4(0);
        gl_FragDepth = 1;
        /* TODO(Felipe): Check if discard is better */
        //discard;
    }
}
