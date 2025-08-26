#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float itime;
uniform float darkness;
uniform bool jekyll;
uniform vec2 cam_scroll;

// Constants
uniform vec2 scroll  = vec2(0.05, -0.05);
uniform vec2 scroll2 = vec2(0.05,  0.05);
uniform vec2 scroll3 = vec2(0.05, -0.15);
uniform vec2 scroll4 = vec2(0.15,  0.05);

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
    for (int i = 0; i < 5; i++) {   // bumped octaves
        value += amplitude * noise(st);
        st *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void foreground(inout vec4 col) {
    vec4 tex_color = texture(texture0, fragTexCoord);
    col = tex_color;

    vec2 baseUV = fragTexCoord * 5.0 + cam_scroll * -0.001;

    float depth1 = fbm(baseUV + itime * scroll * 2.0) * 
                   fbm(baseUV * 1.2 + itime * scroll2 * 0.5);

    float depth2 = fbm(baseUV * 0.8 + itime * scroll3 * 0.7) * 
                   fbm(baseUV * 1.6 + itime * scroll4 * 0.9);

    float depth = mix(depth1, depth2, 0.5);

    // More contrasty fog factor
    float fogFactor = pow(exp(-1.2 + depth * 2.0), 1.5);

    // Dynamic color shifts
    float pulse = 0.5 + 0.5 * sin(itime * 0.8);

    vec3 fog_color1 = jekyll 
        ? vec3(0.2 + 0.1*pulse, 0.15, 0.4)   // cooler shifting
        : vec3(0.4, 0.15 + 0.1*pulse, 0.2); // warmer shifting

    vec3 fog_color2 = jekyll 
        ? vec3(0.1, 0.12 + 0.05*pulse, 0.35) 
        : vec3(0.35, 0.1, 0.15 + 0.05*pulse);

    vec3 fog_color = mix(fog_color1, fog_color2, depth);

    col.rgb = mix(fog_color, col.rgb, fogFactor);

    col = mix(col, vec4(0,0,0,1), darkness);
}

void overlay_frag(inout vec4 col) {
    vec2 uv = fragTexCoord * 6.0 
              + itime * 0.05 
              + cam_scroll * -0.001;

    float n = fbm(uv);

    float center_dis = distance(fragTexCoord, vec2(0.5, 0.5));
    float noise_val = center_dis + n * 0.2;

    vec4 tint = jekyll 
        ? vec4(0.1, 0.15, 0.25, 1.0) 
        : vec4(0.2, 0.05, 0.05, 1.0);

    float darkAmt = max(0.0, noise_val - 0.65) * 8.0;

    col = mix(col, tint, darkAmt * 0.3);
}

void main() {
    vec4 col; 
    foreground(col);
    overlay_frag(col);
    finalColor = col;
}
