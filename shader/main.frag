uniform int nTextureUnits;
uniform sampler2D tex1;
uniform sampler2D tex2;

uniform bool bFlashlight;
uniform bool bNightvision;

void Nightvision();
void ComplexFlashlight();
void Flashlight();

void main()
{
    gl_FragColor = texture2D(tex1, gl_TexCoord[0].st);
    if(nTextureUnits > 1)
        gl_FragColor += texture2D(tex2, gl_TexCoord[1].st);


    if (bFlashlight)
        ComplexFlashlight();

    if (bNightvision)
        Nightvision();
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
    cColor1 += texture2D(tex1, gl_TexCoord[0].st + 0.001);
    cColor1 += texture2D(tex1, gl_TexCoord[0].st + 0.002);
    cColor1 += texture2D(tex1, gl_TexCoord[0].st + 0.003);

    vec4 cColor2 = gl_FragColor / 2.0;
    cColor2 += texture2D(tex2, gl_TexCoord[1].st + 0.001);
    cColor2 += texture2D(tex2, gl_TexCoord[1].st + 0.002);
    cColor2 += texture2D(tex2, gl_TexCoord[1].st + 0.003);

    vec4 cColor = cColor1 * cColor2;

    cColor.r *= 0.2;
    cColor.b *= 0.2;

    gl_FragColor = cColor;
}
