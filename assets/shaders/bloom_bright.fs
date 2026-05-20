#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float threshold;     // 0..1 brightness cutoff
uniform float soft_knee;     // smoothness of the cutoff (0..0.5)

out vec4 finalColor;

void main()
{
    vec3 c = texture(texture0, fragTexCoord).rgb;
    float br = max(c.r, max(c.g, c.b));
    float knee = threshold * soft_knee + 1e-5;
    float soft = clamp(br - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 1e-5);
    float contribution = max(soft, br - threshold);
    contribution /= max(br, 1e-5);
    finalColor = vec4(c * contribution, 1.0);
}
