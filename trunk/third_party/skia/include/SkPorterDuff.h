#ifndef SkPorterDuff_DEFINED
#define SkPorterDuff_DEFINED

#include "SkColor.h"

class SkXfermode;

class SkPorterDuff {
public:
	/**	List of predefined xfermodes. In general, the algebra for the modes
		uses the following symbols:
		Sa, Sc	- source alpha and color
		Da, Dc - destination alpha and color (before compositing)
		[a, c] - Resulting (alpha, color) values
		For these equations, the colors are in premultiplied state.
		If no xfermode is specified, kSrcOver is assumed.
	*/
	enum Mode {
		kClear_Mode,	//!< [0, 0]
		kSrc_Mode,		//!< [Sa, Sc]
		kDst_Mode,		//!< [Da, Dc]
		kSrcOver_Mode,	//!< [Sa + (1 - Sa)*Da, Rc = Sc + (1 - Sa)*Dc] this is the default mode
		kDstOver_Mode,	//!< [Sa + (1 - Sa)*Da, Rc = Dc + (1 - Da)*Sc]
		kSrcIn_Mode,	//!< [Sa * Da, Sc * Da]
		kDstIn_Mode,	//!< [Sa * Da, Sa * Dc]
		kSrcOut_Mode,	//!< [Sa * (1 - Da), Sc * (1 - Da)]
		kDstOut_Mode,	//!< [Da * (1 - Sa), Dc * (1 - Sa)]
		kSrcATop_Mode,	//!< [Da, Sc * Da + (1 - Sa) * Dc]
		kDstATop_Mode,	//!< [Sa, Sa * Dc + Sc * (1 - Da)]
		kXor_Mode,		//!< [Sa + Da - 2 * Sa * Da, Sc * (1 - Da) + (1 - Sa) * Dc]
        kDarken_Mode,   //!< [Sa + Da - Sa\u00B7Da, Sc\u00B7(1 - Da) + Dc\u00B7(1 - Sa) + min(Sc, Dc)]
        kLighten_Mode,  //!< [Sa + Da - Sa\u00B7Da, Sc\u00B7(1 - Da) + Dc\u00B7(1 - Sa) + max(Sc, Dc)]

		kModeCount
	};
	/**	Return an SkXfermode object for the specified mode.
	*/
	static SkXfermode* CreateXfermode(Mode mode);
    
    /** Return a function pointer to a routine that applies the specified porter-duff
        transfer mode.
    */
    static SkXfermodeProc GetXfermodeProc(Mode mode);
};

#endif

