uniform bool bUnit1Enabled;
uniform bool bUnit2Enabled;

uniform bool bFlashlight;
uniform bool bNightvision;

uniform sampler2D tex1;
uniform sampler2D tex2;

void Nightvision();
void ComplexFlashlight();
void Flashlight();

void main()
{
    gl_FragColor = gl_Color;

    vec4 texel1 = vec4(1.0);
    vec4 texel2 = vec4(1.0);

    if(bUnit1Enabled)
        texel1 = texture2D(tex1, gl_TexCoord[0].st);
    if(bUnit2Enabled)
        texel2 = texture2D(tex2, gl_TexCoord[1].st);

    //gl_FragColor = mix(texel1, texel2, 0.5);
    gl_FragColor = vec4(texel1.rgb * texel2.rgb, texel1.a);

    if (bFlashlight)
        ComplexFlashlight();

    if (bNightvision)
        Nightvision();

    // Brightness
    gl_FragColor = gl_FragColor * 2.0;
}

varying vec4 diffuse,ambientGlobal, ambient;
varying vec3 normal,lightDir,halfVector;
varying float dist;

void Flashlight()
{
    vec4 color = gl_FragColor;
    float att,spotEffect;

    spotEffect = dot(normalize(gl_LightSource[0].spotDirection), normalize(-lightDir));
    if (spotEffect > gl_LightSource[0].spotCosCutoff)
    {
        spotEffect = pow(spotEffect, gl_LightSource[0].spotExponent);
        att = spotEffect / (gl_LightSource[0].constantAttenuation +
                            gl_LightSource[0].linearAttenuation * dist +
                            gl_LightSource[0].quadraticAttenuation * dist * dist);

        color += att;
    }

    gl_FragColor = color;
}

void ComplexFlashlight()
{
	vec3 n,halfV;
	float NdotL,NdotHV;
	vec4 color = /*ambientGlobal*/ gl_FragColor;
	float att,spotEffect;

	/* a fragment shader can't write a verying variable, hence we need
	a new variable to store the normalized interpolated normal */
	n = normalize(normal);

	/* compute the dot product between normal and ldir */
	NdotL = max(dot(n,normalize(lightDir)),0.0);

	if (NdotL > 0.0) {

		spotEffect = dot(normalize(gl_LightSource[0].spotDirection), normalize(-lightDir));
		if (spotEffect > gl_LightSource[0].spotCosCutoff) {
			spotEffect = pow(spotEffect, gl_LightSource[0].spotExponent);
			att = spotEffect / (gl_LightSource[0].constantAttenuation +
					gl_LightSource[0].linearAttenuation * dist +
					gl_LightSource[0].quadraticAttenuation * dist * dist);

			color += att * (diffuse * NdotL + ambient);


			halfV = normalize(halfVector);
			NdotHV = max(dot(n,halfV),0.0);
			color += att * gl_FrontMaterial.specular * gl_LightSource[0].specular * pow(NdotHV,gl_FrontMaterial.shininess);
		}
	}

	gl_FragColor = color;
}

void Nightvision()
{
    vec4 cColor1 = gl_FragColor / 2.0;
    cColor1 += texture2D(tex1, gl_TexCoord[0].st + 0.01);
    cColor1 += texture2D(tex1, gl_TexCoord[0].st + 0.02);
    cColor1 += texture2D(tex1, gl_TexCoord[0].st + 0.03);

    vec4 cColor2 = gl_FragColor / 2.0;
    cColor2 += texture2D(tex2, gl_TexCoord[1].st + 0.01);
    cColor2 += texture2D(tex2, gl_TexCoord[1].st + 0.02);
    cColor2 += texture2D(tex2, gl_TexCoord[1].st + 0.03);

    vec4 cColor = cColor1 * cColor2;

    cColor.r *= 0.2;
    cColor.b *= 0.2;

    gl_FragColor = cColor;
}
