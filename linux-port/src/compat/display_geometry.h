#ifndef TITBIT_COMPAT_DISPLAY_GEOMETRY_H
#define TITBIT_COMPAT_DISPLAY_GEOMETRY_H
struct CompatViewport { int x, y, width, height, top; };
inline CompatViewport CompatFitViewport(int width, int height, int logical_w, int logical_h, bool stretch) {
 CompatViewport vp = {0, 0, width, height, 0};
 if (width <= 0 || height <= 0 || logical_w <= 0 || logical_h <= 0) { vp.width = vp.height = 0; return vp; }
 if (!stretch) {
  if (width * logical_h > height * logical_w) vp.width = height * logical_w / logical_h;
  else vp.height = width * logical_h / logical_w;
  vp.x = (width - vp.width) / 2; vp.y = (height - vp.height) / 2;
  vp.top = height - vp.y - vp.height;
 }
 return vp;
}
inline bool CompatMapMouse(const CompatViewport &vp, int x, int y, int logical_w, int logical_h, int *lx, int *ly) {
 if (vp.width <= 0 || vp.height <= 0 || x < vp.x || y < vp.top || x >= vp.x + vp.width || y >= vp.top + vp.height) {
  *lx = *ly = -1; return false;
 }
 *lx = (x - vp.x) * logical_w / vp.width;
 *ly = (y - vp.top) * logical_h / vp.height;
 return true;
}
#endif
