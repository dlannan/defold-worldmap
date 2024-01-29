varying mediump vec4 position;
varying highp vec2 var_texcoord0;

uniform mediump sampler2D tex0;

const highp vec2 pixeluv = vec2(0.0009765625, 0.0009765625);
const highp vec2 pixelhalf = vec2(0.00048828125, 0.00048828125);
const highp float scale = 0.5;

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

// const mediump vec2 poly[6] = { 
// 	{.77, 0.0},
//     {0.15, 0.0625},
//     {-0.5, -0.6375},
//     {-0.63, 0.0},
//     {-0.15, -0.0625},
//     {0.5, -0.0625}
// };

// Original (slightly modified to match the interface of the trigless one)
float cr2(vec2 A, vec2 B)
{
	return A.x * B.y - A.y * B.x;
}

float getAngle(vec2 A, vec2 B)
{
	return atan(cr2(A, B), dot(A, B));
}

float shift_right (float v, float amt) { 
    v = floor(v) + 0.5; 
    return floor(v / exp2(amt)); 
}

float shift_left (float v, float amt) { 
    return floor(v * exp2(amt) + 0.5); 
}

float mask_last (float v, float bits) { 
    return mod(v, shift_left(1.0, bits)); 
}

float extract_bits (float num, float from, float to) { 
    from = floor(from + 0.5); to = floor(to + 0.5); 
    return mask_last(shift_right(num, from), to - from); 
}

vec4 encode_float (float val) { 
    if (val == 0.0) return vec4(0, 0, 0, 0); 
    float sign = val > 0.0 ? 0.0 : 1.0; 
    val = abs(val); 
    float exponent = floor(log2(val)); 
    float biased_exponent = exponent + 127.0; 
    float fraction = ((val / exp2(exponent)) - 1.0) * 8388608.0; 
    float t = biased_exponent / 2.0; 
    float last_bit_of_biased_exponent = fract(t) * 2.0; 
    float remaining_bits_of_biased_exponent = floor(t); 
    float byte4 = extract_bits(fraction, 0.0, 8.0) / 255.0; 
    float byte3 = extract_bits(fraction, 8.0, 16.0) / 255.0; 
    float byte2 = (last_bit_of_biased_exponent * 128.0 + extract_bits(fraction, 16.0, 23.0)) / 255.0; 
    float byte1 = (sign * 128.0 + remaining_bits_of_biased_exponent) / 255.0; 
    return vec4(byte4, byte3, byte2, byte1); 
}

// float decode_float( vec4 val ) {

// 	float r = (val.r * 255.0 + 0.1);
// 	float g = (val.g * 255.0 + 0.1);
// 	float b = (val.b * 255.0 + 0.1);
// 	float a = (val.a * 255.0 + 0.1);	

// 	float sign = ( a / pow( 2., 7. ) ) >= 1. ? -1. : 1.;
// 	float s = a;
// 	if( s > 128. ) s -= 128.;
// 	float exponent = s * 2. + floor( b / pow( 2., 7. ) );
// 	float mantissa = ( r + g * 256. + clamp( b - 128., 0., 255. ) * 256. * 256. );
// 	float t = b;
// 	if( t > 128. ) t -= 128.;
// 	mantissa = t * 256. * 256. + g * 256. + r;
// 	return sign * pow( 2., exponent - 127. ) * ( 1. + mantissa / pow ( 2., 23. ) ) * 2.0;
// }

mediump float decode_float( vec4 val ) {

	mediump float r = (val.r * 255.0);
	mediump float g = (val.g * 255.0);
	mediump float b = (val.b * 255.0);
	mediump float a = (val.a * 255.0);	

	mediump float sign = 1.0;
	if( a > 128.0 ) { a -= 128.0; sign = -1.0; }
	mediump float num = r + (g * 256.0) + (b * 256.0 * 256.0) + (a * 256.0 * 256.0 * 256.0);
	return num * sign * 0.0001;
}
// // note: the 0.1s here an there are voodoo related to precision
// float decode_float(vec4 v) {
// 	vec4 bits = v * 255.0;
// 	float sign = mix(-1.0, 1.0, step(bits[3], 128.0));
// 	float expo = floor(mod(bits[3] + 0.1, 128.0)) * 2.0 +
// 				floor((bits[2] + 0.1) / 128.0) - 127.0;
// 	float sig = bits[0] +
// 				bits[1] * 256.0 +
// 				floor(mod(bits[2] + 0.1, 128.0)) * 256.0 * 256.0;
// 	return sign * (1.0 + sig / 8388607.0) * pow(2.0, expo);
// }

mediump vec2 getvert( int pos, int base )
{
	pos = pos * 2 + base;
	// Convert index into uv rg or ba lookup for points.
	mediump float line = int(pos / 1024) * pixeluv.y;
	mediump float col = (pos % 1024) * pixeluv.x;
	mediump vec2 uv = vec2(col, 1.0 - line);
	mediump vec4 rgba = texture2D(tex0, uv);
	mediump float x = decode_float(rgba);

	line = int((pos + 1) / 1024) * pixeluv.y;
	col = ((pos + 1) % 1024) * pixeluv.x;
	uv = vec2( col, 1.0 - line);
	rgba = texture2D(tex0, uv);
	mediump float y = decode_float(rgba);

	return vec2(x * scale, y * scale) + vec2(0.5, 0.5);
}

int getcount(vec2 fragCoord, int idx )
{
	// Convert index into uv rg or ba lookup for points. 
	int line = (idx / 1024);
	int col = (idx % 1024);

	mediump vec2 uv = vec2( col * pixeluv.x, 1.0 - line  * pixeluv.y);
	mediump vec4 rgba = texture2D(tex0, uv);
	return int(rgba.a * 255.0); //decode_float(rgba);
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
	int pcount = int(getcount(p, idx++));

	int temp = int(getcount(p, idx++));
	if(p.x < temp * pixeluv.x)
	  	return true;
	else 
		return false;

	for (int pi=0; pi<1; pi++)
	{
		int count = int(getcount(p, idx++));

		for (int i = 0, j = count - 1; i < count; j = i, i++)
		{
			mediump vec2 vi = getvert(i,idx);
			mediump vec2 vj = getvert(j,idx);

			mediump vec2 e = vj - vi;
			mediump vec2 w = p - vi;

			// Winding number from http://geomalgorithms.com/a03-_inclusion.html
			bvec3 c = bvec3(p.y >= vi.y, p.y<vj.y, e.x * w.y> e.y * w.x);
			if (all(c) || all(not(c)))
				res = !res;
		}
		//if(res) return res;
		idx += count * 2;
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
	vec3 color = vec3(0.0);

	// if (pointInPolygon(uv))
	// {
	// 	color = vec3(0.078, 0.576, 0.106);
	// }

	// Convert index into uv rg or ba lookup for points. 
	int idx = 2;
	int line = (idx / 1024);
	int col = (idx % 1024);

	highp vec2 uv1 = vec2( col * pixeluv.x, 1.0 - line * pixeluv.y);
	mediump vec4 rgba = texture2D(tex0, uv1);
	vec2 vFontSize = vec2(8.0, 15.0);

	float digit = PrintValue( (fragCoord - vec2(184.0, 5.0)) / vFontSize, rgba.r, 3, 6);
	color = mix( color, vec3(1.0, 0.0, 0.0), digit);
	digit = PrintValue( (fragCoord -  vec2(184.0 + 100, 5.0)) / vFontSize, rgba.g, 3, 6);
	color = mix( color, vec3(1.0, 0.0, 0.0), digit);
	digit = PrintValue( (fragCoord -  vec2(184.0 + 200.0, 5.0)) / vFontSize, rgba.b, 3, 6);
	color = mix( color, vec3(1.0, 0.0, 0.0), digit);
	digit = PrintValue( (fragCoord -  vec2(184.0 + 300.0, 5.0)) / vFontSize, rgba.a, 3, 6);
	color = mix( color, vec3(1.0, 0.0, 0.0), digit);

	digit = PrintValue( (fragCoord -  vec2(184.0 + 500.0, 5.0)) / vFontSize, uv1.x, 3, 6);
	color = mix( color, vec3(0.0, 1.0, 0.0), digit);
	digit = PrintValue( (fragCoord -  vec2(184.0 + 600.0, 5.0)) / vFontSize, uv1.y, 3, 6);
	color = mix( color, vec3(0.0, 1.0, 0.0), digit);


	gl_FragColor = vec4(color, 1.0);
}
