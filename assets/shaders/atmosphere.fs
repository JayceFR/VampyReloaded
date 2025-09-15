#ifdef GL_ES
precision mediump float;
#endif

varying vec2 fragTexCoord;
uniform sampler2D texture0;
uniform float itime;
uniform float darkness;
uniform int jekyll;      // use int for GLES bool
uniform vec2 cam_scroll;

// Constants
const vec2 scroll  = vec2(0.05, -0.05);
const vec2 scroll2 = vec2(0.05,  0.05);
const vec2 scroll3 = vec2(0.05, -0.15);
const vec2 scroll4 = vec2(0.15,  0.05);

// ----------------------- Procedural Noise -----------------------
float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    float a = rand(i);
    float b = rand(i + vec2(1.0, 0.0));
    float c = rand(i + vec2(0.0, 1.0));
    float d = rand(i + vec2(1.0, 1.0));

    vec2 u = f*f*(3.0-2.0*f);

    return mix(a, b, u.x) +
           (c - a)* u.y * (1.0 - u.x) +
           (d - b) * u.x * u.y;
}

float fbm(vec2 st) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(st);
        st *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void foreground(inout vec4 col) {
    vec4 tex_color = texture2D(texture0, fragTexCoord);
    col = tex_color;

    // Larger scale = smoother shapes
    vec2 uv = fragTexCoord * 2.5 + cam_scroll * 0.001;

    // Faster FBM motion (increased multipliers)
    float depth1 = fbm(uv + itime * vec2(0.15, -0.10));
    float depth2 = fbm(uv * 1.2 + itime * vec2(-0.12, 0.18));

    // Blend them softly
    float depth = mix(depth1, depth2, 0.5);

    // Fog factor
    float fogFactor = smoothstep(0.35, 0.75, depth);

    // Subtle pulse to keep it alive
    float pulse = 0.5 + 0.5 * sin(itime * 1.5); // slightly faster pulse

    // Softer bluish palette
    vec3 fog_color1 = (jekyll == 0)
        ? vec3(0.55 + 0.03*pulse, 0.65 + 0.04*pulse, 0.78 + 0.05*pulse)
        : vec3(0.45 + 0.03*pulse, 0.55 + 0.04*pulse, 0.68 + 0.05*pulse);

    vec3 fog_color2 = (jekyll == 0)
        ? vec3(0.4, 0.48, 0.65)
        : vec3(0.3, 0.38, 0.55);

    vec3 fog_color = mix(fog_color2, fog_color1, depth);

    // More subtle fog so player is visible
    col.rgb = mix(col.rgb, fog_color, fogFactor * 0.4);

    // Darkness overlay as before
    col = mix(col, vec4(0.0, 0.0, 0.0, 1.0), darkness);
}



void overlay_frag(inout vec4 col) {
    vec2 uv = fragTexCoord * 6.0 + itime * 0.5 + cam_scroll * -0.005;

    float n = fbm(uv);

    float center_dis = distance(fragTexCoord, vec2(0.5, 0.5));
    float noise_val = center_dis + n * 0.2;

    vec4 tint = (jekyll == 1)
        ? vec4(0.1, 0.15, 0.25, 1.0)
        : vec4(0.2, 0.05, 0.05, 1.0);

    float darkAmt = max(0.0, noise_val - 0.65) * 8.0;
    col = mix(col, tint, darkAmt * 0.3);
}

void main() {
    vec4 col = vec4(0.0);
    foreground(col);
    overlay_frag(col);
    gl_FragColor = col;
}
