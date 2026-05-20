#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 direction;        // (1/w, 0) horizontal pass, (0, 1/h) vertical pass

out vec4 finalColor;

// 9-tap gaussian, sigma ~2.
const float weights[5] = float[5](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main()
{
    vec3 acc = texture(texture0, fragTexCoord).rgb * weights[0];
    for (int i = 1; i < 5; ++i) {
        vec2 off = direction * float(i);
        acc += texture(texture0, fragTexCoord + off).rgb * weights[i];
        acc += texture(texture0, fragTexCoord - off).rgb * weights[i];
    }
    finalColor = vec4(acc, 1.0);
}
