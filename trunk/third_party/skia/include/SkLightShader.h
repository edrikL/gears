#ifndef SkLightShader_DEFINED
#define SkLightShader_DEFINED

#include "SkShader.h"

/**	\class SkLightShader

	SkLightShader applies a simple lighting equation (color * mul + add) to
	the colors it is given. Those colors can either be the SkPaint's color if proxy
	is nil, or the colors are those returned from the proxy shader.
*/
class SkLightShader : public SkShader {
public:
	SkLightShader(SkShader* proxy, U8CPU mul, U8CPU add);
	/**	The mul[] and add[] parameters either hold 3 values (R,G,B), or they
		are nil, in which case default values are used (255 for mul, 0 for add)
	*/
	SkLightShader(SkShader* proxy, const U8 mul[3], const U8 add[3]);
	virtual ~SkLightShader();

	// overrides
	virtual U32	 getFlags();
	virtual bool setContext(const SkBitmap& device,
							const SkPaint& paint,
							const SkMatrix& matrix);
	virtual void shadeSpan(int x, int y, SkPMColor span[], int count);

private:
	SkShader*	fProxy;
	SkPMColor	fPMColor;	// in case fProxy is nil
	U8			fMul[3], fAdd[3];
};

#endif

