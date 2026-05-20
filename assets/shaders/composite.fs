#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;    // scene
uniform sampler2D bloomTex;    // blurred bright pass
uniform float bloom_intensity;
uniform float vignette_strength;
uniform float low_hp;          // 0 = healthy, 1 = critical (boosts vignette + tint)

out vec4 finalColor;

void main()
{
    vec3 scene = texture(texture0, fragTexCoord).rgb;
    vec3 bloom = texture(bloomTex, fragTexCoord).rgb;
    vec3 col   = scene + bloom * bloom_intensity;

    // Vignette — distance from screen centre.
    vec2 d = fragTexCoord - vec2(0.5);
    float r2 = dot(d, d) * 2.5;
    float v  = clamp(1.0 - r2 * (vignette_strength + low_hp * 0.9), 0.0, 1.0);
    col *= v;

    // Critical-HP red tint at the edges.
    if (low_hp > 0.0) {
        float edge = clamp(r2 * 1.4 - 0.2, 0.0, 1.0);
        col = mix(col, col * vec3(1.35, 0.65, 0.55), edge * low_hp);
    }

    finalColor = vec4(col, 1.0);
}
