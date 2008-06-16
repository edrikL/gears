#ifndef SkCullPoints_DEFINED
#define SkCullPoints_DEFINED

#include "SkRect.h"

class SkCullPoints {
public:
    SkCullPoints();
    SkCullPoints(const SkRect16& r);
    
    void    reset(const SkRect16& r);
    
    /** Start a contour at (x,y). Follow this with call(s) to lineTo(...)
    */
    void    moveTo(int x, int y);
    
    enum LineToResult {
        kNo_Result,             //!< line segment was completely clipped out
        kLineTo_Result,         //!< path.lineTo(pts[1]);
        kMoveToLineTo_Result    //!< path.moveTo(pts[0]); path.lineTo(pts[1]);
    };
    /** Connect a line to the previous call to lineTo (or moveTo).
    */
    LineToResult lineTo(int x, int y, SkPoint16 pts[2]);

private:
    SkRect16    fR;             // the caller's rectangle
    SkPoint16   fAsQuad[4];     // cache of fR as 4 points
    SkPoint32   fPrevPt;        // private state
    LineToResult fPrevResult;   // private state
    
    bool sect_test(int x0, int y0, int x1, int y1) const;
};

/////////////////////////////////////////////////////////////////////////////////

class SkPath;

/** \class SkCullPointsPath

    Similar to SkCullPoints, but this class handles the return values
    from lineTo, and automatically builds a SkPath with the result(s).
*/
class SkCullPointsPath {
public:
    SkCullPointsPath();
    SkCullPointsPath(const SkRect16& r, SkPath* dst);

    void reset(const SkRect16& r, SkPath* dst);
    
    void    moveTo(int x, int y);
    void    lineTo(int x, int y);

private:
    SkCullPoints    fCP;
    SkPath*         fPath;
};

#endif
