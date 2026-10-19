/* Authored inverse rigid-transform translation. The three-float return view
 * preserves native inline construction and hidden return-buffer ABI; it does
 * not establish original source type names or allocate native global storage.
 */
#include "geometry_interfaces.h"
ObservedMatrixPoint ObservedMatrix4::CameraOrigin() const {
    return ObservedMatrixPoint(-v[0]*v[12]-v[1]*v[13]-v[2]*v[14],
                               -v[4]*v[12]-v[5]*v[13]-v[6]*v[14],
                               -v[8]*v[12]-v[9]*v[13]-v[10]*v[14]);
}
