#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float u_time;
uniform float u_intensity;   // 0..1, current thrust strength

out vec4 finalColor;

// Cheap value-noise: hash + bilinear sample.
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float vnoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

void main() {
    // The quad is laid out so:
    //   uv.x = 0 → at the ship (plume root, hottest)
    //   uv.x = 1 → far tail (coolest, fading out)
    //   uv.y centered around 0.5 with cross-section width 1.
    vec2 uv = fragTexCoord;
    float u  = uv.x;
    float v  = uv.y - 0.5;          // -0.5 .. 0.5

    // Wobble: more flicker towards the tail; less near the engine where
    // the column is collimated.
    float n  = vnoise(vec2(u * 6.0, u * 14.0 - u_time * 6.0));
    float wob = (n - 0.5) * 0.22 * smoothstep(0.0, 0.7, u);

    // Width profile: thick at the engine, tapers to 0 at the tail.
    float half_w = mix(0.46, 0.02, smoothstep(0.0, 1.0, u));
    float d      = abs(v + wob);
    float mask   = 1.0 - smoothstep(half_w * 0.55, half_w, d);

    // Color sweep along the length (HDR-ish values so bloom catches them):
    //   hot core (cool white-blue) → cyan-blue → yellow → orange → red → dim red.
    vec3 hot     = vec3(1.30, 1.55, 2.20);
    vec3 blue    = vec3(0.35, 0.75, 1.65);
    vec3 yellow  = vec3(1.95, 1.55, 0.45);
    vec3 orange  = vec3(1.85, 0.65, 0.20);
    vec3 red     = vec3(1.20, 0.20, 0.10);
    vec3 dim_red = vec3(0.45, 0.06, 0.03);

    vec3 col;
    if (u < 0.08)      col = mix(hot,    blue,    smoothstep(0.0,  0.08, u));
    else if (u < 0.28) col = mix(blue,   yellow,  smoothstep(0.08, 0.28, u));
    else if (u < 0.55) col = mix(yellow, orange,  smoothstep(0.28, 0.55, u));
    else if (u < 0.85) col = mix(orange, red,     smoothstep(0.55, 0.85, u));
    else               col = mix(red,    dim_red, smoothstep(0.85, 1.0,  u));

    // Tail fade — even within the mask, the column dies out beyond ~0.7.
    float tail_fade = 1.0 - smoothstep(0.55, 1.0, u);
    mask *= tail_fade;

    // Engine-side brightness boost so the core glows hard.
    float core_glow = 1.0 + 0.6 * smoothstep(0.10, 0.0, u);
    mask *= core_glow;

    mask *= u_intensity;

    finalColor = vec4(col * mask, mask);
}
