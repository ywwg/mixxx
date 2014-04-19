uniform int viewportWidth;
uniform int viewportHeigth;

uniform vec4 lowColor;
uniform vec4 midColor;
uniform vec4 highColor;

uniform float lowGain;
uniform float midGain;
uniform float highGain;

uniform int textureSize;
uniform sampler2D signalTexture;

void main(void)
{
    vec2 uv = gl_TexCoord[0].st;

    vec4 rawFiltredSignal = texture2D(signalTexture, uv);

    //(vrince) debug see pre-computed signal
    //gl_FragColor = rawFiltredSignal;
    //return;

    vec4 outputColor = vec4(0.0, 0.0, 0.0, 0.0);

    float ourDistance = abs((uv.y - 0.5) * 2.0);
    vec4 signalDistance = rawFiltredSignal - ourDistance;

    // Fade waveforms based on gain, but never fade all the way out.
    float lowFade = lowGain * .8 + .2;
    float midFade = midGain * .8 + .2;
    float highFade = highGain * .8 + .2;

    if (lowFade > 1.0) {
      lowFade = 1.0;
    }
    if (midFade > 1.0) {
      midFade = 1.0;
    }
    if (highFade > 1.0) {
      highFade = 1.0;
    }

    if (signalDistance.x > 0.0) {
      outputColor.x += lowColor.x * lowFade;
      outputColor.y += lowColor.y * lowFade;
      outputColor.z += lowColor.z * lowFade;
      outputColor.w = 0.8;
    }

    if (signalDistance.y > 0.0) {
      outputColor.x += midColor.x * midFade;
      outputColor.y += midColor.y * midFade;
      outputColor.z += midColor.z * midFade;
      outputColor.w = 0.8;
    }

    if (signalDistance.z > 0.0) {
      outputColor.x += highColor.x * highFade;
      outputColor.y += highColor.y * highFade;
      outputColor.z += highColor.z * highFade;
      outputColor.w = 0.8;
    }

    /*
    vec4 distanceToRigthSignal = 0.5 - uv.y - 0.5 *texture2D(signalTexture,vec2(uv.x,0.25));
    vec4 distanceToLeftSignal = uv.y - 0.5 * texture2D(signalTexture,vec2(uv.x,0.75)) - 0.5;

    if (distanceToRigthSignal.x < 0.0 && distanceToLeftSignal.x < 0.0)
        outputColor += lowColor;

    if( distanceToRigthSignal.y < 0.0 && distanceToLeftSignal.y < 0.0)
        outputColor += midColor;

    if( distanceToRigthSignal.z < 0.0 && distanceToLeftSignal.z < 0.0)
        outputColor += highColor;
    */
    gl_FragColor = outputColor;
    return;

    /*
    uv.y = 0.25;
    vec4 signalRigth = texture2D(signalTexture,uv);


    vec4 outputColor = vec4(0.0,0.0,0.0,0.0);
    vec3 accumulatedData = vec3(0.0,0.0,0.0);
    //vec3 meanData = vec3(0.0);

    for( float i = firstPixelPosition; i < lastPixelPosition; i += 2.0) {
        vec4 currentData;
        if( uv.y > 0.5) {
            //currentData = getWaveformData_linearInterpolation(i);
            currentData = getWaveformData(i);

            if( currentData.x > uv.y-0.5) //low
                accumulatedData.x += pixelWeigth;
            if( currentData.y > uv.y-0.5) //Mid
                accumulatedData.y += pixelWeigth;
            if( currentData.z > uv.y-0.5) //High
                accumulatedData.z += pixelWeigth;
        }
        else {
            currentData = getWaveformData(i+1);

            if( currentData.x > -uv.y+0.5) //low
                accumulatedData.x += pixelWeigth;
            if( currentData.y > -uv.y+0.5) //Mid
                accumulatedData.y += pixelWeigth;
            if( currentData.z > -uv.y+0.5) //High
                accumulatedData.z += pixelWeigth;
        }
    }

    vec4 low = lowColor;
    //low.a = 0.25+3.0*abs(uv.y-0.5)/accumulatedData.x;
    low.a = accumulatedData.x;

    vec4 mid = midColor;
    //mid.a = 0.5+3.0*abs(uv.y-0.5)/accumulatedData.y;
    mid.a = accumulatedData.y;

    vec4 high = highColor;
    //high.a = clamp(0.25+(1.0-abs(uv.y-0.5)-0.1*accumulatedData.z)/accumulatedData.z,0.f,1.f);
    high.a = accumulatedData.z;

    if( accumulatedData.x > 0)
        outputColorColor = mix( outputColorColor, low, clamp(accumulatedData.x,0.1f,0.9f));
    if( accumulatedData.y > 0)
        outputColorColor = mix( outputColorColor, mid, clamp(accumulatedData.y,0.1f,0.9f));
    if( accumulatedData.z > 0)
        outputColorColor = mix( outputColorColor, high, clamp(accumulatedData.z,0.1f,0.9f));

    gl_FragColor = outputColorColor;
    return;
    */
}
