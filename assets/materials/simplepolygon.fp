varying mediump vec4 position;
varying highp vec2 var_texcoord0;

uniform mediump sampler2D tex0;

const highp vec2 pixeluv = vec2(0.0009765625, 0.0009765625);
const highp vec2 pixelhalf = vec2(0.00048828125, 0.00048828125);
const highp float scale = 1.0/10000.0;

float DigitBin( const int x )
{
    return x==0?480599.0:x==1?139810.0:x==2?476951.0:x==3?476999.0:x==4?350020.0:x==5?464711.0:x==6?464727.0:x==7?476228.0:x==8?481111.0:x==9?481095.0:0.0;
}

float PrintValue( vec2 vStringCoords, float fValue, float fMaxDigits, float fDecimalPlaces )
{       
    if ((vStringCoords.y < 0.0) || (vStringCoords.y >= 1.0)) return 0.0;
    
    bool bNeg = ( fValue < 0.0 );
	fValue = abs(fValue);
    
	float fLog10Value = log2(abs(fValue)) / log2(10.0);
	float fBiggestIndex = max(floor(fLog10Value), 0.0);
	float fDigitIndex = fMaxDigits - floor(vStringCoords.x);
	float fCharBin = 0.0;
	if(fDigitIndex > (-fDecimalPlaces - 1.01)) {
		if(fDigitIndex > fBiggestIndex) {
			if((bNeg) && (fDigitIndex < (fBiggestIndex+1.5))) fCharBin = 1792.0;
		} else {		
			if(fDigitIndex == -1.0) {
				if(fDecimalPlaces > 0.0) fCharBin = 2.0;
			} else {
                float fReducedRangeValue = fValue;
                if(fDigitIndex < 0.0) { fReducedRangeValue = fract( fValue ); fDigitIndex += 1.0; }
				float fDigitValue = (abs(fReducedRangeValue / (pow(10.0, fDigitIndex))));
                fCharBin = DigitBin(int(floor(mod(fDigitValue, 10.0))));
			}
        }
	}
    return floor(mod((fCharBin / pow(2.0, floor(fract(vStringCoords.x) * 4.0) + (floor(vStringCoords.y * 5.0) * 4.0))), 2.0));
}

mediump float decode_float( vec4 val ) {

	mediump float r = (val.r * 255.0);
	mediump float g = (val.g * 255.0);
	mediump float b = (val.b * 255.0);
	mediump float a = (val.a * 255.0);	

	mediump float sign = 1.0;
	//if( a > 254.0 ) { a -= 255.0; sign = -1.0; }
	unsigned int num = unsigned int(r) + unsigned int(g * 256.0) + unsigned int(b * 256.0 * 256.0) + unsigned int(a * 256.0 * 256.0 * 256.0);
	return int(num) * 0.001;
}

mediump vec2 getvert( int pos, int base )
{
	pos = pos * 2 + base;
	// Convert index into uv rg or ba lookup for points.
	mediump int line = (pos / 1024);
	mediump int col = (pos % 1024);
	mediump vec2 uv = vec2(col * pixeluv.x, 1.0 - line * pixeluv.y);
	mediump vec4 rgba = texture2D(tex0, uv);
	mediump float x = decode_float(rgba);

	line = ((pos + 1) / 1024);
	col = ((pos + 1) % 1024);
	uv = vec2( col * pixeluv.x, 1.0 - line * pixeluv.y);
	rgba = texture2D(tex0, uv);
	mediump float y = decode_float(rgba);

	return vec2(x * scale, y * scale);
}

float getcount(int idx )
{
	// Convert index into uv rg or ba lookup for points. 
	int line = (idx / 1024);
	int col = (idx % 1024);

	mediump vec2 uv = vec2( col * pixeluv.x, 1.0 - line  * pixeluv.y);
	mediump vec4 rgba = texture2D(tex0, uv);
	return decode_float(rgba);
}


// Simple point in polygon test extracted from @iq's polygon
// distance shader: https://www.shadertoy.com/view/wdBXRW
// bool pointInPolygon(in vec2 p, in vec2[N] v) {
//     bool res = false;
//     for (int i=0, j=N - 1; i < N; j = i, i++) {
//         vec2 e = v[j] - v[i];
//         vec2 w =    p - v[i];

//         // Winding number from http://geomalgorithms.com/a03-_inclusion.html
//         bvec3 c = bvec3(p.y >= v[i].y, p.y < v[j].y, e.x * w.y > e.y * w.x);
//         if (all(c) || all(not(c))) res = !res;  
//     }
    
//     return res;
// }


// Simple point in polygon test extracted from @iq's polygon
// distance shader: https://www.shadertoy.com/view/wdBXRW
bool pointInPolygon(in vec2 p)
{
	bool res = false;

	int idx = 0;
	//vec4 col = encode_float(6.0);
	int pcount = int(getcount(idx++));

	// int temp = int(getcount(p, idx++));
	// if(p.x < temp * pixeluv.x)
	//   	return true;
	// else 
	// 	return false;

	for (int pi=0; pi<1; pi++)
	{
		int count = int(getcount(idx++));
		res = false;

		for (int i = 0, j = count - 1; i < count; i++)
		{
			mediump vec2 vi = getvert(i,idx);
			mediump vec2 vj = getvert(j,idx);

			mediump vec2 e = vj - vi;

			// If the distance is huge, then there is something wrong with number so ignore it
			if( dot(e, e) < 2.2 )
			{
				mediump vec2 w = p - vi;

				// Winding number from http://geomalgorithms.com/a03-_inclusion.html
				bvec3 c = bvec3(p.y >= vi.y, p.y<vj.y, e.x * w.y> e.y * w.x);
				if (all(c) || all(not(c)))
					res = !res;

				j = i;
			}
		}
		idx += count * 2;
		if(res) return res;
	}

	return res;
}

float sdLine(in vec2 p, in vec2 a, in vec2 b)
{
	vec2 pa = p - a, ba = b - a;
	return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0));
}

void main()
{
    // vec2 uv = (fragCoord - center) / iResolution.y;
    // vec2 mouse = (iMouse.xy - center) / iResolution.y;
    // float unit = 2.0 / iResolution.y;
    // float time = 0.5 * iTime;
    // vec3 color = vec3(1.0);
	// poly[0] = vec2(.77, 0.0);
    // poly[1] = vec2(0.15, sqrt(0.25));
    // poly[2] = vec2(-0.5, poly[1].y-0.7);
    // poly[3] = vec2(-0.63, 0.0);
    // poly[4] = vec2(-0.15, -poly[1].y);
    // poly[5] = vec2(0.5, -poly[1].y);

	vec2 uv = var_texcoord0.xy;
	vec2 fragCoord = uv * 1024;
	vec3 color = vec3(1.0);

	if (pointInPolygon(uv))
	{
		color = vec3(0.078, 0.576, 0.106);
	}

// const mediump vec2 poly[6] = { 
// 	   {.77, 0.0},
//     {0.15, 0.0625},
//     {-0.5, -0.6375},
//     {-0.63, 0.0},
//     {-0.15, -0.0625},
//     {0.5, -0.0625}
// };

	// Convert index into uv rg or ba lookup for points. 
	// int idx = 3;
	// int line = (idx / 1024);
	// int col = (idx % 1024);

	// highp vec2 uv1 = vec2( col * pixeluv.x, 1.0 - line * pixeluv.y);
	// mediump vec4 rgba = texture2D(tex0, uv1);
	// vec2 vFontSize = vec2(8.0, 15.0);

	// float digit = PrintValue( (fragCoord - vec2(184.0, 5.0)) / vFontSize, rgba.r * 255.0, 3, 6);
	// color = mix( color, vec3(1.0, 0.0, 0.0), digit);
	// digit = PrintValue( (fragCoord -  vec2(184.0 + 100, 5.0)) / vFontSize, rgba.g * 255.0, 3, 6);
	// color = mix( color, vec3(1.0, 0.0, 0.0), digit);
	// digit = PrintValue( (fragCoord -  vec2(184.0 + 200.0, 5.0)) / vFontSize, rgba.b * 255.0, 3, 6);
	// color = mix( color, vec3(1.0, 0.0, 0.0), digit);
	// digit = PrintValue( (fragCoord -  vec2(184.0 + 300.0, 5.0)) / vFontSize, rgba.a * 255.0, 3, 6);
	// color = mix( color, vec3(1.0, 0.0, 0.0), digit);

	// float num = decode_float(rgba);
	// digit = PrintValue( (fragCoord -  vec2(184.0 + 500.0, 5.0)) / vFontSize, num, 3, 6);
	// color = mix( color, vec3(0.0, 0.0, 1.0), digit);
	// digit = PrintValue( (fragCoord -  vec2(184.0 + 600.0, 5.0)) / vFontSize, uv1.y, 3, 6);
	// color = mix( color, vec3(0.0, 0.0, 1.0), digit);


	gl_FragColor = vec4(color, 1.0);
}
